#include "gguf/quant_name.h"

#include "common/model_names.h"

#include <QHash>
#include <QRegularExpression>

QString QuantName::of(const QVariant &fileType, const QString &fileName) {
    if (fileType.isValid() && !fileType.isNull()) {
        const QString name = fromFileType(fileType.toLongLong());
        if (!name.isEmpty()) {
            return name;
        }
    }
    return fromFileName(fileName);
}

QString QuantName::fromFileType(qint64 fileType) {
    // llama.h llama_ftype in the bundled fork (third_party\llama.cpp-upstream); gaps are removed types.
    static const QHash<qint64, QString> NAMES{
        {0, QStringLiteral("F32")},      {1, QStringLiteral("F16")},      {2, QStringLiteral("Q4_0")},
        {3, QStringLiteral("Q4_1")},     {7, QStringLiteral("Q8_0")},     {8, QStringLiteral("Q5_0")},
        {9, QStringLiteral("Q5_1")},     {10, QStringLiteral("Q2_K")},    {11, QStringLiteral("Q3_K_S")},
        {12, QStringLiteral("Q3_K_M")},  {13, QStringLiteral("Q3_K_L")},  {14, QStringLiteral("Q4_K_S")},
        {15, QStringLiteral("Q4_K_M")},  {16, QStringLiteral("Q5_K_S")},  {17, QStringLiteral("Q5_K_M")},
        {18, QStringLiteral("Q6_K")},    {19, QStringLiteral("IQ2_XXS")}, {20, QStringLiteral("IQ2_XS")},
        {21, QStringLiteral("Q2_K_S")},  {22, QStringLiteral("IQ3_XS")},  {23, QStringLiteral("IQ3_XXS")},
        {24, QStringLiteral("IQ1_S")},   {25, QStringLiteral("IQ4_NL")},  {26, QStringLiteral("IQ3_S")},
        {27, QStringLiteral("IQ3_M")},   {28, QStringLiteral("IQ2_S")},   {29, QStringLiteral("IQ2_M")},
        {30, QStringLiteral("IQ4_XS")},  {31, QStringLiteral("IQ1_M")},   {32, QStringLiteral("BF16")},
        {36, QStringLiteral("TQ1_0")},   {37, QStringLiteral("TQ2_0")},   {38, QStringLiteral("MXFP4_MOE")},
        {39, QStringLiteral("NVFP4")},   {40, QStringLiteral("Q1_0")},    {41, QStringLiteral("Q2_0")},
        {42, QStringLiteral("F8_E4M3")},
    };
    return NAMES.value(fileType);
}

QString QuantName::fromFileName(const QString &fileName) {
    static const QRegularExpression TOKEN(
        QStringLiteral("(?:^|[-_.])((?:UD-)?I?Q\\d(?:_[A-Z0-9]+)*|BF16|F16|F32|MXFP4(?:_MOE)?|\\d+(?:\\.\\d+)?bit)"
                       "(?=$|[-_.])"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = TOKEN.match(ModelNames::modelStem(fileName));
    return match.hasMatch() ? match.captured(1) : QString();
}
