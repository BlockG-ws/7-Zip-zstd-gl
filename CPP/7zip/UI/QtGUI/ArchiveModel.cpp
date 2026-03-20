#include "ArchiveModel.h"

#include <algorithm>

ArchiveModel::ArchiveModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_entries.size();
}

int ArchiveModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColCount;
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const ArchiveEntry &entry = m_entries.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:
            return entry.name;
        case ColSize:
            return entry.isDirectory ? QVariant() : QVariant(entry.size);
        case ColCompressed:
            return entry.isDirectory ? QVariant() : QVariant(entry.compressedSize);
        case ColMethod:
            return entry.method;
        case ColModified:
            return entry.modified.isValid()
                       ? entry.modified.toString("yyyy-MM-dd HH:mm:ss")
                       : QVariant();
        case ColCRC:
            return entry.crc;
        default:
            return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case ColSize:
        case ColCompressed:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

QVariant ArchiveModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case ColName:       return tr("Name");
    case ColSize:       return tr("Size");
    case ColCompressed: return tr("Compressed");
    case ColMethod:     return tr("Method");
    case ColModified:   return tr("Modified");
    case ColCRC:        return tr("CRC");
    default:            return QVariant();
    }
}

void ArchiveModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    std::sort(m_entries.begin(), m_entries.end(),
              [column, order](const ArchiveEntry &a, const ArchiveEntry &b) {
                  bool less = false;
                  switch (column) {
                  case ColName:       less = a.name < b.name; break;
                  case ColSize:       less = a.size < b.size; break;
                  case ColCompressed: less = a.compressedSize < b.compressedSize; break;
                  case ColMethod:     less = a.method < b.method; break;
                  case ColModified:   less = a.modified < b.modified; break;
                  case ColCRC:        less = a.crc < b.crc; break;
                  default: break;
                  }
                  return order == Qt::AscendingOrder ? less : !less;
              });

    endResetModel();
}

void ArchiveModel::parseOutput(const QString &output)
{
    beginResetModel();
    m_entries.clear();

    // "7zz l -slt <archive>" emits key = value pairs separated by blank lines.
    // A dashed line ("----------") marks the start of the per-file block section.
    const QStringList lines = output.split('\n');

    bool inFileSection = false;
    ArchiveEntry current;
    bool hasCurrent = false;

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        // The dashed separator signals that file blocks follow
        if (line.startsWith("----------")) {
            inFileSection = true;
            continue;
        }

        if (!inFileSection) {
            continue;
        }

        // A blank line ends the current entry block
        if (line.isEmpty()) {
            if (hasCurrent && !current.name.isEmpty()) {
                m_entries.append(current);
            }
            current = ArchiveEntry();
            hasCurrent = false;
            continue;
        }

        const int eqPos = line.indexOf(" = ");
        if (eqPos < 0) {
            continue;
        }

        hasCurrent = true;
        const QString key   = line.left(eqPos).trimmed();
        const QString value = line.mid(eqPos + 3).trimmed();

        if (key == "Path") {
            current.name = value;
        } else if (key == "Size") {
            current.size = value.toLongLong();
        } else if (key == "Packed Size") {
            current.compressedSize = value.toLongLong();
        } else if (key == "Method") {
            current.method = value;
        } else if (key == "Modified") {
            // "7zz -slt" format: "2024-01-15 12:34:56"
            current.modified = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");
        } else if (key == "CRC") {
            current.crc = value;
        } else if (key == "Attributes") {
            current.attributes = value;
            // Directories have 'D' as the first attribute character
            current.isDirectory = value.startsWith('D');
        }
    }

    // Flush the final entry if there was no trailing blank line
    if (hasCurrent && !current.name.isEmpty()) {
        m_entries.append(current);
    }

    endResetModel();
}

void ArchiveModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}
