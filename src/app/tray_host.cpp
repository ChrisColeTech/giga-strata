#include "tray_host.h"

#include <QAction>
#include <QEvent>
#include <QJSEngine>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QTimer>
#include <QWindow>

namespace {

// Sizes Windows asks for across tray, taskbar and alt-tab at 100-300% scaling.
constexpr int ICON_SIZES[] = {16, 20, 24, 32, 40, 48, 64};
// The dot covers the lower right 45% so it stays readable at 16 px; a cleared ring around it
// separates it from the bands without introducing a color of its own.
constexpr double DOT_FRACTION = 0.45;
constexpr double RING_FRACTION = 0.12;

QPixmap paintedDot(const QIcon &baseIcon, int size, const QColor &dotColor) {
    QPixmap pixmap = baseIcon.pixmap(size, size);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const double dotSize = size * DOT_FRACTION;
    const double ring = size * RING_FRACTION;
    const QRectF dot(size - dotSize, size - dotSize, dotSize, dotSize);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawEllipse(dot.adjusted(-ring, -ring, ring, ring));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setBrush(dotColor);
    painter.drawEllipse(dot);
    return pixmap;
}

} // namespace

TrayHost::TrayHost(const QIcon &baseIcon, QObject *parent)
    : QObject(parent), m_baseIcon(baseIcon), m_statusText(tr("Stopped")) {
    QAction *showAction = m_menu.addAction(tr("Show"));
    m_stopAction = m_menu.addAction(tr("Stop server"));
    m_menu.addSeparator();
    QAction *quitAction = m_menu.addAction(tr("Quit"));
    connect(showAction, &QAction::triggered, this, &TrayHost::showWindow);
    connect(m_stopAction, &QAction::triggered, this, &TrayHost::stopRequested);
    connect(quitAction, &QAction::triggered, this, &TrayHost::quitRequested);
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showWindow();
        }
    });
    m_tray.setContextMenu(&m_menu);
    refresh();
    m_tray.show();
}

void TrayHost::provide(TrayHost *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

TrayHost *TrayHost::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "TrayHost::create", "main.cpp must call TrayHost::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QIcon TrayHost::appIcon() {
    QSvgRenderer renderer(QStringLiteral(":/giga-strata/app-icon.svg"));
    QIcon icon;
    for (const int size : ICON_SIZES) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

QIcon TrayHost::iconWithDot(const QIcon &baseIcon, const QColor &dotColor) {
    QIcon icon;
    for (const int size : ICON_SIZES) {
        icon.addPixmap(paintedDot(baseIcon, size, dotColor));
    }
    return icon;
}

void TrayHost::setServing(bool isServing) {
    if (m_isServing == isServing) {
        return;
    }
    m_isServing = isServing;
    refresh();
    emit stateChanged();
}

void TrayHost::setFailed(bool hasFailed) {
    if (m_hasFailed == hasFailed) {
        return;
    }
    m_hasFailed = hasFailed;
    refresh();
    emit stateChanged();
}

void TrayHost::setStatusText(const QString &statusText) {
    if (m_statusText == statusText) {
        return;
    }
    m_statusText = statusText;
    refresh();
    emit stateChanged();
}

void TrayHost::setDotColors(const QColor &servingColor, const QColor &failedColor) {
    m_servingColor = servingColor;
    m_failedColor = failedColor;
    refresh();
}

void TrayHost::attachWindow(QWindow *window) {
    m_window = window;
    window->installEventFilter(this);
}

void TrayHost::showWindow() {
    if (!m_window) {
        return;
    }
    if (m_wasMaximized) {
        m_window->showMaximized();
    } else {
        m_window->showNormal();
    }
    m_window->raise();
    m_window->requestActivate();
}

bool TrayHost::eventFilter(QObject *watched, QEvent *event) {
    if (watched != m_window) {
        return QObject::eventFilter(watched, event);
    }
    if (event->type() == QEvent::Close && shouldHideOnClose()) {
        event->ignore();
        m_wasMaximized = m_window->windowStates() & Qt::WindowMaximized;
        m_window->hide();
        return true;
    }
    // Hiding inside the state-change event fights the minimize animation; defer it one turn.
    if (event->type() == QEvent::WindowStateChange && shouldHideOnMinimize()
        && (m_window->windowStates() & Qt::WindowMinimized)) {
        m_wasMaximized = m_window->windowStates() & Qt::WindowMaximized;
        QTimer::singleShot(0, m_window, [window = m_window] { window->hide(); });
    }
    return QObject::eventFilter(watched, event);
}

void TrayHost::setHidePreferences(bool isKeepingServingInTray, bool isMinimizingToTray) {
    m_isKeepingServingInTray = isKeepingServingInTray;
    m_isMinimizingToTray = isMinimizingToTray;
}

void TrayHost::refresh() {
    m_stopAction->setEnabled(m_isServing);
    m_tray.setToolTip(tr("Giga Strata: %1").arg(m_statusText));
    if (m_hasFailed && m_failedColor.isValid()) {
        m_tray.setIcon(iconWithDot(m_baseIcon, m_failedColor));
    } else if (m_isServing && m_servingColor.isValid()) {
        m_tray.setIcon(iconWithDot(m_baseIcon, m_servingColor));
    } else {
        m_tray.setIcon(m_baseIcon);
    }
}
