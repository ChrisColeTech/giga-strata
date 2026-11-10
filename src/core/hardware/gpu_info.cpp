#include "hardware/gpu_info.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace {

constexpr int CSV_FIELD_COUNT = 4;

QString unreadableRow(const QString &row) {
    return QCoreApplication::translate("GpuInfo", "nvidia-smi printed a GPU row this app cannot read: \"%1\".").arg(row);
}

} // namespace

QStringList GpuInfo::nvidiaSmiArguments() {
    return {QStringLiteral("--query-gpu=index,name,memory.free,memory.total"),
            QStringLiteral("--format=csv,noheader,nounits")};
}

Result<QVector<Gpu>> GpuInfo::parseCsv(const QString &csv) {
    QVector<Gpu> gpus;
    const QStringList rows = csv.trimmed().split(QRegularExpression(QStringLiteral("\\r?\\n")), Qt::SkipEmptyParts);
    for (const QString &row : rows) {
        const QStringList fields = row.split(QLatin1Char(','));
        if (fields.size() != CSV_FIELD_COUNT) {
            return Result<QVector<Gpu>>::failure(unreadableRow(row.trimmed()));
        }
        bool isIndexValid = false;
        bool isFreeValid = false;
        bool isTotalValid = false;
        Gpu gpu;
        gpu.index = fields[0].trimmed().toInt(&isIndexValid);
        gpu.name = fields[1].trimmed();
        gpu.freeMib = fields[2].trimmed().toLongLong(&isFreeValid);
        gpu.totalMib = fields[3].trimmed().toLongLong(&isTotalValid);
        if (!isIndexValid || !isFreeValid || !isTotalValid) {
            return Result<QVector<Gpu>>::failure(unreadableRow(row.trimmed()));
        }
        gpus.append(gpu);
    }
    return Result<QVector<Gpu>>::success(gpus);
}

QString GpuInfo::shortName(const QString &name) {
    static const QRegularExpression vendorPrefix(QStringLiteral("^(NVIDIA\\s+)?(GeForce\\s+)?"));
    const QString shortened = QString(name).remove(vendorPrefix).trimmed();
    return shortened.isEmpty() ? name : shortened;
}
