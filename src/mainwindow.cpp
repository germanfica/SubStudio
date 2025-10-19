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
#include <QVBoxLayout>
#include <QWidget>
#include <QTextEdit>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>

class CPSDelegate : public QStyledItemDelegate {
public:
    explicit CPSDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    // Blends 'fg' over 'bg' according to alpha (alpha ∈ [0,1])
    static QColor blend(const QColor &fg, const QColor &bg, double alpha) {
        alpha = std::min(std::max(alpha, 0.0), 1.0);
        int r = qRound(fg.red()   * alpha + bg.red()   * (1.0 - alpha));
        int g = qRound(fg.green() * alpha + bg.green() * (1.0 - alpha));
        int b = qRound(fg.blue()  * alpha + bg.blue()  * (1.0 - alpha));
        return QColor(r, g, b);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        if (option.state & QStyle::State_Selected) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QVariant v = index.data(Qt::DisplayRole);
        bool ok = false;
        int cps = v.toInt(&ok);
        if (!ok || cps <= 0) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QSettings s;
        int warn = s.value("Subtitle/CPSWarning", 15).toInt();
        int error = s.value("Subtitle/CPSError", 25).toInt();
        QColor errorColor = s.value("Colour/CpsError", QColor(255,0,0)).value<QColor>();

        if (error < warn) error = warn;

        if (cps <= warn) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        double alpha = (double)(cps - warn + 1) / (double)(error - warn + 1);
        alpha = std::clamp(alpha, 0.0, 1.0);

        // background base (considera alternate rows)
        QColor baseBg = option.palette.color(QPalette::Base);
        if (option.features.testFlag(QStyleOptionViewItem::Alternate))
            baseBg = option.palette.color(QPalette::AlternateBase);

        QColor blendedBg = blend(errorColor, baseBg, alpha);

        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(blendedBg);
        QRect r = option.rect;
        r.adjust(0, 1, 0, 0);
        painter->drawRect(r);

        QString text = index.data(Qt::DisplayRole).toString();
        QColor origText = option.palette.color(QPalette::Text);
        QColor textColor = blend(QColor(0,0,0), origText, alpha);

        painter->setPen(textColor);
        painter->drawText(option.rect, Qt::AlignCenter, text);
        painter->restore();
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupActions();
    setupUi();
}

void MainWindow::updateWindowTitle() {
    const QString appSuffix = QStringLiteral(" - SubStudio 0.1.0");
    QString name;
    if (!currentFilePath_.isEmpty()) {
        QFileInfo fi(currentFilePath_);
        name = fi.fileName();
    } else {
        name = QStringLiteral("Untitled");
    }
    QString prefix = dirty_ ? QStringLiteral("* ") : QString();
    setWindowTitle(prefix + name + appSuffix);
}

bool MainWindow::saveFile() {
    // si no hay path, pedimos Save As
    if (currentFilePath_.isEmpty()) {
        onSaveAs();
        // onSaveAs seteara currentFilePath_ si el usuario guardo
        if (currentFilePath_.isEmpty()) return false; // usuario canceló Save As
    }
    if (model_->saveSrt(currentFilePath_)) {
        dirty_ = false;
        statusBar()->showMessage(tr("Saved: %1").arg(currentFilePath_), 3000);
        updateWindowTitle();
        return true;
    } else {
        statusBar()->showMessage(tr("Failed to save file"), 3000);
        return false;
    }
}

void MainWindow::setupActions() {
    openAction_ = new QAction(tr("Open."), this);
    openAction_->setShortcut(QKeySequence::Open);
    connect(openAction_, &QAction::triggered, this, &MainWindow::onOpenFile);

    saveAction_ = new QAction(tr("Save"), this);
    saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSave);

    saveAsAction_ = new QAction(tr("Save As."), this);
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::onSaveAs);

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(openAction_);
    fileMenu->addAction(saveAction_);
    fileMenu->addAction(saveAsAction_);
    fileMenu->addAction(tr("Exit"), this, &QMainWindow::close);
}

