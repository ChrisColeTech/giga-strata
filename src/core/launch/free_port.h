#pragma once

#include "common/result.h"

// A TCP port on 127.0.0.1 that was free a moment ago: bind port 0, read it back, close, as
// probe_server does. QtNetwork is not linked into core, so this talks to Winsock directly.
class FreePort {
public:
    static Result<int> pick();
};
