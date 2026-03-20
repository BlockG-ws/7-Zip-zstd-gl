#include "MainWindow.h"
#include "ArchiveModel.h"

#include <QTableView>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QIcon>
#include <QProcess>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_model(new ArchiveModel(this))
{
    setWindowTitle("7-Zip ZS");
    resize(900, 600);
    setAcceptDrops(true);

    m_sevenZipBin = findSevenZip();

    setupUI();
    setupMenus();
    setupToolBar();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(m_tableView, &QTableView::doubleClicked,
            this, &MainWindow::onTableDoubleClicked);

    setCentralWidget(m_tableView);

    m_statusLabel = new QLabel(tr("Ready — open an archive to begin"), this);
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Open…"), this, &MainWindow::onOpen, QKeySequence::Open);
    fileMenu->addAction(tr("&Create Archive…"), this, &MainWindow::onCreateArchive,
                        QKeySequence(tr("Ctrl+N")));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close, QKeySequence::Quit);

    QMenu *actionsMenu = menuBar()->addMenu(tr("&Actions"));
    actionsMenu->addAction(tr("&Extract All…"), this, &MainWindow::onExtractAll,
                           QKeySequence(tr("Ctrl+E")));
    actionsMenu->addAction(tr("Extract &Selected…"), this, &MainWindow::onExtractSelected,
                           QKeySequence(tr("Ctrl+Shift+E")));
    actionsMenu->addSeparator();
    actionsMenu->addAction(tr("&Add Files…"), this, &MainWindow::onAddFiles,
                           QKeySequence(tr("Ctrl+A")));
    actionsMenu->addSeparator();
    actionsMenu->addAction(tr("&Test Archive"), this, &MainWindow::onTest,
                           QKeySequence(tr("Ctrl+T")));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setMovable(false);

    toolBar->addAction(QIcon::fromTheme("document-open"), tr("Open"),
                       this, &MainWindow::onOpen);
    toolBar->addSeparator();
    toolBar->addAction(QIcon::fromTheme("archive-extract"), tr("Extract All"),
                       this, &MainWindow::onExtractAll);
    toolBar->addAction(QIcon::fromTheme("list-add"), tr("Add Files"),
                       this, &MainWindow::onAddFiles);
    toolBar->addAction(QIcon::fromTheme("document-new"), tr("Create Archive"),
                       this, &MainWindow::onCreateArchive);
    toolBar->addSeparator();
    toolBar->addAction(QIcon::fromTheme("dialog-apply"), tr("Test"),
                       this, &MainWindow::onTest);
}

QString MainWindow::findSevenZip() const
{
    // Prefer a 7zz binary next to the application itself
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/7zz",
        appDir + "/7z",
        "/usr/local/bin/7zz",
        "/usr/bin/7zz",
        "/usr/local/bin/7z",
        "/usr/bin/7z",
    };

    for (const QString &candidate : candidates) {
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }

    // Fall back to whatever is on PATH
    return "7zz";
}

void MainWindow::openArchive(const QString &path)
{
    m_archivePath = path;
    setWindowTitle(QString("7-Zip ZS — %1").arg(QFileInfo(path).fileName()));
    refreshArchive();
}

void MainWindow::refreshArchive()
{
    if (m_archivePath.isEmpty()) {
        return;
    }

    m_statusLabel->setText(tr("Loading archive…"));
    QApplication::processEvents();

    QProcess proc;
    proc.start(m_sevenZipBin, {"l", "-slt", m_archivePath});
    if (!proc.waitForFinished(30000)) {
        m_statusLabel->setText(tr("Error: 7zz timed out or could not be started"));
        return;
    }

    if (proc.exitCode() != 0) {
        const QString err = proc.readAllStandardError().trimmed();
        m_statusLabel->setText(
            tr("Error listing archive: %1").arg(err.isEmpty() ? proc.errorString() : err));
        return;
    }

    m_model->parseOutput(proc.readAllStandardOutput());
    m_tableView->resizeColumnsToContents();
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    const int n = m_model->rowCount();
    m_statusLabel->setText(tr("%n item(s)", nullptr, n));
}

void MainWindow::onOpen()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open Archive"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        tr("Archives (*.7z *.zip *.tar *.tar.gz *.tgz *.tar.bz2 *.tar.xz "
           "*.rar *.gz *.bz2 *.xz *.zst *.lz4 *.lzma *.cab *.iso *.wim);;"
           "All Files (*)"));

    if (!path.isEmpty()) {
        openArchive(path);
    }
}

