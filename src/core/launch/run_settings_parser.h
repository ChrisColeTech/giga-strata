#pragma once

#include "common/result.h"
#include "launch/run_settings.h"

#include <QVariantMap>

// llama_launcher.read_settings: the saved values (strings for text fields, bools for toggles,
// under the Python keys) turned into RunSettings, with the launcher's two error sentences.
class RunSettingsParser {
public:
    static Result<RunSettings> parse(const QVariantMap &values);

    static QString missingModelMessage();
    static QString notWholeNumbersMessage();
};
