#include "launch_preparer.h"

#include "server_build.h"

#include "launch/server_env.h"

#include <QHostAddress>
#include <QNetworkInterface>
#include <QProcessEnvironment>

namespace {

QString lanAddress() {
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        const auto flags = interface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning)
            || flags.testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
            const QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLinkLocal()) {
                return address.toString();
            }
        }
    }
    return QString();
}

} // namespace

Result<AssembledLaunch> LaunchPreparer::prepare(const PlanJobResult &result, const QString &binary,
                                                const ServerBuild &build, const QString &usageDirectory,
                                                const QString &hotDirectory) {
    const RunSettings &settings = *result.settings;
    const auto hotFile = LaunchAssembler::writeHotFile(result.run, hotDirectory, settings.modelPath);
    if (!hotFile.ok()) {
        return Result<AssembledLaunch>::failure(hotFile.error());
    }
    const QProcessEnvironment environment =
        ServerEnv::build(QProcessEnvironment::systemEnvironment(),
                         ServerEnv::vendorDllDirectories(build.environment(), binary), settings.gpuIds);
    return LaunchAssembler::assemble(settings, result.run, binary, result.flags, environment, usageDirectory);
}

QString LaunchPreparer::clientUrl(bool isOnLan, const QString &port) {
    const QString address = isOnLan ? lanAddress() : QString();
    return QStringLiteral("http://%1:%2").arg(address.isEmpty() ? QStringLiteral("127.0.0.1") : address, port);
}
