#pragma once
#include <QMainWindow>

class QTableView;
class QAction;
class SubtitleModel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onOpenFile();
    void onHeaderContextMenuRequested(const QPoint &pos);
    void toggleColumnVisible();

private:
    void setupActions();
    void setupUi();

    QTableView *tableView_ = nullptr;
    SubtitleModel *model_ = nullptr;
    QAction *openAction_ = nullptr;
};
