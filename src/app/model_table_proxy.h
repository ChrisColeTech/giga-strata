#pragma once

#include <QHash>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QVariant>

// Header sorting and spreadsheet-style filters of the Models table (04 section 2.1): a text
// match on the name, ticked values on the other columns. Filters are kept per role number.
class ModelTableProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    // Text (QString) for the name role, a QStringList of shown values for the others; an empty
    // value clears that role's filter.
    void setRoleFilter(int role, const QVariant &value);
    void clearRoleFilters();
    QList<int> filteredRoles() const { return m_filters.keys(); }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QHash<int, QVariant> m_filters;
};
