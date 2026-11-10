#include "system_info.h"

#include <QFontDatabase>
#include <QJSEngine>
#include <QWindow>

#include <windows.h>

#include <dwmapi.h>

namespace {

// Qt exposes no reduced-motion API; Windows reports it as "animate controls" being off.
double readMotionScale() {
    BOOL isAnimationEnabled = TRUE;
    if (!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &isAnimationEnabled, 0)) {
        return 1.0;
    }
    return isAnimationEnabled ? 1.0 : 0.0;
}

// Cascadia Mono ships with Windows Terminal, not with Windows; Consolas always exists.
QString pickMonoFamily() {
    const QString preferred = QStringLiteral("Cascadia Mono");
    return QFontDatabase::hasFamily(preferred) ? preferred : QStringLiteral("Consolas");
}

} // namespace

SystemInfo::SystemInfo(QObject *parent) : QObject(parent), m_motionScale(readMotionScale()), m_monoFamily(pickMonoFamily()) {}

void SystemInfo::provide(SystemInfo *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

SystemInfo *SystemInfo::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "SystemInfo::create", "main.cpp must call SystemInfo::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void SystemInfo::applyDarkTitleBar(QWindow *window) {
    constexpr DWORD USE_IMMERSIVE_DARK_MODE = 20; // DWMWA_USE_IMMERSIVE_DARK_MODE, Windows 10 2004+
    const BOOL isDark = TRUE;
    const auto handle = reinterpret_cast<HWND>(window->winId());
    DwmSetWindowAttribute(handle, USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
}
