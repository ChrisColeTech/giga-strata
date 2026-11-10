#include "golden_scenario.h"

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

// Every scenario of tests/golden/fixtures/scenarios through the C++ Run path, compared field by
// field with what the Python launcher did (README section 5.2). Prints identical/total and, for
// each scenario that differs, its first differing field and entry.
namespace {

const QString ROOT = QStringLiteral(GIGA_STRATA_GOLDEN);

// Compared fields; README 5.2 lists more (plan_call, trace, messagebox) that are diagnostics.
const QStringList FIELDS{QStringLiteral("args"),        QStringLiteral("report"),     QStringLiteral("warnings"),
                         QStringLiteral("use_mmap"),    QStringLiteral("ubatch"),     QStringLiteral("arch"),
                         QStringLiteral("hot_path"),    QStringLiteral("hot_file_text"), QStringLiteral("argv"),
                         QStringLiteral("env_delta"),   QStringLiteral("log"),        QStringLiteral("error"),
                         QStringLiteral("error_stage"), QStringLiteral("files_after")};

QJsonValue orNull(const QJsonValue &value) {
    return value.isUndefined() ? QJsonValue() : value;
}

QString shown(const QJsonValue &value) {
    if (value.isString()) {
        return value.toString().left(300);
    }
    const QJsonDocument document = value.isArray() ? QJsonDocument(value.toArray()) : QJsonDocument(value.toObject());
    return value.isArray() || value.isObject() ? QString::fromUtf8(document.toJson(QJsonDocument::Compact)).left(300)
                                               : value.toVariant().toString();
}

QString firstDifference(const QString &field, const QJsonValue &expected, const QJsonValue &actual) {
    if (expected.isArray() && actual.isArray()) {
        const QJsonArray left = expected.toArray();
        const QJsonArray right = actual.toArray();
        for (qsizetype at = 0; at < std::max(left.size(), right.size()); ++at) {
            if (left.at(at) != right.at(at)) {
                return QStringLiteral("%1[%2]\n    expected: %3\n    actual:   %4")
                    .arg(field)
                    .arg(at)
                    .arg(shown(left.at(at)), shown(right.at(at)));
            }
        }
    }
    return QStringLiteral("%1\n    expected: %2\n    actual:   %3").arg(field, shown(expected), shown(actual));
}

// README 5.2: an OSError raised by the OS itself carries CPython's text; only its presence is
// compared, in the error field and in the log's CRITICAL line.
void exemptOsError(QJsonObject &expected, const QJsonObject &actual) {
    const QString error = expected.value(QStringLiteral("error")).toString();
    if (!error.startsWith(QStringLiteral("[Errno ")) || actual.value(QStringLiteral("error")).toString().isEmpty()) {
        return;
    }
    const QString own = actual.value(QStringLiteral("error")).toString();
    expected.insert(QStringLiteral("error"), own);
    QJsonArray log = expected.value(QStringLiteral("log")).toArray();
    for (qsizetype at = 0; at < log.size(); ++at) {
        log[at] = log.at(at).toString().replace(error, own);
    }
    expected.insert(QStringLiteral("log"), log);
}

} // namespace

class GoldenTest : public QObject {
    Q_OBJECT

private slots:
    void everyScenarioMatchesPython() {
        const GoldenModels models(ROOT + QStringLiteral("/fixtures/models"));
        const GoldenScenario scenario(models, ROOT + QStringLiteral("/fixtures/usage"));
        const QJsonArray ids = GoldenModels::readJson(ROOT + QStringLiteral("/fixtures/scenarios/index.json"))
                                   .value(QStringLiteral("scenarios"))
                                   .toArray();
        const QString startDirectory = QDir::currentPath();
        QStringList failures;
        QElapsedTimer timer;
        QPair<qint64, QString> slowest;
        timer.start();
        for (const QJsonValue &id : ids) {
            const QString name = id.toString() + QStringLiteral(".json");
            const QJsonObject fixture = GoldenModels::readJson(ROOT + QStringLiteral("/fixtures/scenarios/") + name);
            QJsonObject expected = GoldenModels::readJson(ROOT + QStringLiteral("/expected/scenarios/") + name);
            QTemporaryDir work;
            QVERIFY(work.isValid() && QDir::setCurrent(work.path()));
            const qint64 started = timer.elapsed();
            const QJsonObject actual = scenario.run(fixture);
            slowest = std::max(slowest, qMakePair(timer.elapsed() - started, id.toString()));
            QDir::setCurrent(startDirectory);
            exemptOsError(expected, actual);
            for (const QString &field : FIELDS) {
                const QJsonValue want = orNull(expected.value(field));
                const QJsonValue got = orNull(actual.value(field));
                if (want != got) {
                    failures.append(id.toString() + u' ' + firstDifference(field, want, got));
                    break;
                }
            }
        }
        const qsizetype identical = ids.size() - failures.size();
        qInfo().noquote() << QStringLiteral("golden scenarios: %1/%2 identical in %3 ms (slowest %4, %5 ms)")
                                 .arg(identical)
                                 .arg(ids.size())
                                 .arg(timer.elapsed())
                                 .arg(slowest.second)
                                 .arg(slowest.first);
        for (const QString &failure : failures) {
            qWarning().noquote() << failure;
        }
        QVERIFY(ids.size() > 0);
        QCOMPARE(failures.size(), 0);
    }
};

QTEST_GUILESS_MAIN(GoldenTest)
#include "golden_test.moc"
