#include "model_table_model.h"

int ModelTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant ModelTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const ModelEntry &entry = m_entries[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return entry.name;
    case PathRole:
        return entry.path;
    case TypeRole:
        return entry.type;
    case QuantRole:
        return entry.quant;
    case SizeBytesRole:
        return double(entry.sizeBytes);
    case ExpertsRole:
        return entry.experts;
    case FilesRole:
        return entry.files;
    case ModifiedRole:
        return entry.modified;
    case HasDraftRole:
        return entry.hasDraft;
    case HasProjectorRole:
        return entry.hasProjector;
    case HasUsageRole:
        return entry.hasUsage;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModelTableModel::roleNames() const {
    return {{NameRole, "name"},          {PathRole, "path"},         {TypeRole, "type"},
            {QuantRole, "quant"},        {SizeBytesRole, "sizeBytes"}, {ExpertsRole, "experts"},
            {FilesRole, "files"},        {ModifiedRole, "modified"}, {HasDraftRole, "hasDraft"},
            {HasProjectorRole, "hasProjector"}, {HasUsageRole, "hasUsage"}};
}

void ModelTableModel::setEntries(const QVector<ModelEntry> &entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

bool ModelTableModel::removePath(const QString &path) {
    for (int row = 0; row < m_entries.size(); ++row) {
        if (m_entries[row].path.compare(path, Qt::CaseInsensitive) == 0) {
            beginRemoveRows(QModelIndex(), row, row);
            m_entries.removeAt(row);
            endRemoveRows();
            return true;
        }
    }
    return false;
}

int ModelTableModel::roleFor(const QString &name) const {
    const QHash<int, QByteArray> roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == name.toUtf8()) {
            return it.key();
        }
    }
    return -1;
}
