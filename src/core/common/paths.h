#pragma once

#include <QString>

// Every on-disk location the engine uses, derived once from the project root at startup and
// read-only afterwards (R24). Layout matches llama_launcher.py so existing data keeps working.
class Paths {
public:
    explicit Paths(const QString &projectRoot);

    // Walks up from startDirectory to the first folder holding src/main.cpp (a checkout);
    // falls back to startDirectory when none is found.
    static QString findProjectRoot(const QString &startDirectory);

    const QString &projectRoot() const { return m_projectRoot; }
    QString settingsFile() const;
    QString usageDirectory() const;
    QString hotDirectory() const;
    QString modelsDirectory() const;
    QString thirdPartyDirectory() const;

private:
    QString m_projectRoot;
};
