// Portage C++ de main.py.
#include "gui/mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ChallengeDXCC");
    QCoreApplication::setApplicationName("QtCpp");

    // Racine de l'application : dossier contenant l'exécutable (resources/,
    // config/, profiles/, gui/, runtime/ y sont copiés par CMake).
    QDir appRoot = QDir(QCoreApplication::applicationDirPath());

    MainWindow window(appRoot);
    window.show();

    return app.exec();
}
