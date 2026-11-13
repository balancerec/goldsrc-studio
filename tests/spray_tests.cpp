#include "core/Spray.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <iostream>
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);QTemporaryDir tmp;if(!tmp.isValid())return 1;
 QImage image(256,256,QImage::Format_RGBA8888);image.fill(Qt::transparent);image.setPixelColor(128,128,Qt::red);
 auto spray=goldsrc::prepareSpray(image);if(!spray.ok()||spray.bytes.size()>20480||spray.preview.pixelColor(0,0).alpha()!=0)return 2;
 auto small=goldsrc::prepareSpray(image,true);if(!small.ok()||small.preview.width()>64||small.preview.height()>64)return 3;
 QString error;auto path=tmp.filePath("tempdecal.wad");if(!goldsrc::saveSpray(path,spray.bytes,&error))return 4;
 auto loaded=goldsrc::readWad3(path);if(!loaded.ok()||loaded.textures[0].type!=0x40)return 5;
 auto corrupt=spray.bytes;for(int i=0;i<4;++i)corrupt[40+i]=char(255);
 if(goldsrc::validateSpray(corrupt))return 6;
 QFile invalid(tmp.filePath("bad.wad"));if(!invalid.open(QIODevice::WriteOnly))return 7;invalid.write(corrupt);invalid.close();
 if(goldsrc::readWad3(invalid.fileName()).ok())return 8;
 if(goldsrc::saveSpray(path,corrupt))return 9;
 QFile still(path);if(!still.open(QIODevice::ReadOnly)||still.readAll()!=spray.bytes)return 10;
 QString game=tmp.filePath("cstrike");QDir().mkpath(game);QFile marker(game+"/liblist.gam");if(!marker.open(QIODevice::WriteOnly))return 11;marker.close();
 QString backup;if(!goldsrc::installSpray(game,spray.bytes,&backup,&error)){std::cerr<<error.toStdString();return 12;}
 if(!goldsrc::installSpray(game,small.bytes,&backup,&error))return 13;
 QFile old(backup+"/tempdecal.wad");if(!old.open(QIODevice::ReadOnly)||old.readAll()!=spray.bytes)return 14;
 if(!QFile::exists(backup+"/manifest.json"))return 15;
 if(!goldsrc::installSpray(game,spray.bytes,&backup,&error))return 16;
 if(goldsrc::installSpray(tmp.path(),spray.bytes,&backup,&error))return 17;
 QImage wide(1000,20,QImage::Format_RGBA8888);wide.fill(Qt::green);auto banner=goldsrc::prepareSpray(wide);if(!banner.ok()||banner.preview.width()>256)return 18;
 std::cout<<"Spray: budget, alpha, decal import, invalid mipmap, backup and restore OK\n";return 0;
}
