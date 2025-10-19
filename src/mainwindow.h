#pragma once
#include <QMainWindow>

class QTableView;
class QAction;
class SubtitleModel;
class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onOpenFile();
    void onHeaderContextMenuRequested(const QPoint &pos);
    void toggleColumnVisible();
    void onSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void onSave();
    void onSaveAs();
    void onEditorTextChanged();

private:
    void setupActions();
    void setupUi();

    QTableView *tableView_ = nullptr;
    SubtitleModel *model_ = nullptr;
    QAction *openAction_ = nullptr;
    QAction *saveAction_ = nullptr;
    QAction *saveAsAction_ = nullptr;
    QTextEdit *editor_ = nullptr;
    QString currentFilePath_;
};
