#pragma once

#include <QString>

#include <optional>
#include <utility>

// R18: every fallible core function returns Result<T>; no exception crosses a layer boundary.
// The error text is already a user-quality sentence (R19).
template <typename T>
class Result {
public:
    static Result success(T value) { return Result(std::move(value), QString()); }
    static Result failure(QString error) { return Result(std::nullopt, std::move(error)); }

    bool ok() const { return m_value.has_value(); }
    const T &value() const { return *m_value; }
    T &value() { return *m_value; }
    const QString &error() const { return m_error; }

private:
    Result(std::optional<T> value, QString error) : m_value(std::move(value)), m_error(std::move(error)) {}

    std::optional<T> m_value;
    QString m_error;
};

// For operations that produce nothing but can fail.
template <>
class Result<void> {
public:
    static Result success() { return Result(true, QString()); }
    static Result failure(QString error) { return Result(false, std::move(error)); }

    bool ok() const { return m_isOk; }
    const QString &error() const { return m_error; }

private:
    Result(bool isOk, QString error) : m_isOk(isOk), m_error(std::move(error)) {}

    bool m_isOk;
    QString m_error;
};
