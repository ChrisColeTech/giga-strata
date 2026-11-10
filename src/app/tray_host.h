#pragma once

#include <QColor>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QSystemTrayIcon>
#include <QtQml/qqmlregistration.h>

class QAction;
class QJSEngine;
class QQmlEngine;
class QWindow;

// FR-6 tray icon and lifecycle. Closing the window while a server runs hides it to the tray (the
// server keeps running) when the keep-serving-in-tray preference is on (04 section 3), otherwise
// close quits as usual; minimizing hides it to the tray when the minimize-to-tray preference is on. The tray menu
// offers Show, Stop server (only while serving) and Quit; N4 dots mark serving and failed.
// The launch controller drives isServing/hasFailed/statusText; until it exists they stay at
// their stopped defaults, so nothing pretends a server is running.
class TrayHost : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool isServing READ isServing WRITE setServing NOTIFY stateChanged)
    Q_PROPERTY(bool hasFailed READ hasFailed WRITE setFailed NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText WRITE setStatusText NOTIFY stateChanged)

public:
    explicit TrayHost(const QIcon &baseIcon, QObject *parent = nullptr);

    static void provide(TrayHost *instance);
    static TrayHost *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // assets/app-icon.svg rendered at every icon size (window, taskbar and tray).
    static QIcon appIcon();
    // The base icon with a dot of `dotColor` in its lower right corner, at every icon size.
    static QIcon iconWithDot(const QIcon &baseIcon, const QColor &dotColor);

    bool isServing() const { return m_isServing; }
    bool hasFailed() const { return m_hasFailed; }
    const QString &statusText() const { return m_statusText; }
    bool shouldHideOnClose() const { return m_isServing && m_isKeepingServingInTray; }
    bool shouldHideOnMinimize() const { return m_isMinimizingToTray; }
    const QMenu &menu() const { return m_menu; }
    const QSystemTrayIcon &trayIcon() const { return m_tray; }

    void setServing(bool isServing);
    void setFailed(bool hasFailed);
    void setStatusText(const QString &statusText);
    // Semantic colors come from Theme (the only owner of color literals), handed over by main.cpp.
    void setDotColors(const QColor &servingColor, const QColor &failedColor);
    void attachWindow(QWindow *window);
    // The Settings > General > Window preferences (SettingsStore keepServingInTray, minimizeToTray).
    void setHidePreferences(bool isKeepingServingInTray, bool isMinimizingToTray);

public slots:
    void showWindow();

signals:
    void stateChanged();
    void stopRequested();
    void quitRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refresh();

    static inline TrayHost *s_instance = nullptr;

    QIcon m_baseIcon;
    QSystemTrayIcon m_tray;
    QMenu m_menu;
    QAction *m_stopAction = nullptr;
    QPointer<QWindow> m_window;
    QColor m_servingColor;
    QColor m_failedColor;
    QString m_statusText;
    bool m_isServing = false;
    bool m_hasFailed = false;
    bool m_isKeepingServingInTray = true;
    bool m_isMinimizingToTray = false;
    // Restoring from the tray brings the window back the way it was hidden, maximized or not.
    bool m_wasMaximized = false;
};
