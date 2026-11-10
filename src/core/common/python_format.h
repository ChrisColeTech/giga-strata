#pragma once

#include <QString>

// Python format-spec output, byte for byte: report lines are golden-compared (02 H8).
// Python rounds the exact binary value and breaks exact ties to even ("0.25" -> "0.2").
namespace python_format {

// f"{value:.{digits}f}"
QString fixed(double value, int digits);

// strata_planner._gib: f"{bytes / 1024**3:.1f} GiB"
QString gib(qint64 bytes);

// f"{value:,}"
QString thousands(qint64 value);

// str(list) of ints: "[0, 1]"
QString intList(const QVector<int> &values);

} // namespace python_format
