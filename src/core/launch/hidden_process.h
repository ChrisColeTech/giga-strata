#pragma once

class QProcess;

// Every llama-server the engine starts runs without a console window, as Python's
// creationflags=CREATE_NO_WINDOW does (02 D6). Without it each probe flashes a console.
class HiddenProcess {
public:
    static void configure(QProcess &process);
};
