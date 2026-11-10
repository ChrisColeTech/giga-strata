#pragma once

#include "gguf_builder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

// Small real GGUF files for controller tests: a MoE model whose layout reads like a real one
// (attention, experts, output, embeddings), its MTP draft, and a vision projector. Kilobytes, so
// planning works on them exactly as on a 100 GB model.
class SyntheticModel {
public:
    static constexpr quint64 ATTENTION_BYTES = 4096;
    static constexpr quint64 EXPERT_BYTES = 65536;
    static constexpr quint32 EMBEDDING = 64;

    // `blocks` layers of `arch` with 8 experts; nextn > 0 makes it an MTP draft of a
    // (blocks - nextn)-layer model.
    static QByteArray moe(const QString &arch, quint32 blocks, quint32 nextn = 0, quint32 fileType = 15) {
        GgufBuilder builder;
        builder.addString(QStringLiteral("general.architecture"), arch)
            .addUint32(QStringLiteral("general.file_type"), fileType)
            .addUint32(arch + QStringLiteral(".block_count"), blocks)
            .addUint32(arch + QStringLiteral(".expert_count"), 8)
            .addUint32(arch + QStringLiteral(".expert_used_count"), 2)
            .addUint32(arch + QStringLiteral(".embedding_length"), EMBEDDING)
            .addUint32(arch + QStringLiteral(".attention.head_count"), 4)
            .addUint32(arch + QStringLiteral(".attention.head_count_kv"), 2);
        if (nextn > 0) {
            builder.addUint32(arch + QStringLiteral(".nextn_predict_layers"), nextn);
        }
        quint64 offset = 0;
        for (quint32 block = 0; block < blocks; ++block) {
            builder.addTensor(QStringLiteral("blk.%1.attn_q.weight").arg(block), offset);
            offset += ATTENTION_BYTES;
            builder.addTensor(QStringLiteral("blk.%1.ffn_up_exps.weight").arg(block), offset);
            offset += EXPERT_BYTES;
        }
        builder.addTensor(QStringLiteral("output.weight"), offset);
        offset += ATTENTION_BYTES;
        builder.addTensor(QStringLiteral("token_embd.weight"), offset);
        offset += ATTENTION_BYTES;
        return builder.header() + QByteArray(qsizetype(offset), '\0');
    }

    static QByteArray projector(quint32 projectionDim = EMBEDDING) {
        GgufBuilder builder;
        builder.addString(QStringLiteral("general.architecture"), QStringLiteral("clip"))
            .addUint32(QStringLiteral("clip.vision.projection_dim"), projectionDim)
            .addTensor(QStringLiteral("v.patch_embd.weight"), 0);
        return builder.header() + QByteArray(1024, '\0');
    }

    static QString write(const QString &path, const QByteArray &bytes) {
        QDir().mkpath(QFileInfo(path).path());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(bytes);
        }
        return QDir::toNativeSeparators(path);
    }
};
