#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// What Help > About shows about the bundled llama-server (04 section 7.1): the fork's commit and
// branch from <app dir>\server\BUILD_INFO.txt, read once at startup. Empty when the file is
// missing (a build without the fork).
class AboutInfo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString serverCommit READ serverCommit CONSTANT)
    Q_PROPERTY(QString serverBranch READ serverBranch CONSTANT)

public:
    AboutInfo(const QString &buildInfoPath, QObject *parent = nullptr);

    static void provide(AboutInfo *instance);
    static AboutInfo *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    const QString &serverCommit() const { return m_serverCommit; }
    const QString &serverBranch() const { return m_serverBranch; }

private:
    static inline AboutInfo *s_instance = nullptr;

    QString m_serverCommit;
    QString m_serverBranch;
};
