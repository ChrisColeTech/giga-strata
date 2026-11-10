#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;
class QWindow;

// OS facts the theme needs, read once at startup (01 section 6.9): reduced motion and the
// monospace family that is actually installed. Also forces the dark native title bar (N1).
class SystemInfo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(double motionScale READ motionScale CONSTANT)
    Q_PROPERTY(QString monoFamily READ monoFamily CONSTANT)

public:
    explicit SystemInfo(QObject *parent);

    static void provide(SystemInfo *instance);
    static SystemInfo *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    double motionScale() const { return m_motionScale; }
    const QString &monoFamily() const { return m_monoFamily; }

    // Following the system theme puts a white title bar over the graphite UI in light mode.
    static void applyDarkTitleBar(QWindow *window);

private:
    static inline SystemInfo *s_instance = nullptr;

    double m_motionScale;
    QString m_monoFamily;
};
