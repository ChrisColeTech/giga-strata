#pragma once

#include <QtGlobal>

#include <optional>

// System RAM from GlobalMemoryStatusEx, the fallback strata_planner.free_ram_bytes uses.
// nullopt when Windows does not answer, so the planner can treat RAM as unknown.
class RamInfo {
public:
    static std::optional<qint64> freeBytes();
    static std::optional<qint64> totalBytes();
};
