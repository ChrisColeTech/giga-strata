#include "tray_host.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QImage>
#include <QPixmap>
#include <QWindow>
#include <QtTest>

// Runs on the offscreen platform: no window flashes on screen and no tray icon is registered.
class TestTrayHost : public QObject {
    Q_OBJECT

private:
    static QIcon solidIcon() {
        QPixmap pixmap(64, 64);
        pixmap.fill(Qt::blue);
        return QIcon(pixmap);
    }

private slots:
    void appIconLoadsFromResources() {
        const QIcon icon = TrayHost::appIcon();
        QVERIFY(!icon.isNull());
        QCOMPARE(icon.pixmap(32, 32).toImage().pixelColor(16, 16).alpha(), 255);
    }

    void dotIsPaintedInLowerRightCorner() {
        const QImage image = TrayHost::iconWithDot(solidIcon(), Qt::green).pixmap(64, 64).toImage();
        QCOMPARE(image.pixelColor(58, 58).rgb(), QColor(Qt::green).rgb());
        QCOMPARE(image.pixelColor(8, 8).rgb(), QColor(Qt::blue).rgb());
    }

    void stopActionFollowsServing() {
        TrayHost tray(solidIcon());
        QAction *stopAction = tray.menu().actions().at(1);
        QVERIFY(!stopAction->isEnabled());
        tray.setServing(true);
        QVERIFY(stopAction->isEnabled());
        QVERIFY(tray.trayIcon().toolTip().startsWith(QStringLiteral("Giga Strata")));
    }

    void closeQuitsNormallyWhenNotServing() {
        TrayHost tray(solidIcon());
        QWindow window;
        tray.attachWindow(&window);
        window.show();
        QCloseEvent close;
        QCoreApplication::sendEvent(&window, &close);
        QVERIFY(close.isAccepted());
    }

    void closeHidesToTrayWhileServing() {
        TrayHost tray(solidIcon());
        QWindow window;
        tray.attachWindow(&window);
        tray.setServing(true);
        window.show();
        QVERIFY(window.isVisible());
        QCloseEvent close;
        QCoreApplication::sendEvent(&window, &close);
        QVERIFY(!close.isAccepted());
        QVERIFY(!window.isVisible());
        tray.showWindow();
        QVERIFY(window.isVisible());
    }

    void closeQuitsWhileServingWhenKeepServingIsOff() {
        TrayHost tray(solidIcon());
        QWindow window;
        tray.attachWindow(&window);
        tray.setServing(true);
        tray.setHidePreferences(false, false);
        window.show();
        QCloseEvent close;
        QCoreApplication::sendEvent(&window, &close);
        QVERIFY(close.isAccepted());
    }

    void minimizeHidesOnlyWhenPreferred() {
        TrayHost tray(solidIcon());
        QVERIFY(!tray.shouldHideOnMinimize());
        tray.setHidePreferences(true, true);
        QVERIFY(tray.shouldHideOnMinimize());
        QVERIFY(!tray.shouldHideOnClose());
        tray.setServing(true);
        QVERIFY(tray.shouldHideOnClose());
    }
};

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", GIGA_STRATA_QT_PLATFORMS);
    QApplication application(argc, argv);
    TestTrayHost test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_tray_host.moc"
