#pragma once

#include <QString>
#include <QUrl>

#include <functional>

// The two things the app hands to Windows: start a detached program with a raw command line
// (explorer /select, needs it unquoted by Qt) and open a file with its default program. Injected
// so controller tests record the calls instead of opening windows.
struct ShellActions {
    std::function<bool(const QString &program, const QString &nativeArguments)> startDetached;
    std::function<bool(const QUrl &url)> openUrl;

    static ShellActions system();
};
