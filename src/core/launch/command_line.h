#pragma once

#include <QString>
#include <QStringList>

// Python subprocess.list2cmdline: the exact Windows command line Popen hands to CreateProcess.
// The launcher passes this string through QProcess::setNativeArguments, because QProcess's own
// quoting differs and llama-server must see the same argv the Python launcher produced.
class CommandLine {
public:
    static QString join(const QStringList &arguments);
};
