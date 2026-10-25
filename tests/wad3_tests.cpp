#include "core/Wad3.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QColor>
#include <iostream>

static bool exportRead(const QString& path,const QVector<goldsrc::Texture>& textures,goldsrc::Result& loaded){
 QString err;
 if(!goldsrc::writeWad3(path,textures,&err)){std::cerr<<err.toStdString()<<"\n";return false;}
 loaded=goldsrc::readWad3(path);
 if(!loaded.ok()){std::cerr<<loaded.error.toStdString()<<"\n";return false;}
 return true;
}
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);QTemporaryDir temp;
 if(!temp.isValid())return 1;
 QImage img(64,64,QImage::Format_RGBA8888);img.fill(QColor(100,90,80));
 QVector<goldsrc::Texture> ts{{"BRICK01",img},{"{GRADE",img}};
 goldsrc::Result result;
 const QString path=temp.filePath("roundtrip.wad");
 if(!exportRead(path,ts,result)||result.textures.size()!=2)return 2;
 if(result.textures[0].image.size()!=QSize(64,64)||result.textures[0].name!="BRICK01")return 3;
 if(goldsrc::normalizedName("{GRADE")!="{GRADE")return 4;
 if(goldsrc::normalizedName("STEEL.01")!="STEEL.01")return 5;
 const QString copied=temp.filePath("lossless.wad");
 QString err;
 if(!goldsrc::writeWad3(copied,result.textures,&err))return 6;
 QFile source(path),copy(copied);
 if(!source.open(QIODevice::ReadOnly)||!copy.open(QIODevice::ReadOnly)||source.readAll()!=copy.readAll())return 7;
 auto renamed=result.textures;renamed[0].name="BRICK_NEW";
 goldsrc::Result renamedLoaded;
 if(!exportRead(temp.filePath("renamed.wad"),renamed,renamedLoaded)||renamedLoaded.textures[0].name!="BRICK_NEW")return 8;
 auto changed=result.textures;changed[0].image.fill(QColor(220,10,10));
 goldsrc::Result edited;
 if(!exportRead(temp.filePath("edited.wad"),changed,edited)||edited.textures[0].image==result.textures[0].image)return 9;
 // 3 cores exatas devem sobreviver sem perdas na paleta adaptativa.
 QImage tri(16,16,QImage::Format_RGBA8888);
 tri.fill(QColor(234,12,70));
 for(int y=0;y<16;++y)for(int x=0;x<16;++x){
  if(x<5)tri.setPixelColor(x,y,QColor(8,211,140));
  else if(x<10)tri.setPixelColor(x,y,QColor(234,12,70));
  else tri.setPixelColor(x,y,QColor(19,40,245));
 }
 goldsrc::Result colors;
 if(!exportRead(temp.filePath("colors.wad"),{{"TRICOLOR",tri}},colors))return 10;
 for(int y=0;y<16;++y)for(int x=0;x<16;++x)
  if(colors.textures[0].image.pixelColor(x,y)!=tri.pixelColor(x,y))return 11;
 // Nome com { reserva índice 255 para pixels transparentes.
 QImage alpha(16,16,QImage::Format_RGBA8888);alpha.fill(QColor(25,175,80));
 alpha.setPixelColor(2,3,QColor(0,0,0,0));
 goldsrc::Result transparent;
 if(!exportRead(temp.filePath("alpha.wad"),{{"{ALPHA",alpha}},transparent))return 12;
 if(transparent.textures[0].image.pixelColor(2,3).alpha()!=0)return 13;
 if(transparent.textures[0].image.pixelColor(0,0).alpha()!=255)return 14;
 // Nome sem { não permite transparência na semântica WAD3.
 goldsrc::Result opaque;
 if(!exportRead(temp.filePath("opaque.wad"),{{"OPAQUE",alpha}},opaque))return 15;
 if(opaque.textures[0].image.pixelColor(2,3).alpha()!=255)return 16;
 // Rejeita nomes duplicados.
 if(goldsrc::writeWad3(temp.filePath("duplicates.wad"),{{"DUP",img},{"dup",img}},&err))return 17;
 std::cout<<"WAD3: round-trip, palette, alpha, rename and duplicate checks OK\n";
 return 0;
}
