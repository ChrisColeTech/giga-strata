#include "common/model_names.h"

#include "common/path_key.h"

#include <QRegularExpression>

#include <cmath>

QString ModelNames::modelKey(const QString &modelPath, qint64 sizeBytes, qint64 modifiedTimeNs) {
    return path_key::of(modelPath) + u'|' + QString::number(sizeBytes) + u'|'
           + QString::number(pythonMtimeSeconds(modifiedTimeNs));
}

QString ModelNames::modelStem(const QString &modelPath) {
    // Python's \d matches any Unicode digit; PCRE needs the Unicode option to agree.
    static const QRegularExpression SPLIT_SUFFIX(
        QStringLiteral("(-\\d{5}-of-\\d{5})?\\.gguf$"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    return baseName(modelPath).replace(SPLIT_SUFFIX, QString());
}

QString ModelNames::usagePath(const QString &usageDirectory, const QString &modelPath) {
    return path_key::join(usageDirectory, modelStem(modelPath) + QStringLiteral(".usage.json"));
}

QString ModelNames::baseName(const QString &path) {
    const qsizetype cut = std::max(path.lastIndexOf(u'\\'), path.lastIndexOf(u'/'));
    if (cut >= 0) {
        return path.mid(cut + 1);
    }
    const bool hasDrive = path.size() >= 2 && path.at(1) == u':';
    return hasDrive ? path.mid(2) : path;
}

QString ModelNames::normcase(const QString &path) {
    QString folded = path.toLower();
    folded.replace(u'/', u'\\');
    return folded;
}

qint64 ModelNames::pythonMtimeSeconds(qint64 modifiedTimeNs) {
    constexpr qint64 NS_PER_SECOND = 1000000000;
    qint64 seconds = modifiedTimeNs / NS_PER_SECOND;
    qint64 fraction = modifiedTimeNs % NS_PER_SECOND;
    if (fraction < 0) {
        fraction += NS_PER_SECOND;
        --seconds;
    }
    // os.stat: st_mtime = sec + nsec * 1e-9 in doubles, which can round up to the next second.
    const double asFloat = double(seconds) + double(fraction) * 1e-9;
    return qint64(std::trunc(asFloat));
}
