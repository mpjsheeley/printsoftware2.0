#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{

    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            printf("Communicate with the Image Portreit's server and send the downloaded\n");
            printf("pictures to a GDI printer.\n\n");
            printf("Usage: MemoryPortreita\n");
            return 0;
        }
        if (!strcmp(argv[i], "--version")) {
            printf("2.0\n"); // The 1st one was in Java
            return 0;
        }
    }

    QCoreApplication::setOrganizationName("Memory Portraits");
    QCoreApplication::setOrganizationDomain("memoryportraits.com");
    QCoreApplication::setApplicationName("Memory Portraits Printing");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
