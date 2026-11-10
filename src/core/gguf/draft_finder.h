#pragma once

#include <QString>
#include <QStringList>

#include <optional>

class ModelSource;

// strata_planner.find_mtp_draft: a small GGUF of the same architecture whose only extra layer is
// the multi-token-prediction block, next to the model or below the search directories.
class DraftFinder {
public:
    static std::optional<QString> find(const ModelSource &source, const QString &modelPath,
                                       const QStringList &searchDirectories);
};
