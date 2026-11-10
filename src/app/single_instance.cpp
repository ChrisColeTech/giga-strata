#include "single_instance.h"

#include <QCryptographicHash>
#include <QDir>
#include <QLocalSocket>

#include <utility>

#include <windows.h>

namespace {

const QByteArray ACTIVATE_MESSAGE = QByteArrayLiteral("activate\n");
constexpr int KEY_HASH_LENGTH = 12;

} // namespace

SingleInstance::SingleInstance(QString key, QObject *parent) : QObject(parent), m_key(std::move(key)) {
    connect(&m_server, &QLocalServer::newConnection, this, &SingleInstance::acceptConnections);
}

QString SingleInstance::keyForInstallation(const QString &applicationDirectory) {
    // Windows paths are case-insensitive; the same folder must always give the same key.
    const QByteArray folder = QDir::cleanPath(applicationDirectory).toLower().toUtf8();
    const QByteArray hash = QCryptographicHash::hash(folder, QCryptographicHash::Sha1).toHex().left(KEY_HASH_LENGTH);
    return QStringLiteral("giga-strata-") + QString::fromLatin1(hash);
}

bool SingleInstance::claim() {
    if (signalRunningInstance()) {
        return false;
    }
    // A crashed first instance can leave the name registered; clearing it lets us listen.
    QLocalServer::removeServer(m_key);
    m_server.listen(m_key);
    return true;
}

bool SingleInstance::signalRunningInstance() {
    QLocalSocket socket;
    socket.connectToServer(m_key);
    if (!socket.waitForConnected(CONNECT_TIMEOUT_MS)) {
        return false;
    }
    // This process was just started by the user, so it may hand the foreground to the running
    // instance; without that, Windows only flashes its taskbar button.
    AllowSetForegroundWindow(ASFW_ANY);
    socket.write(ACTIVATE_MESSAGE);
    socket.waitForBytesWritten(CONNECT_TIMEOUT_MS);
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::acceptConnections() {
    while (QLocalSocket *socket = m_server.nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        emit activationRequested();
    }
}
