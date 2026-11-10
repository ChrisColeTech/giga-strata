#pragma once

#include <QtGlobal>

// Values carried over verbatim from the Python reference (scripts/*.py). Changing one changes
// planner output, so the golden fixtures must be re-exported with it.
namespace constants {

constexpr qint64 MIB = 1024 * 1024;
constexpr qint64 GIB = 1024 * MIB;

// strata_planner.py
constexpr qint64 RAM_SPARE_BYTES = 8 * GIB;
constexpr qint64 MOE_HOT_GPU0_EXTRA_BYTES = 1792 * MIB;
constexpr qint64 MAX_DRAFT_FILE_BYTES = 8 * GIB;
constexpr qint64 DEFAULT_VOCAB_SIZE = 152064;
constexpr qint64 DEFAULT_EMBEDDING_LENGTH = 4096;

// moe_hot.py
constexpr double DEFAULT_BANDWIDTH_GIB_PER_SECOND = 29.7;
constexpr double DEFAULT_LAYER_OVERHEAD_MS = 0.29;
constexpr qint64 KNAPSACK_UNIT_BYTES = 4 * MIB;

// llama_launcher.py
constexpr int USAGE_FLUSH_TOKENS = 64;
constexpr qint64 USAGE_MIN_TOKENS = 20000;
constexpr int PROBE_TIMEOUT_SECONDS = 120;
constexpr int FLAG_PROBE_TIMEOUT_SECONDS = 60;
constexpr quint32 EXIT_CODE_DLL_NOT_FOUND = 0xC0000135;

} // namespace constants
