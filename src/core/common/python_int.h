#pragma once

#include <QString>

#include <optional>

// Python int(text) in base 10, which the reference applies to hot-file lines and LM Studio
// folder versions: surrounding whitespace, a sign, "_" between digits and any Unicode decimal
// digit are accepted ("  +1_0 " and "١٢" both parse). Values beyond qint64 are rejected.
namespace python_int {

std::optional<qint64> parse(const QString &text);

} // namespace python_int
