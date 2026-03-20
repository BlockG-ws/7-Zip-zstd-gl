#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>
#include <QString>

struct ArchiveEntry {
    QString name;
    qint64 size = 0;
    qint64 compressedSize = 0;
    QString method;
    QDateTime modified;
    QString attributes;
    QString crc;
    bool isDirectory = false;
};

class ArchiveModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ArchiveModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void parseOutput(const QString &output);
    void clear();

    enum Column {
        ColName = 0,
        ColSize,
        ColCompressed,
        ColMethod,
        ColModified,
        ColCRC,
        ColCount
    };

private:
    QList<ArchiveEntry> m_entries;
};
