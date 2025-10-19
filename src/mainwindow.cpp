#include "mainwindow.h"
#include "subtitlemodel.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QFileDialog>
#include <QTableView>
#include <QHeaderView>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupActions();
    setupUi();
}

void MainWindow::setupActions() {
    openAction_ = new QAction(tr("Open..."), this);
    openAction_->setShortcut(QKeySequence::Open);
    connect(openAction_, &QAction::triggered, this, &MainWindow::onOpenFile);

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(openAction_);
    fileMenu->addAction(tr("Exit"), this, &QMainWindow::close);
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("SubStudio");

    // toolbar
    QToolBar *tb = addToolBar(tr("Main"));
    tb->addAction(openAction_);

    // model + view
    model_ = new SubtitleModel(this);
    tableView_ = new QTableView(this);
    tableView_->setModel(model_);

    // hide the default vertical row header (we use our own '#' column)
    tableView_->verticalHeader()->setVisible(false);

    // column resize policy: make 'Text' stretch, others resize to contents
    for (int c = 0; c < model_->columnCount(); ++c) {
        if (c == SubtitleModel::Text) {
            tableView_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        } else {
            tableView_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        }
    }

    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setAlternatingRowColors(true);
    tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // enable custom context menu on header
    tableView_->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView_->horizontalHeader(), &QHeaderView::customContextMenuRequested,
            this, &MainWindow::onHeaderContextMenuRequested);

    setCentralWidget(tableView_);

    QStatusBar *s = statusBar();
    s->showMessage(tr("Ready"));
}

void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Open subtitle"), QString(),
                                                      tr("SubRip files (*.srt *.str);;All files (*.*)"));
    if (path.isEmpty()) return;
    bool ok = model_->loadSrt(path);
    if (!ok) {
        statusBar()->showMessage(tr("Failed to load file"));
    } else {
        statusBar()->showMessage(tr("Loaded: %1").arg(path));
        tableView_->resizeColumnsToContents();
    }
}

void MainWindow::onHeaderContextMenuRequested(const QPoint &pos) {
    QHeaderView *h = tableView_->horizontalHeader();
    QMenu menu(this);

    // Build list of column names in sync with model
    const QStringList fullLabels = {
        "#",
        "Start Time",
        "End Time",
        "Characters Per Second",
        "Text"
    };

    for (int c = 0; c < model_->columnCount(); ++c) {
        QAction *act = new QAction(fullLabels.value(c, model_->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString()), &menu);
        act->setCheckable(true);
        act->setChecked(!tableView_->isColumnHidden(c));

        // Protect the Text column: never allow hiding it
        if (c == SubtitleModel::Text) {
            act->setChecked(true);
            act->setEnabled(false); // no puede desmarcarse
        } else {
            act->setData(c); // store column index only for non-protected ones
            connect(act, &QAction::triggered, this, &MainWindow::toggleColumnVisible);
        }
        menu.addAction(act);
    }

    menu.exec(h->mapToGlobal(pos));
}

void MainWindow::toggleColumnVisible() {
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act) return;
    int col = act->data().toInt();
    bool visible = act->isChecked();
    tableView_->setColumnHidden(col, !visible);
}
