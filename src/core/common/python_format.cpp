#include "common/python_format.h"

#include <QStringList>
#include <QVector>

#include <array>
#include <charconv>

QString python_format::fixed(double value, int digits) {
    // std::to_chars with a precision rounds the exact binary value, ties to even, like Python.
    std::array<char, 400> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed, digits);
    return QString::fromLatin1(buffer.data(), qsizetype(result.ptr - buffer.data()));
}

QString python_format::gib(qint64 bytes) {
    constexpr double BYTES_PER_GIB = 1024.0 * 1024.0 * 1024.0;
    return fixed(double(bytes) / BYTES_PER_GIB, 1) + QStringLiteral(" GiB");
}

QString python_format::thousands(qint64 value) {
    const bool isNegative = value < 0;
    QString digits = QString::number(isNegative ? -value : value);
    for (qsizetype at = digits.size() - 3; at > 0; at -= 3) {
        digits.insert(at, u',');
    }
    return isNegative ? u'-' + digits : digits;
}

QString python_format::intList(const QVector<int> &values) {
    QStringList parts;
    for (const int value : values) {
        parts.append(QString::number(value));
    }
    return u'[' + parts.join(QStringLiteral(", ")) + u']';
}
