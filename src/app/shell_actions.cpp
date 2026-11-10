#include "shell_actions.h"

#include <QDesktopServices>
#include <QProcess>

ShellActions ShellActions::system() {
    ShellActions actions;
    actions.startDetached = [](const QString &program, const QString &nativeArguments) {
        QProcess process;
        process.setProgram(program);
        process.setNativeArguments(nativeArguments);
        return process.startDetached();
    };
    actions.openUrl = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
    return actions;
}
