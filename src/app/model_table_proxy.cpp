#include "model_table_proxy.h"

#include <QDateTime>

namespace {

bool isEmptyFilter(const QVariant &value) {
    if (value.typeId() == QMetaType::QStringList || value.typeId() == QMetaType::QVariantList) {
        return value.toStringList().isEmpty();
    }
    return value.toString().isEmpty();
}

} // namespace

void ModelTableProxy::setRoleFilter(int role, const QVariant &value) {
    if (isEmptyFilter(value)) {
        m_filters.remove(role);
    } else {
        m_filters.insert(role, value);
    }
    invalidateRowsFilter();
}

void ModelTableProxy::clearRoleFilters() {
    m_filters.clear();
    invalidateRowsFilter();
}

bool ModelTableProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    for (auto it = m_filters.cbegin(); it != m_filters.cend(); ++it) {
        const QString value = index.data(it.key()).toString();
        const bool isList = it.value().typeId() == QMetaType::QStringList || it.value().typeId() == QMetaType::QVariantList;
        const bool isShown = isList ? it.value().toStringList().contains(value)
                                    : value.contains(it.value().toString(), Qt::CaseInsensitive);
        if (!isShown) {
            return false;
        }
    }
    return true;
}

bool ModelTableProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    const QVariant a = left.data(sortRole());
    const QVariant b = right.data(sortRole());
    if (a.typeId() == QMetaType::QString) {
        return a.toString().compare(b.toString(), Qt::CaseInsensitive) < 0;
    }
    if (a.typeId() == QMetaType::QDateTime) {
        return a.toDateTime() < b.toDateTime();
    }
    return a.toDouble() < b.toDouble();
}
