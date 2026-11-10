#pragma once

#include <QLocalServer>
#include <QObject>
#include <QString>

// FR-6.3: the first process of an installation listens on a local socket; a later launch of the
// same installation connects, asks it to come to the front and exits. Keyed per install folder,
// so a second build of the app elsewhere (another checkout) still runs side by side.
class SingleInstance : public QObject {
    Q_OBJECT

public:
    static constexpr int CONNECT_TIMEOUT_MS = 500;

    explicit SingleInstance(QString key, QObject *parent = nullptr);

    static QString keyForInstallation(const QString &applicationDirectory);

    // True when this process is the first one and now listens; false after the running instance
    // has been asked to show itself, in which case the caller exits.
    bool claim();

signals:
    void activationRequested();

private:
    bool signalRunningInstance();
    void acceptConnections();

    QString m_key;
    QLocalServer m_server;
};
