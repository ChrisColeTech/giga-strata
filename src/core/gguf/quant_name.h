#pragma once

#include <QString>
#include <QVariant>

// The quantization a model is published as (04 section 2.1, Quant column): general.file_type
// mapped to llama.cpp's llama_ftype names, else the quant token in the file name, else empty.
class QuantName {
public:
    static QString of(const QVariant &fileType, const QString &fileName);

    // LLAMA_FTYPE_MOSTLY_<name> for a general.file_type value; empty when unknown.
    static QString fromFileType(qint64 fileType);

    // "Q4_K_M", "IQ3_XXS", "BF16", "3.0bit", ... as written in the file name; empty when none.
    static QString fromFileName(const QString &fileName);
};
