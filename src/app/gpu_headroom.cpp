#include "gpu_headroom.h"

#include <QStringList>

#include <algorithm>
#include <cmath>

QMap<int, double> GpuHeadroom::parse(const QString &text) {
    QMap<int, double> gigabytes;
    for (const QString &entry : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QStringList parts = entry.split(QLatin1Char(':'));
        if (parts.size() != 2) {
            continue;
        }
        bool isIndex = false;
        bool isAmount = false;
        const int index = parts[0].trimmed().toInt(&isIndex);
        const double amount = parts[1].trimmed().toDouble(&isAmount);
        if (isIndex && isAmount && index >= 0 && amount >= 0 && std::isfinite(amount)) {
            gigabytes.insert(index, amount);
        }
    }
    return gigabytes;
}

QString GpuHeadroom::format(const QMap<int, double> &gigabytes) {
    QStringList entries;
    for (auto it = gigabytes.cbegin(); it != gigabytes.cend(); ++it) {
        entries.append(QStringLiteral("%1:%2").arg(it.key()).arg(QString::number(it.value(), 'g', 6)));
    }
    return entries.join(QLatin1Char(','));
}

QString GpuHeadroom::merged(const QString &appWide, const QString &model) {
    QMap<int, double> result = parse(appWide);
    const QMap<int, double> overrides = parse(model);
    for (auto it = overrides.cbegin(); it != overrides.cend(); ++it) {
        result.insert(it.key(), it.value());
    }
    return format(result);
}

void GpuHeadroom::apply(HardwareSnapshot &hardware, const QString &text) {
    const QMap<int, double> gigabytes = parse(text);
    for (Gpu &gpu : hardware.gpus) {
        const qint64 headroomMib = std::llround(gigabytes.value(gpu.index, 0.0) * 1024.0);
        gpu.freeMib = std::max<qint64>(gpu.freeMib - headroomMib, 0);
    }
}
