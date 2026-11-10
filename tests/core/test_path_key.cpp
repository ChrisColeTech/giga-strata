#include "common/path_key.h"

#include <QtTest>

class TestPathKey : public QObject {
    Q_OBJECT

private slots:
    void keyIsLowercaseNativeAndCollapsed() {
        QCOMPARE(path_key::of(QStringLiteral("N:/Models/GLM/../GLM/A.GGUF")), QStringLiteral("n:\\models\\glm\\a.gguf"));
    }

    void parentDirectoryMatchesPythonDirname() {
        QCOMPARE(path_key::parentDirectory(QStringLiteral("N:\\Models\\m.gguf")), QStringLiteral("N:\\Models"));
        QCOMPARE(path_key::parentDirectory(QStringLiteral("N:\\m.gguf")), QStringLiteral("N:\\"));
    }

    void joinAddsOneSeparator() {
        QCOMPARE(path_key::join(QStringLiteral("N:\\a"), QStringLiteral("models")), QStringLiteral("N:\\a\\models"));
        QCOMPARE(path_key::join(QStringLiteral("N:\\"), QStringLiteral("models")), QStringLiteral("N:\\models"));
    }
};

QTEST_APPLESS_MAIN(TestPathKey)
#include "test_path_key.moc"
