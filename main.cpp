#include "MainWidget.h"
#include <QApplication>

#include <QFileInfo>
#include <QTextCodec>
#include <QDebug>
int main(int argc, char *argv[])
{
    QTextCodec *xcodec = QTextCodec::codecForLocale() ;
    QString exeDir = xcodec->toUnicode( QByteArray(argv[0]) ) ;
    QString BKE_CURRENT_DIR = QFileInfo(exeDir).path() ;
    QStringList  libpath;
    libpath << BKE_CURRENT_DIR+QString::fromLocal8Bit("/plugins/platforms");
    libpath << BKE_CURRENT_DIR <<BKE_CURRENT_DIR+QString::fromLocal8Bit("/plugins/imageformats");
    libpath << BKE_CURRENT_DIR+QString::fromLocal8Bit("/plugins");
    libpath << QApplication::libraryPaths();
    libpath << QApplication::libraryPaths();
    qDebug()<<libpath;
    QApplication::setLibraryPaths(libpath);
    qDebug()<<QApplication::libraryPaths();
    {
        int a,b,c,d;
        a=(b=7,c=14,d=18);
    }
    QApplication a(argc, argv);

    //===
    MainWidget w;
    w.setWindowFlags(Qt::FramelessWindowHint);    // 设置窗口标志
    w.showMaximized();

    return a.exec();
}
