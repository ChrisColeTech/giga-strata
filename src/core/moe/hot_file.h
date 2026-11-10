#pragma once

#include "common/result.h"
#include "moe/hot_map.h"

#include <QByteArray>
#include <QString>

// The llama-server --moe-hot file: moe_hot.write_hot and read_hot. Existing hot files written
// by the Python launcher must keep reading back the same, so both directions are byte-exact.
class HotFile {
public:
    // write_hot: header line, then "<layer>: all|none|<experts>" per layer, "\n" line ends.
    static QByteArray write(int expertCount, const HotMap &hot);

    // read_hot. A later line for the same layer replaces an earlier one, as in Python. `path` only
    // goes into the error text ("<path>: not a llama-moe-hot file for N experts").
    static Result<HotMap> read(const QByteArray &text, int expertCount, const QString &path = QString());
};
