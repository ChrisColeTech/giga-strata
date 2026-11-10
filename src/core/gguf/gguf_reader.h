#pragma once

#include "common/result.h"
#include "gguf/gguf_shard.h"

#include <QByteArray>
#include <QString>

class ModelSource;

// Reads a GGUF header (metadata + tensor table) without touching tensor data, exactly as
// strata_planner._read_shard does, including which string arrays it skips.
class GgufReader {
public:
    // Served from the source's ShardCache when it has one.
    static Result<GgufShard> read(const ModelSource &source, const QString &path);

    // Parses a header held in memory. `fileSize` is the whole file's size, needed for the last
    // tensor. Fails with isTruncated set when `header` ends before the tensor table does.
    static Result<GgufShard> parse(const QByteArray &header, qint64 fileSize, const QString &fileName,
                                   bool *isTruncated);

private:
    static Result<GgufShard> readUncached(const ModelSource &source, const QString &path, qint64 knownSize);
};
