#include <QApplication>
#include "ui/MainWindow.h"
int main(int argc,char** argv){QApplication app(argc,argv);app.setApplicationName("GoldSrc Studio");app.setOrganizationName("GoldSrcStudio");MainWindow window;window.show();return app.exec();}
