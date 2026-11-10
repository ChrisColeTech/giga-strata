#pragma once

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPair>
#include <QString>
#include <QVector>

// The small golden fixtures (tests/golden/fixtures/small + expected/small, README section 6):
// each case of fixtures/<name>.json paired with the expected case of the same index and id.
class GoldenCases {
public:
    static QString root() { return QStringLiteral(GIGA_STRATA_TEST_DATA "/../../golden"); }

    static QJsonObject readObject(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    static QVector<QPair<QJsonObject, QJsonObject>> small(const QString &name) {
        const QJsonArray inputs =
            readObject(root() + QStringLiteral("/fixtures/small/") + name + QStringLiteral(".json"))
                .value(QStringLiteral("cases"))
                .toArray();
        const QJsonArray expected =
            readObject(root() + QStringLiteral("/expected/small/") + name + QStringLiteral(".json"))
                .value(QStringLiteral("cases"))
                .toArray();
        QVector<QPair<QJsonObject, QJsonObject>> cases;
        for (qsizetype at = 0; at < inputs.size() && at < expected.size(); ++at) {
            cases.append({inputs.at(at).toObject(), expected.at(at).toObject()});
        }
        return cases;
    }
};
