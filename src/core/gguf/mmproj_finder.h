#pragma once

#include <QString>
#include <QStringList>

#include <optional>

class ModelSource;

// strata_planner vision-projector helpers: find_mmproj, mmproj_matches, mmproj_reserve_bytes.
class MmprojFinder {
public:
    // A projector next to the model, in its parent's models\ folder or in searchDirectories,
    // matched by embedding width.
    static std::optional<QString> find(const ModelSource &source, const QString &modelPath,
                                       const QStringList &searchDirectories);

    // True when mmprojPath is a CLIP projector whose output width equals the model's embedding.
    static bool matches(const ModelSource &source, const QString &modelPath, const QString &mmprojPath);

    // VRAM the projector needs on its GPU: the file plus 1 GiB for the image encoder's compute.
    static std::optional<qint64> reserveBytes(const ModelSource &source, const QString &mmprojPath);
};