void MainWindow::setupUi() {
    resize(1000, 700);
    updateWindowTitle();

    // toolbar: Open + Save
    QToolBar *tb = addToolBar(tr("Main"));
    tb->addAction(openAction_);
    tb->addAction(saveAction_);

    // central widget with vertical layout: editor (fixed height) above table
    QWidget *central = new QWidget(this);
    QVBoxLayout *vlay = new QVBoxLayout(central);
    vlay->setContentsMargins(4,4,4,4);
    vlay->setSpacing(6);

    // editor: fixed height 141 px, full width
    editor_ = new QTextEdit(central);
    editor_->setFixedHeight(141); // requested fixed height
    editor_->setAcceptRichText(false); // plain text
    vlay->addWidget(editor_);

    // connect editor changes to update model immediately
    connect(editor_, &QTextEdit::textChanged, this, &MainWindow::onEditorTextChanged);

    // table
    model_ = new SubtitleModel(this);
    tableView_ = new QTableView(central);
    tableView_->setModel(model_);
    tableView_->setItemDelegateForColumn(SubtitleModel::CPS, new CPSDelegate(this));

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

    // header context menu
    tableView_->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView_->horizontalHeader(), &QHeaderView::customContextMenuRequested,
            this, &MainWindow::onHeaderContextMenuRequested);

    vlay->addWidget(tableView_);
    setCentralWidget(central);

    // selection -> when user selects a row, show text in editor
    connect(tableView_->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onSelectionChanged);

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
        currentFilePath_ = path;
        dirty_ = false;
        statusBar()->showMessage(tr("Loaded: %1").arg(path));
        tableView_->resizeColumnsToContents();
        // clear editor
        editor_->clear();
        updateWindowTitle();
    }
}

void MainWindow::onSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/) {
    if (!current.isValid()) {
        editor_->blockSignals(true);
        editor_->clear();
        editor_->blockSignals(false);
        return;
    }
    const int row = current.row();
    const QVariant v = model_->data(model_->index(row, SubtitleModel::Text), Qt::DisplayRole);

    // Block signals so setPlainText does NOT call onEditorTextChanged
    editor_->blockSignals(true);
    editor_->setPlainText(v.toString());
    editor_->blockSignals(false);
}

void MainWindow::onSave() {
    saveFile();
}

void MainWindow::onSaveAs() {
    const QString path = QFileDialog::getSaveFileName(this, tr("Save subtitle as"), QString(),
                                                      tr("SubRip files (*.srt);;All files (*.*)"));
    if (path.isEmpty()) return;
    if (model_->saveSrt(path)) {
        currentFilePath_ = path;
        dirty_ = false;
        statusBar()->showMessage(tr("Saved: %1").arg(path));
        updateWindowTitle();
    } else {
        statusBar()->showMessage(tr("Failed to save file"));
    }
}

void MainWindow::onEditorTextChanged() {
    // get currently selected row
    const QModelIndex current = tableView_->selectionModel()->currentIndex();
    if (!current.isValid()) return;

    const int row = current.row();
    const QString newText = editor_->toPlainText();

    // update model immediately (this updates Text and CPS columns and emits dataChanged)
    model_->setTextAt(row, newText);

    // marcar como cambios sin guardar y actualizar title
    if (!dirty_) {
        dirty_ = true;
        updateWindowTitle();
    }

    // optionally show a small status message
    statusBar()->showMessage(tr("Edited row %1").arg(row + 1), 1500);
}

void MainWindow::onHeaderContextMenuRequested(const QPoint &pos) {
    QHeaderView *h = tableView_->horizontalHeader();
    QMenu menu(this);

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
            act->setEnabled(false);
        } else {
            act->setData(c);
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

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!dirty_) {
        event->accept();
        return;
    }

    // Preguntar: Yes / No / Cancel (Yes por defecto)
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Warning);
    msg.setWindowTitle(QStringLiteral("Unsaved changes"));
    const QString pathToShow = currentFilePath_.isEmpty() ? QStringLiteral("Untitled") : QDir::toNativeSeparators(currentFilePath_);
    msg.setText(tr("Do you want to save changes to %1?").arg(pathToShow));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Yes);

    int ret = msg.exec();
    if (ret == QMessageBox::Yes) {
        // intentar guardar; si el guardado falla o usuario cancela SaveAs, NO cerramos
        if (saveFile()) {
            event->accept();
        } else {
            event->ignore();
        }
    } else if (ret == QMessageBox::No) {
        event->accept();
    } else { // Cancel
        event->ignore();
    }
}
