#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TRYX Panorama Manager");
    app.setOrganizationName("DXVSI");

    MainWindow window;
    window.show();

    return app.exec();
}
