#include "common/paths.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TestPaths : public QObject {
    Q_OBJECT

private slots:
    void layoutMatchesPythonLauncher() {
        const Paths paths(QStringLiteral("D:/Projects/giga-strata"));
        QCOMPARE(paths.projectRoot(), QStringLiteral("D:\\Projects\\giga-strata"));
        QCOMPARE(paths.settingsFile(), QStringLiteral("D:\\Projects\\giga-strata\\launcher_settings.json"));
        QCOMPARE(paths.usageDirectory(), QStringLiteral("D:\\Projects\\giga-strata\\moe_usage"));
        QCOMPARE(paths.hotDirectory(), QStringLiteral("D:\\Projects\\giga-strata\\moe_usage\\hot"));
        QCOMPARE(paths.modelsDirectory(), QStringLiteral("D:\\Projects\\giga-strata\\models"));
    }

    void findsRootAboveBuildFolder() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QDir(root.path()).mkpath(QStringLiteral("src"));
        QDir(root.path()).mkpath(QStringLiteral("build/release/bin"));
        QFile marker(root.filePath(QStringLiteral("src/main.cpp")));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.close();

        const QString found = Paths::findProjectRoot(root.filePath(QStringLiteral("build/release/bin")));
        QCOMPARE(QDir(found).canonicalPath(), QDir(root.path()).canonicalPath());
    }

    void fallsBackToStartWithoutMarker() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QCOMPARE(QDir(Paths::findProjectRoot(root.path())).canonicalPath(), QDir(root.path()).canonicalPath());
    }
};

QTEST_APPLESS_MAIN(TestPaths)
#include "test_paths.moc"
