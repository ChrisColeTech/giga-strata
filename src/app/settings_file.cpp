#include "settings_file.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

namespace {

// Python's json.dump(indent=2) step; matching it keeps the rewritten file familiar.
constexpr int INDENT_WIDTH = 2;

void appendValue(QByteArray &out, const QJsonValue &value, int depth);

QByteArray indentation(int depth) {
    return QByteArray(depth * INDENT_WIDTH, ' ');
}

// QJsonDocument only serializes containers; wrapping a scalar in an array reuses Qt's escaping
// and number formatting instead of re-implementing them.
QByteArray scalarText(const QJsonValue &value) {
    const QByteArray wrapped = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return wrapped.mid(1, wrapped.size() - 2);
}

void appendObject(QByteArray &out, const QJsonObject &object, const QStringList &keys, int depth) {
    if (keys.isEmpty()) {
        out += "{}";
        return;
    }
    out += "{\n";
    for (qsizetype i = 0; i < keys.size(); ++i) {
        out += indentation(depth + 1) + scalarText(QJsonValue(keys[i])) + ": ";
        appendValue(out, object.value(keys[i]), depth + 1);
        out += i + 1 < keys.size() ? ",\n" : "\n";
    }
    out += indentation(depth) + '}';
}

void appendArray(QByteArray &out, const QJsonArray &array, int depth) {
    if (array.isEmpty()) {
        out += "[]";
        return;
    }
    out += "[\n";
    for (qsizetype i = 0; i < array.size(); ++i) {
        out += indentation(depth + 1);
        appendValue(out, array[i], depth + 1);
        out += i + 1 < array.size() ? ",\n" : "\n";
    }
    out += indentation(depth) + ']';
}

void appendValue(QByteArray &out, const QJsonValue &value, int depth) {
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        appendObject(out, object, object.keys(), depth);
    } else if (value.isArray()) {
        appendArray(out, value.toArray(), depth);
    } else {
        out += scalarText(value);
    }
}

QString translated(const char *text) {
    return QCoreApplication::translate("SettingsFile", text);
}

} // namespace

Result<QJsonObject> SettingsFile::read(const QString &filePath) {
    QFile file(filePath);
    if (!file.exists()) {
        return Result<QJsonObject>::success(QJsonObject());
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<QJsonObject>::failure(
            translated("Could not read %1. Defaults are in use until it is readable.").arg(QDir::toNativeSeparators(filePath)));
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return Result<QJsonObject>::failure(
            translated("%1 is not valid JSON. Defaults are in use; the next change overwrites it.")
                .arg(QDir::toNativeSeparators(filePath)));
    }
    return Result<QJsonObject>::success(document.object());
}

Result<void> SettingsFile::write(const QString &filePath, const QJsonObject &document, const QStringList &leadingKeys) {
    QSaveFile file(filePath);
    const bool isWritten = file.open(QIODevice::WriteOnly) && file.write(serialize(document, leadingKeys)) >= 0;
    if (!isWritten || !file.commit()) {
        return Result<void>::failure(
            translated("Could not save settings to %1. Check that the folder is writable.").arg(QDir::toNativeSeparators(filePath)));
    }
    return Result<void>::success();
}

QByteArray SettingsFile::serialize(const QJsonObject &document, const QStringList &leadingKeys) {
    QStringList keys;
    for (const QString &key : leadingKeys) {
        if (document.contains(key)) {
            keys.append(key);
        }
    }
    for (const QString &key : document.keys()) {
        if (!keys.contains(key)) {
            keys.append(key);
        }
    }
    QByteArray out;
    appendObject(out, document, keys, 0);
    out += '\n';
    return out;
}
