#include "gguf/shard_finder.h"

#include "gguf/model_source.h"

#include <QFileInfo>
#include <QRegularExpression>

Result<QStringList> ShardFinder::shardPaths(const ModelSource &source, const QString &path) {
    static const QRegularExpression shardPattern(QStringLiteral("^(.*)-(\\d{5})-of-(\\d{5})\\.gguf$"),
                                                 QRegularExpression::CaseInsensitiveOption);
    const auto match = shardPattern.match(path);
    if (!match.hasMatch()) {
        return Result<QStringList>::success({path});
    }
    const QString base = match.captured(1);
    const QString total = match.captured(3);
    // Python rebuilds every name with a lowercase ".gguf" and the original total digits.
    QStringList paths;
    QStringList missing;
    for (int i = 1; i <= total.toInt(); ++i) {
        const QString shard = QStringLiteral("%1-%2-of-%3.gguf").arg(base).arg(i, 5, 10, QLatin1Char('0')).arg(total);
        paths.append(shard);
        if (!source.exists(shard)) {
            missing.append(QFileInfo(shard).fileName());
        }
    }
    if (!missing.isEmpty()) {
        return Result<QStringList>::failure(QStringLiteral("missing split file(s): %1").arg(missing.join(QStringLiteral(", "))));
    }
    return Result<QStringList>::success(paths);
}
