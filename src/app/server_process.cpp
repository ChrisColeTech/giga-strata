#include "server_process.h"

#include "launch/command_line.h"
#include "launch/hidden_process.h"

#include <QCoreApplication>
#include <QDir>

ServerProcess::ServerProcess(QObject *parent) : QObject(parent) {
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    HiddenProcess::configure(m_process);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &ServerProcess::readOutput);
    connect(&m_process, &QProcess::finished, this, &ServerProcess::onFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &ServerProcess::onError);
}

ServerProcess::~ServerProcess() {
    if (isRunning()) {
        m_process.disconnect(this);
        m_process.kill();
        m_process.waitForFinished();
    }
}

void ServerProcess::start(const AssembledLaunch &launch) {
    m_parser = LogParser();
    m_process.setProgram(launch.argv.value(0));
    m_process.setNativeArguments(CommandLine::join(launch.argv.mid(1)));
    m_process.setProcessEnvironment(launch.environment);
    m_process.start();
}

void ServerProcess::kill() {
    if (isRunning()) {
        m_process.kill();
    }
}

void ServerProcess::readOutput() {
    for (const LogEvent &event : m_parser.feed(m_process.readAllStandardOutput())) {
        emit eventRead(event);
    }
}

void ServerProcess::onFinished(int exitCode) {
    readOutput();
    for (const LogEvent &event : m_parser.finish()) {
        emit eventRead(event);
    }
    emit exited(quint32(exitCode));
}

void ServerProcess::onError(QProcess::ProcessError error) {
    if (error != QProcess::FailedToStart) {
        return;  // crashes and kills arrive through finished() with their exit code
    }
    emit failedToStart(QCoreApplication::translate("ServerProcess", "Could not start %1: %2")
                           .arg(QDir::toNativeSeparators(m_process.program()), m_process.errorString()));
}
