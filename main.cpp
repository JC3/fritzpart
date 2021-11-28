#include "mainwindow.h"
#include <QApplication>

int main (int argc, char *argv[]) {

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setOrganizationName("dev");
    QApplication::setApplicationName("fritzpart");

    QApplication a(argc, argv);
    MainWindow w;

    if (argc > 1)
        w.loadFile(argv[1]);

    w.show();

    return a.exec();

}
