#include "hardware/ram_info.h"

#include <windows.h>

namespace {

std::optional<MEMORYSTATUSEX> memoryStatus() {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        return std::nullopt;
    }
    return status;
}

} // namespace

std::optional<qint64> RamInfo::freeBytes() {
    const std::optional<MEMORYSTATUSEX> status = memoryStatus();
    return status ? std::optional<qint64>(static_cast<qint64>(status->ullAvailPhys)) : std::nullopt;
}

std::optional<qint64> RamInfo::totalBytes() {
    const std::optional<MEMORYSTATUSEX> status = memoryStatus();
    return status ? std::optional<qint64>(static_cast<qint64>(status->ullTotalPhys)) : std::nullopt;
}
