#pragma once

#include "launch/log_event.h"

#include <QString>

// Which LogView color a line gets (01 C3): the app's own "[PLANNER] ..." / "[LAUNCHER] ..." lines,
// and server lines by the new logger's level letter (E/W) or, for builds without one (glm5),
// by their words.
class LogLineClassifier {
public:
    static LogEvent::Kind classify(const QString &line);
};
