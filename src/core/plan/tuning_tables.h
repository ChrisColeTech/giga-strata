#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

#include <optional>

// llama_launcher ARCH_TUNING and MTP_SPEC: per-architecture settings measured on 2x RTX 5090.
// Optional flags apply only when the server build understands them, in the listed order.
struct ArchTuning {
    QStringList args;
    QList<QPair<QString, QStringList>> argsIfSupported; // flag the build must list -> args to add
    QList<QPair<QString, QString>> environment;         // ordered like the Python dict
};

struct MtpSpeculation {
    QStringList args;
    std::optional<QStringList> argsWithNgram; // used when the build supports ngram-mod
};

class TuningTables {
public:
    static std::optional<ArchTuning> tuningFor(const QString &arch);
    static std::optional<MtpSpeculation> speculationFor(const QString &arch);
};
