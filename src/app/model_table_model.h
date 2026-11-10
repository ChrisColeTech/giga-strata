#pragma once

#include "model_entry.h"

#include <QAbstractListModel>
#include <QVector>

// The Models table rows (05 section 1); ModelTableProxy sorts and filters it for the view.
class ModelTableModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        PathRole,
        TypeRole,
        QuantRole,
        SizeBytesRole,
        ExpertsRole,
        FilesRole,
        ModifiedRole,
        HasDraftRole,
        HasProjectorRole,
        HasUsageRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<ModelEntry> &entries);
    // Removes the row of `path` (first shard); false when no row has it.
    bool removePath(const QString &path);
    const QVector<ModelEntry> &entries() const { return m_entries; }

    // The role number for a role name ("sizeBytes"), -1 when unknown.
    int roleFor(const QString &name) const;

private:
    QVector<ModelEntry> m_entries;
};
