#pragma once

#include "common/result.h"
#include "moe/usage_counts.h"

#include <QByteArray>
#include <QPair>
#include <QString>
#include <QStringList>

#include <optional>

class UsageSource;

// moe_hot.read_usage / usage_tokens / read_counts for llama-server usage files
// (LLAMA_MOE_USAGE_OUT). imatrix GGUF counts are not read: the launcher only ever passes usage
// JSON (02 section 2), so any other file is an error.
class UsageIo {
public:
    // read_usage on the file's bytes. bucket: "decode", "batch", or none for decode when the file
    // recorded any decode tokens, else batch. Layers with no counts are left out. The error text
    // is Python's ("not a llama-moe-usage file"); readCounts prefixes the path the way
    // read_usage does, because that message reaches the Run log verbatim.
    static Result<UsageCounts> readUsage(const QByteArray &json, std::optional<QString> bucket = std::nullopt);

    // usage_tokens: (decode, batch) tokens summed over the .json files; other files are skipped.
    static Result<QPair<qint64, qint64>> usageTokens(const UsageSource &source, const QStringList &paths);

    // read_counts: read_usage of every file, summed per layer. Like Python's zip, a layer whose
    // files disagree on the expert count keeps the shorter length.
    static Result<UsageCounts> readCounts(const UsageSource &source, const QStringList &paths);
};
