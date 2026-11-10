#include "launch/line_splitter.h"

QStringList LineSplitter::feed(const QByteArray &chunk) {
    QStringList lines;
    for (const char byte : chunk) {
        if (m_isAfterCarriageReturn) {
            m_isAfterCarriageReturn = false;
            if (byte == '\n') {
                continue;
            }
        }
        if (byte == '\r' || byte == '\n') {
            lines.append(QString::fromUtf8(m_pending));
            m_pending.clear();
            m_isAfterCarriageReturn = byte == '\r';
            continue;
        }
        m_pending.append(byte);
    }
    return lines;
}

QStringList LineSplitter::finish() {
    m_isAfterCarriageReturn = false;
    if (m_pending.isEmpty()) {
        return {};
    }
    const QString last = QString::fromUtf8(m_pending);
    m_pending.clear();
    return {last};
}
