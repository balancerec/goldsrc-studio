#include "core/Wad3.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);QTemporaryDir temp; if(!temp.isValid())return 1;
 QImage img(64,64,QImage::Format_RGBA8888);img.fill(QColor(100,90,80));
 QVector<goldsrc::Texture> ts{{"BRICK01",img},{"{GRADE",img}};QString err;
 const QString path=temp.filePath("roundtrip.wad");if(!goldsrc::writeWad3(path,ts,&err)){std::cerr<<err.toStdString();return 2;}
 QFile raw(path);if(!raw.open(QIODevice::ReadOnly)||raw.read(4)!="WAD3")return 3;
 auto result=goldsrc::readWad3(path);if(!result.ok()||result.textures.size()!=2){std::cerr<<result.error.toStdString();return 4;}
 if(result.textures[0].image.size()!=QSize(64,64)||result.textures[0].name!="BRICK01")return 5;
 if(goldsrc::normalizedName("{grade.png")!="{GRADE")return 6;
 std::cout<<"WAD3 round-trip OK\n";return 0;
}
