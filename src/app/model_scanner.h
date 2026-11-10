#pragma once

#include "model_entry.h"

#include <QString>
#include <QStringList>
#include <QVector>

class ModelSource;

// The model library scan (05 section 1, 04 section 2.3): every *.gguf under the folders,
// recursively; a split set is one row (its -00001-of- file); MTP drafts (the find_mtp_draft
// rule), vision projectors (clip) and files whose layout does not read are not rows. Blocking;
// ModelLibrary runs it on a worker.
class ModelScanner {
public:
    static QVector<ModelEntry> scan(const ModelSource &source, const QStringList &folders,
                                    const QString &usageDirectory);

    // True for "<base>-0000N-of-0000M.gguf" with N > 1: those shards belong to their first one.
    static bool isLaterShard(const QString &path);
};
