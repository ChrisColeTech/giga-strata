#include "common/python_int.h"

#include "common/python_text.h"

#include <limits>

std::optional<qint64> python_int::parse(const QString &text) {
    qsizetype begin = 0;
    qsizetype end = text.size();
    while (begin < end && python_text::isSpace(text.at(begin))) {
        ++begin;
    }
    while (end > begin && python_text::isSpace(text.at(end - 1))) {
        --end;
    }
    bool isNegative = false;
    if (begin < end && (text.at(begin) == u'+' || text.at(begin) == u'-')) {
        isNegative = text.at(begin) == u'-';
        ++begin;
    }
    if (begin == end) {
        return std::nullopt;
    }
    constexpr qint64 MAXIMUM = std::numeric_limits<qint64>::max();
    qint64 value = 0;
    bool previousWasDigit = false;
    for (qsizetype at = begin; at < end; ++at) {
        const QChar character = text.at(at);
        if (character == u'_') {
            // One underscore, only between two digits.
            if (!previousWasDigit || at + 1 == end) {
                return std::nullopt;
            }
            previousWasDigit = false;
            continue;
        }
        const int digit = character.category() == QChar::Number_DecimalDigit ? character.digitValue() : -1;
        if (digit < 0 || value > (MAXIMUM - digit) / 10) {
            return std::nullopt;
        }
        value = value * 10 + digit;
        previousWasDigit = true;
    }
    return isNegative ? -value : value;
}
