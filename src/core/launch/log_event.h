#pragma once

#include <QString>

// One thing LogParser read in llama-server output. Every complete line produces a Line event
// (for the LogView, colored by kind per 01 C3); recognized lines are followed by a typed event.
struct LogEvent {
    enum class Type {
        Line,            // text = the line, kind set, elapsedMs from the new logger's prefix or -1
        LoadingStarted,  // text = model path from "load_model: loading model '...'"
        LoadProgress,    // progressPercent: glm5 progress dots so far; 100 when the dot line ends
        Listening,       // text = URL, e.g. "http://127.0.0.1:8080"
        RequestTiming,   // isPrompt, taskId, milliseconds, tokens, tokensPerSecond(+Text) as printed
        LoadError,       // text = reason (first of the duplicated lines, gguf_init reason preferred)
        StartupError,    // LM Studio's LMSTUDIO_STARTUP_ERROR json: code, text = message
        ArgumentError,   // text = the server's complaint about its command line
    };
    enum class Kind { Server, Planner, Launcher, Warning, Error };

    Type type = Type::Line;
    QString text;
    Kind kind = Kind::Server;
    qint64 elapsedMs = -1;
    int progressPercent = -1;
    bool isPrompt = false;
    int taskId = -1;
    double milliseconds = 0.0;
    qint64 tokens = 0;
    double tokensPerSecond = 0.0;
    QString tokensPerSecondText;  // the readout shows the server's own figure, never a recomputed one
    QString code;
};
