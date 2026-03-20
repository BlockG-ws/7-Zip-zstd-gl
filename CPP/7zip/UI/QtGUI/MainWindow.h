#pragma once

#include <QMainWindow>
#include <QString>

class QTableView;
class QLabel;
class QToolBar;
class QDragEnterEvent;
class QDropEvent;
class ArchiveModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openArchive(const QString &path);

private slots:
    void onOpen();
    void onExtractAll();
    void onExtractSelected();
    void onAddFiles();
    void onCreateArchive();
    void onTest();
    void onAbout();
    void onTableDoubleClicked(const QModelIndex &index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    void setupMenus();
    void setupToolBar();

    QString findSevenZip() const;
    bool runSevenZip(const QStringList &args, const QString &description = QString());
    void refreshArchive();

    QString m_archivePath;
    QString m_sevenZipBin;

    QTableView *m_tableView;
    QLabel *m_statusLabel;
    ArchiveModel *m_model;
};
