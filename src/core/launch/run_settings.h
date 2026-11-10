#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

// The dict llama_launcher.read_settings returns: one model's run parameters with text fields
// already converted (02 H12 keeps them as strings on disk). Python key in brackets.
struct RunSettings {
    QString binary;
    QString modelPath;             // [model]
    QVector<int> gpuIds;           // [gpu_ids] nvidia-smi indices in CUDA order
    qint64 contextSize = 0;        // [ctx]
    qint64 marginMib = 0;          // [margin]
    std::optional<int> microBatch; // [ubatch] nullopt = "auto"
    QString host;
    QString port;
    bool useMmap = true;           // [mmap] allow SSD streaming
    bool useFlashAttention = true; // [fa]
    QStringList extraArgs;         // [extra] tokenized
    bool autoServer = true;        // [auto_server]
    bool useMtp = true;            // [mtp]
    bool useVision = false;        // [vision]
    QString mmprojPath;            // [mmproj] empty = search for one
    bool isRecordingUsage = false; // [record_usage]
    bool usePerExpert = false;     // [per_expert]
};