void MainWindow::onExtractAll()
{
    if (m_archivePath.isEmpty()) {
        QMessageBox::information(this, tr("No Archive Open"),
                                 tr("Please open an archive first."));
        return;
    }

    const QString outDir = QFileDialog::getExistingDirectory(
        this,
        tr("Extract To Directory"),
        QFileInfo(m_archivePath).absolutePath());

    if (!outDir.isEmpty()) {
        runSevenZip({"x", m_archivePath, "-o" + outDir, "-y"},
                    tr("Extracting archive…"));
    }
}

void MainWindow::onExtractSelected()
{
    if (m_archivePath.isEmpty()) {
        QMessageBox::information(this, tr("No Archive Open"),
                                 tr("Please open an archive first."));
        return;
    }

    const QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Nothing Selected"),
                                 tr("Please select one or more files to extract."));
        return;
    }

    const QString outDir = QFileDialog::getExistingDirectory(
        this,
        tr("Extract To Directory"),
        QFileInfo(m_archivePath).absolutePath());

    if (outDir.isEmpty()) {
        return;
    }

    QStringList args = {"x", m_archivePath, "-o" + outDir, "-y"};
    for (const QModelIndex &idx : selected) {
        args << m_model->data(m_model->index(idx.row(), 0)).toString();
    }

    runSevenZip(args, tr("Extracting selected files…"));
}

void MainWindow::onAddFiles()
{
    if (m_archivePath.isEmpty()) {
        QMessageBox::information(this, tr("No Archive Open"),
                                 tr("Please open or create an archive first."));
        return;
    }

    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Add Files to Archive"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    if (!files.isEmpty()) {
        QStringList args = {"a", m_archivePath};
        args += files;
        runSevenZip(args, tr("Adding files…"));
    }
}

void MainWindow::onCreateArchive()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select Files to Compress"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    if (files.isEmpty()) {
        return;
    }

    const QString archivePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Archive As"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        tr("7-Zip Archive (*.7z);;"
           "ZIP Archive (*.zip);;"
           "TAR Archive (*.tar);;"
           "All Files (*)"));

    if (!archivePath.isEmpty()) {
        QStringList args = {"a", archivePath};
        args += files;
        if (runSevenZip(args, tr("Creating archive…"))) {
            openArchive(archivePath);
        }
    }
}

void MainWindow::onTest()
{
    if (m_archivePath.isEmpty()) {
        QMessageBox::information(this, tr("No Archive Open"),
                                 tr("Please open an archive first."));
        return;
    }

    QProcess proc;
    proc.start(m_sevenZipBin, {"t", m_archivePath});
    QProgressDialog progress(tr("Testing archive integrity…"), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    while (proc.state() != QProcess::NotRunning) {
        QApplication::processEvents();
        proc.waitForFinished(100);
        if (progress.wasCanceled()) {
            proc.kill();
            break;
        }
    }
    progress.hide();

    const QString out = proc.readAllStandardOutput();
    if (proc.exitCode() == 0) {
        QMessageBox::information(this, tr("Test Passed"),
                                 tr("Archive integrity test passed.\n\n%1").arg(out.trimmed()));
    } else {
        QMessageBox::critical(this, tr("Test Failed"),
                              tr("Archive integrity test failed.\n\n%1")
                                  .arg(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed()));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(
        this,
        tr("About 7-Zip ZS"),
        tr("<h2>7-Zip ZS</h2>"
           "<p>A graphical archive manager for Linux with Zstandard support.</p>"
           "<p>Based on <b>7-Zip</b> by Igor Pavlov and the "
           "<b>Zstandard</b> codec by Meta.</p>"
           "<p><a href=\"https://github.com/BlockG-ws/7-Zip-zstd-gl\">"
           "https://github.com/BlockG-ws/7-Zip-zstd-gl</a></p>"));
}

void MainWindow::onTableDoubleClicked(const QModelIndex &index)
{
    // Reserved for future navigation into sub-directories inside the archive.
    Q_UNUSED(index);
}

bool MainWindow::runSevenZip(const QStringList &args, const QString &description)
{
    QProcess proc;
    proc.setProgram(m_sevenZipBin);
    proc.setArguments(args);

    QProgressDialog progress(description, tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    proc.start();
    while (proc.state() != QProcess::NotRunning) {
        QApplication::processEvents();
        proc.waitForFinished(100);
        if (progress.wasCanceled()) {
            proc.kill();
            m_statusLabel->setText(tr("Cancelled"));
            return false;
        }
    }
    progress.hide();

    if (proc.exitCode() != 0) {
        const QString err = proc.readAllStandardError().trimmed();
        QMessageBox::critical(
            this,
            tr("Operation Failed"),
            tr("The operation failed:\n\n%1")
                .arg(err.isEmpty() ? proc.errorString() : err));
        m_statusLabel->setText(tr("Error"));
        return false;
    }

    m_statusLabel->setText(tr("Done"));
    refreshArchive();
    return true;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        openArchive(urls.first().toLocalFile());
    }
}
