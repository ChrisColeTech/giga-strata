#include "launch/run_planner.h"

#include "gguf/draft_finder.h"
#include "gguf/layout_reader.h"
#include "gguf/mmproj_finder.h"
#include "gguf/model_source.h"
#include "launch/expert_usage_args.h"
#include "plan/buffer_model.h"
#include "plan/placement_planner.h"
#include "plan/tuning_tables.h"

namespace {

constexpr int DRAFT_SIZING_MICRO_BATCH = 2048;

// One Run in progress: the log so far and the model layout, read once (02 D4).
class RunState {
public:
    RunState(const RunSettings &settings, const RunEnvironment &environment)
        : m_settings(settings), m_environment(environment) {}

    RunPlan &run() { return m_run; }
    void log(const QString &line) { m_run.logLines.append(line); }

    bool fail(const QString &error) {
        m_run.error = error;
        m_run.plan.reset();
        log(RunPlanner::criticalLine(error));
        return false;
    }

    bool readLayout() {
        if (m_run.layout) {
            return true;
        }
        const auto layout = LayoutReader::read(*m_environment.models, m_settings.modelPath);
        if (!layout.ok()) {
            return fail(layout.error());
        }
        m_run.layout = layout.value();
        return true;
    }

    // find_draft
    bool findDraft() {
        if (!m_settings.useMtp) {
            return true;
        }
        if (!readLayout()) {
            return false;
        }
        if (!TuningTables::speculationFor(m_run.layout->arch)) {
            return true;
        }
        const auto draft = DraftFinder::find(*m_environment.models, m_settings.modelPath,
                                             {m_environment.modelsDirectory});
        if (!draft) {
            return true;
        }
        if (!m_environment.flags.contains(QStringLiteral("draft-mtp"))) {
            log(QStringLiteral("[MTP] Found %1, but this llama-server build can't use MTP drafts; not speculating\n")
                    .arg(*draft));
            return true;
        }
        log(QStringLiteral("[MTP] Speculating with %1\n").arg(*draft));
        m_run.draftPath = draft;
        return true;
    }

    // find_mmproj
    bool findMmproj() {
        if (!m_settings.useVision) {
            return true;
        }
        const ModelSource &models = *m_environment.models;
        const auto path = !m_settings.mmprojPath.isEmpty()
                              ? std::optional<QString>(m_settings.mmprojPath)
                              : MmprojFinder::find(models, m_settings.modelPath, {m_environment.modelsDirectory});
        if (!path) {
            log(QStringLiteral("[VISION] No vision projector (mmproj) found for this model; text only\n"));
            return true;
        }
        if (!models.exists(*path)) {
            return fail(QStringLiteral("vision projector not found: %1").arg(*path));
        }
        if (!MmprojFinder::matches(models, m_settings.modelPath, *path)) {
            log(QStringLiteral("[VISION] WARNING: %1 does not look like a projector for this model (embedding width "
                               "differs); the server will probably refuse it\n")
                    .arg(*path));
        }
        if (!m_environment.flags.contains(QStringLiteral("--mmproj-device"))) {
            log(QStringLiteral("[VISION] This llama-server build has no --mmproj-device; the projector goes to the "
                               "first GPU\n"));
        }
        log(QStringLiteral("[VISION] Image input with %1\n").arg(*path));
        m_run.mmprojPath = path;
        return true;
    }

    // The draft and the projector run on the last GPU; with -ub auto the planner may pick 2048,
    // so the draft is sized for that.
    bool reserveBytes(qint64 &reserve) {
        reserve = 0;
        const ModelSource &models = *m_environment.models;
        if (m_run.draftPath) {
            const auto size = models.fileSize(*m_run.draftPath);
            if (!size) {
                return fail(QStringLiteral("Cannot read %1.").arg(*m_run.draftPath));
            }
            if (!readLayout()) {
                return false;
            }
            const int micro = m_settings.microBatch.value_or(-1);
            reserve = BufferModel::draftReserveBytes(*m_run.layout, *size, micro >= 0 ? micro : DRAFT_SIZING_MICRO_BATCH,
                                                     m_settings.contextSize);
        }
        if (m_run.mmprojPath) {
            const auto projector = MmprojFinder::reserveBytes(models, *m_run.mmprojPath);
            if (!projector) {
                return fail(QStringLiteral("Cannot read %1.").arg(*m_run.mmprojPath));
            }
            reserve += *projector;
        }
        return true;
    }

    // make_plan, launcher side: reserve, per-expert arguments, the planner, its report.
    bool makePlan(const std::atomic<bool> *cancel) {
        log(QStringLiteral("[PLANNER] Reading model layout and measuring free VRAM / RAM...\n"));
        qint64 reserve = 0;
        if (!reserveBytes(reserve)) {
            return false;
        }
        const auto perExpert = ExpertUsageArgs::decide(m_settings, m_environment, reserve);
        if (!perExpert.ok()) {
            return fail(perExpert.error());
        }
        m_run.logLines += perExpert.value().logLines;
        if (perExpert.value().request) {
            m_run.hotPathToPrune = perExpert.value().request->hotFilePath;
        }
        if (!readLayout()) {
            return false;
        }
        PlanRequest request;
        request.layout = *m_run.layout;
        request.gpuIds = m_settings.gpuIds;
        request.contextSize = m_settings.contextSize;
        request.marginMib = m_settings.marginMib;
        request.allowSsd = m_settings.useMmap;
        request.microBatch = m_settings.microBatch;
        request.reserveLastBytes = reserve;
        m_run.reserveLastBytes = reserve;
        request.perExpert = perExpert.value().request;
        const auto plan = PlacementPlanner::plan(request, m_environment.hardware, cancel);
        if (!plan.ok()) {
            return fail(plan.error());
        }
        for (const QString &line : plan.value().report) {
            log(QStringLiteral("[PLANNER] %1\n").arg(line));
        }
        for (const QString &line : plan.value().warnings) {
            log(QStringLiteral("[PLANNER] WARNING: %1\n").arg(line));
        }
        m_run.plan = plan.value();
        return true;
    }

private:
    const RunSettings &m_settings;
    const RunEnvironment &m_environment;
    RunPlan m_run;
};

} // namespace

RunPlan RunPlanner::plan(const RunSettings &settings, const RunEnvironment &environment,
                         const std::atomic<bool> *cancel) {
    RunState state(settings, environment);
    if (state.findDraft() && state.findMmproj()) {
        state.makePlan(cancel);
    }
    return state.run();
}

QString RunPlanner::criticalLine(const QString &error) {
    return QStringLiteral("\n[CRITICAL RUNTIME ERROR]: %1\n").arg(error);
}
