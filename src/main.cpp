#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SubStudio");
    MainWindow w;
    w.setWindowTitle("SubStudio");
    w.show();
    return app.exec();
}
