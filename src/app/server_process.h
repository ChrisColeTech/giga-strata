#pragma once

#include "launch/launch_assembler.h"
#include "launch/log_event.h"
#include "launch/log_parser.h"

#include <QObject>
#include <QProcess>

// One running llama-server: started hidden (02 D6) with the exact Python command line
// (CommandLine::join through setNativeArguments), its merged output parsed as it arrives, and
// killed on stop: a console llama-server ignores the WM_CLOSE that terminate() posts.
class ServerProcess : public QObject {
    Q_OBJECT

public:
    explicit ServerProcess(QObject *parent = nullptr);
    ~ServerProcess() override;

    void start(const AssembledLaunch &launch);
    void kill();
    bool isRunning() const { return m_process.state() != QProcess::NotRunning; }
    const LogSummary &summary() const { return m_parser.summary(); }

signals:
    void eventRead(const LogEvent &event);
    // exitCode as Windows reports it, so NTSTATUS codes like 0xC0000135 survive.
    void exited(quint32 exitCode);
    void failedToStart(const QString &message);

private:
    void readOutput();
    void onFinished(int exitCode);
    void onError(QProcess::ProcessError error);

    QProcess m_process;
    LogParser m_parser;
};
