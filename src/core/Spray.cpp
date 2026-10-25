#include "Spray.h"
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QPainter>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QDateTime>
#include <algorithm>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif
namespace goldsrc {
static bool fail(QString* error,const QString& message){if(error)*error=message;return false;}
static quint32 get32(const QByteArray& b,int at){quint32 v=0;for(int i=0;i<4;++i)v|=quint32(uchar(b[at+i]))<<(8*i);return v;}
bool validateSpray(const QByteArray& b,QString* error){
 if(b.size()<44 || b.size()>SprayMaxBytes || b.left(4)!="WAD3" || get32(b,4)!=1)
  return fail(error,"Spray exige um WAD3 com uma textura e até 20.480 bytes.");
 quint32 dir=get32(b,8);
 if(quint64(dir)+32!=quint64(b.size()))return fail(error,"Diretório de spray inválido.");
 quint32 pos=get32(b,dir),size=get32(b,dir+4);
 if(pos<12 || quint64(pos)+size>dir || size<810 || get32(b,dir+8)!=size || uchar(b[dir+13])!=0 || (uchar(b[dir+12])!=0x40 && uchar(b[dir+12])!=0x43))return fail(error,"Lump de spray inválido.");
 quint32 w=get32(b,pos+16),h=get32(b,pos+20);
 if(!w||!h||w>256||h>256||w%16||h%16)return fail(error,"Dimensões de spray inválidas.");
 quint32 end=40;
 for(int i=0;i<4;++i){if(get32(b,pos+24+4*i)!=end)return fail(error,"Mipmap inválido.");end+=(w>>i)*(h>>i);}
 if(end+770>size || uchar(b[pos+end])!=0 || uchar(b[pos+end+1])!=1)return fail(error,"Paleta inválida.");
 return true;
}
Spray prepareSpray(const QImage& source,bool conservative){
 Spray result;if(source.isNull()){result.error="Selecione uma imagem.";return result;}
 // Largest aspect-preserving fit inside an aligned canvas and the wire budget.
 QSize best(16,16);double bestScale=0;int bestArea=65537;
 const int max=conservative?64:256;
 for(int w=16;w<=max;w+=16)for(int h=16;h<=max;h+=16){
  int area=w*h;if(854+area+area/4+area/16+area/64>SprayMaxBytes)continue;
  double scale=std::min(double(w)/source.width(),double(h)/source.height());
  if(scale>bestScale || (scale==bestScale && area<bestArea)){best={w,h};bestScale=scale;bestArea=area;}
 }
 QImage canvas(best,QImage::Format_RGBA8888);canvas.fill(Qt::transparent);
 auto scaled=source.scaled(best,Qt::KeepAspectRatio,Qt::SmoothTransformation);
 {QPainter p(&canvas);p.drawImage((best.width()-scaled.width())/2,(best.height()-scaled.height())/2,scaled);}
 QTemporaryDir tmp;if(!tmp.isValid()){result.error="Não foi possível criar prévia temporária.";return result;}
 Texture texture{"LOGO",canvas};texture.type=0x40;
 const auto path=tmp.filePath("tempdecal.wad");
 if(!writeWad3(path,{texture},&result.error))return result;
 QFile file(path);if(!file.open(QIODevice::ReadOnly)){result.error=file.errorString();return result;}result.bytes=file.readAll();
 if(!validateSpray(result.bytes,&result.error)){result.bytes.clear();return result;}
 auto loaded=readWad3(path);if(!loaded.ok()){result.error=loaded.error;result.bytes.clear();return result;}
 result.preview=loaded.textures[0].image;return result;
}
bool saveSpray(const QString& path,const QByteArray& bytes,QString* error){
 if(!validateSpray(bytes,error))return false;
 QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())return fail(error,"Não foi possível salvar. Verifique permissões e somente leitura.");return true;
}
bool gameRunning(const QString& cstrike){
#ifdef Q_OS_WIN
 const auto root=QFileInfo(cstrike).dir().canonicalPath();
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)return true;
 PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);bool found=false;
 if(Process32FirstW(snap,&entry))do{
  const auto name=QString::fromWCharArray(entry.szExeFile).toLower();
  if(name!="hl.exe"&&name!="cs.exe"&&name!="cstrike.exe")continue;
  HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);
  if(!process){found=true;break;}
  wchar_t path[32768];DWORD length=32768;
  if(!QueryFullProcessImageNameW(process,0,path,&length))found=true;
  else found=QFileInfo(QString::fromWCharArray(path,length)).dir().canonicalPath().compare(root,Qt::CaseInsensitive)==0;
  CloseHandle(process);if(found)break;
 }while(Process32NextW(snap,&entry));CloseHandle(snap);return found;
#else
 Q_UNUSED(cstrike);return false;
#endif
}
bool installSpray(const QString& directory,const QByteArray& bytes,QString* backup,QString* error){
 if(!validateSpray(bytes,error))return false;
 QDir game(directory);
 if(!game.exists("liblist.gam")||QFileInfo(game.absolutePath()).fileName().compare("cstrike",Qt::CaseInsensitive)!=0)return fail(error,"Selecione a pasta cstrike que contém liblist.gam.");
 if(gameRunning(directory))return fail(error,"Feche o CS antes de instalar o spray.");
 const QString target=game.filePath("tempdecal.wad");
 if(QFileInfo(target).isSymLink())return fail(error,"O destino é um link; selecione uma instalação regular.");
 const QString folder=game.filePath("spray-backups/"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
 if(!QDir().mkpath(folder))return fail(error,"Não foi possível criar backup.");
 bool hadOriginal=QFile::exists(target);QByteArray previous;
 if(hadOriginal){QFile old(target);if(!old.open(QIODevice::ReadOnly))return fail(error,old.errorString());previous=old.readAll();if(!QFile::copy(target,folder+"/tempdecal.wad"))return fail(error,"Backup falhou; instalação cancelada.");}
 auto hash=[](const QByteArray& data){return QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex());};
 QJsonObject metadata{{"target",target},{"previous_sha256",hash(previous)},{"new_sha256",hash(bytes)},{"had_original",hadOriginal}};
 QSaveFile manifest(folder+"/manifest.json");auto json=QJsonDocument(metadata).toJson();
 if(!manifest.open(QIODevice::WriteOnly)||manifest.write(json)!=json.size()||!manifest.commit())return fail(error,"Não foi possível registrar backup.");
 if(!saveSpray(target,bytes,error))return false;
 QFile installed(target);bool verified=installed.open(QIODevice::ReadOnly)&&installed.readAll()==bytes;installed.close();
 if(!verified){
  bool restored=false;
  if(hadOriginal){QSaveFile rollback(target);restored=rollback.open(QIODevice::WriteOnly)&&rollback.write(previous)==previous.size()&&rollback.commit();}
  else restored=QFile::remove(target);
  return fail(error,restored?"Verificação falhou; arquivo anterior restaurado.":"Verificação e restauração falharam. Backup: "+folder);
 }
 if(backup)*backup=folder;return true;
}
}
