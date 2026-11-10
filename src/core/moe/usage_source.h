#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

// Reads recorded expert usage and existing hot files, so goldens can serve fixtures
// (02 section 5). Writing hot files is not here: planning is pure (02 D1).
class UsageSource {
public:
    virtual ~UsageSource() = default;

    virtual bool exists(const QString &path) const = 0;
    virtual std::optional<QByteArray> readAll(const QString &path) const = 0;
    virtual std::optional<qint64> fileSize(const QString &path) const = 0;
    virtual std::optional<qint64> modifiedTimeNs(const QString &path) const = 0;
};
