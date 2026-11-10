#include "plan/tuning_tables.h"

std::optional<ArchTuning> TuningTables::tuningFor(const QString &arch) {
    if (arch == QLatin1String("qwen4exp")) {
        // Qwen 3.8: 114 -> 119 tok/s without MTP
        return ArchTuning{{QStringLiteral("-t"), QStringLiteral("8")},
                          {{QStringLiteral("--backend-sampling"), {QStringLiteral("-bs")}},
                           {QStringLiteral("--lazy-mode"), {QStringLiteral("--lazy-mode"), QStringLiteral("off")}}},
                          {{QStringLiteral("GGML_CUDA_GRAPH_CHUNK_SIZE"), QStringLiteral("0")}}};
    }
    if (arch == QLatin1String("glm5-next")) {
        // GLM 5.3 runs its RAM-tier experts on the CPU; 24 threads on the 285K throttle within ~20 s
        // of decoding (96 ms/token), 16 hold 58-64 ms/token
        return ArchTuning{{QStringLiteral("-t"), QStringLiteral("16")},
                          {{QStringLiteral("--backend-sampling"), {QStringLiteral("-bs")}}},
                          {}};
    }
    return std::nullopt;
}

std::optional<MtpSpeculation> TuningTables::speculationFor(const QString &arch) {
    const QString draftMtp = QStringLiteral("draft-mtp");
    if (arch == QLatin1String("qwen4exp")) {
        return MtpSpeculation{{QStringLiteral("--spec-type"), draftMtp, QStringLiteral("--spec-draft-n-max"), QStringLiteral("3")},
                              std::nullopt};
    }
    if (arch == QLatin1String("glm5-next")) {
        // each verified draft token costs GLM a share of a RAM-tier expert read, so it drafts one:
        // 22.2 -> 24.5-26.1 tok/s; with ngram-mod in front 29.2 on a 4K prompt, 31.3 on edits
        return MtpSpeculation{{QStringLiteral("--spec-type"), draftMtp, QStringLiteral("--spec-draft-n-max"), QStringLiteral("1")},
                              QStringList{QStringLiteral("--spec-type"), QStringLiteral("ngram-mod,draft-mtp"),
                                          QStringLiteral("--spec-draft-n-max"), QStringLiteral("1"),
                                          QStringLiteral("--spec-ngram-mod-n-max"), QStringLiteral("3"),
                                          QStringLiteral("--spec-ngram-mod-n-min"), QStringLiteral("1")}};
    }
    return std::nullopt;
}
