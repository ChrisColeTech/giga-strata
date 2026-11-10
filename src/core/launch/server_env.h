#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVector>

class ServerEnvironment;

// llama_launcher.server_env and _vendor_dll_dirs: the environment a llama-server runs with.
class ServerEnv {
public:
    // LM Studio keeps the CUDA runtime in <backends>\vendor: entries whose name holds the build's
    // kind ("cuda12", or "cuda" without "cuda12"). Files count as well as folders, and a CUDA 13
    // build gets kind "cuda", exactly as Python lists them.
    static QStringList vendorDllDirectories(const ServerEnvironment &environment, const QString &binary);

    // server_env: CUDA_DEVICE_ORDER=PCI_BUS_ID, CUDA_VISIBLE_DEVICES=<ids>, vendor folders in front
    // of PATH.
    static QProcessEnvironment build(const QProcessEnvironment &base, const QStringList &vendorDirectories,
                                     const QVector<int> &gpuIds);

    // The CPU-only trial-load environment. Python sets CUDA_VISIBLE_DEVICES="", which Windows
    // treats as unset, so its probe initializes CUDA on every GPU; "-1" really hides them (02 D7).
    static QProcessEnvironment forProbe(const QProcessEnvironment &base, const QStringList &vendorDirectories);
};
