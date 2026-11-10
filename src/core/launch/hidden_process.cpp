#include "launch/hidden_process.h"

#include <QProcess>

#include <windows.h>

void HiddenProcess::configure(QProcess &process) {
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *arguments) {
        arguments->flags |= CREATE_NO_WINDOW;
    });
}
