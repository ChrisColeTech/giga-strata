#include "moe/hot_file.h"

#include "common/python_int.h"
#include "common/python_text.h"

#include <QStringList>

#include <algorithm>
#include <limits>

namespace {

QString header(int expertCount) {
    return QStringLiteral("llama-moe-hot 1 n_expert=%1").arg(expertCount);
}

std::optional<int> layerOrExpert(const QString &text) {
    const auto value = python_int::parse(text);
    if (!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return int(*value);
}

} // namespace

QByteArray HotFile::write(int expertCount, const HotMap &hot) {
    QByteArray text = header(expertCount).toLatin1() + '\n';
    for (auto layer = hot.cbegin(); layer != hot.cend(); ++layer) {
        QByteArray body;
        if (layer.value().size() == expertCount) {
            body = "all";
        } else if (layer.value().isEmpty()) {
            body = "none";
        } else {
            QByteArrayList experts;
            for (const int expert : layer.value()) {
                experts.append(QByteArray::number(expert));
            }
            body = experts.join(' ');
        }
        text += QByteArray::number(layer.key()) + ": " + body + '\n';
    }
    return text;
}

Result<HotMap> HotFile::read(const QByteArray &text, int expertCount, const QString &path) {
    const QString notHot = (path.isEmpty() ? QStringLiteral("This file") : path)
                           + QStringLiteral(": not a llama-moe-hot file for %1 experts").arg(expertCount);
    const QString malformed = path.isEmpty() ? QStringLiteral("The hot expert file is damaged.")
                                             : QStringLiteral("The hot expert file %1 is damaged.").arg(path);
    if (std::any_of(text.cbegin(), text.cend(), [](char byte) { return static_cast<unsigned char>(byte) > 0x7F; })) {
        return Result<HotMap>::failure(malformed);
    }
    const QStringList lines = python_text::splitLines(QString::fromLatin1(text));
    if (lines.isEmpty() || python_text::strip(lines.first()) != header(expertCount)) {
        return Result<HotMap>::failure(notHot);
    }
    HotMap hot;
    for (qsizetype at = 1; at < lines.size(); ++at) {
        const QString &line = lines[at];
        if (python_text::strip(line).isEmpty()) {
            continue;
        }
        const qsizetype colon = line.indexOf(u':');
        const auto layer = colon < 0 ? std::nullopt : layerOrExpert(line.left(colon));
        if (!layer) {
            return Result<HotMap>::failure(malformed);
        }
        const QString body = python_text::strip(line.mid(colon + 1));
        QVector<int> experts;
        if (body == QStringLiteral("all")) {
            for (int expert = 0; expert < expertCount; ++expert) {
                experts.append(expert);
            }
        } else if (body != QStringLiteral("none")) {
            for (const QString &word : python_text::splitWords(body)) {
                const auto expert = layerOrExpert(word);
                if (!expert) {
                    return Result<HotMap>::failure(malformed);
                }
                experts.append(*expert);
            }
            std::sort(experts.begin(), experts.end());
        }
        hot.insert(*layer, experts);
    }
    return Result<HotMap>::success(hot);
}
