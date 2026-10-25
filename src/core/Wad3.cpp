#include "Wad3.h"
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QSet>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
namespace goldsrc {
namespace {
quint16 u16(const QByteArray& b, qsizetype i) { const auto* p=reinterpret_cast<const uchar*>(b.constData()+i); return quint16(p[0]) | quint16(p[1])<<8; }
quint32 u32(const QByteArray& b, qsizetype i) { const auto* p=reinterpret_cast<const uchar*>(b.constData()+i); return quint32(p[0]) | quint32(p[1])<<8 | quint32(p[2])<<16 | quint32(p[3])<<24; }
bool span(qsizetype offset, quint64 n, qsizetype size) { return offset>=0 && quint64(offset)<=quint64(size) && n<=quint64(size-offset); }
void put16(QByteArray& b, quint16 v) { b.append(char(v&255)); b.append(char((v>>8)&255)); }
void put32(QByteArray& b, quint32 v) { for(int i=0;i<4;++i)b.append(char((v>>(i*8))&255)); }
void set32(QByteArray& b, qsizetype pos, quint32 v) { for(int i=0;i<4;++i)b[pos+i]=char((v>>(i*8))&255); }
QByteArray name16(const QString& s){ QByteArray n=s.toLatin1().left(15); n.append(QByteArray(16-n.size(), '\0'));return n; }
int nextMultiple16(int x){return ((std::clamp(x,1,4096)+15)/16)*16;}
}
QString normalizedName(const QString& input){
  // Nomes MIPTEX não são caminhos; preserve pontos e prefixos do GoldSrc.
  QString s=input.trimmed().toUpper();
  if(s.isEmpty())s="TEXTURA";
  QString out;
  for(const QChar c:s){
    if((c.unicode()>='A'&&c.unicode()<='Z')||
       (c.unicode()>='0'&&c.unicode()<='9')||
       c=='_'||c=='.'||c=='{'||c=='!'||c=='+'||c=='-'||c=='~')out+=c;
    else out+='_';
  }
  return out.left(15);
}
Result readWad3(const QString& path){
  QFile f(path);if(!f.open(QIODevice::ReadOnly))return {{},"Não foi possível abrir o arquivo."};
  const QByteArray b=f.readAll();const auto size=b.size();
  if(size<12 || b.left(4)!="WAD3")return {{},"Cabeçalho WAD3 inválido."};
  const quint32 count=u32(b,4), dir=u32(b,8);
  if(count>100000 || !span(dir,quint64(count)*32,size))return {{},"Diretório WAD3 corrompido."};
  Result r;
  for(quint32 i=0;i<count;++i){
    const qsizetype e=qsizetype(dir)+qsizetype(i)*32;
    const quint32 pos=u32(b,e), disk=u32(b,e+4);
    const uchar type=uchar(b[e+12]), comp=uchar(b[e+13]);
    if(type!=0x43 || comp!=0)continue; // Apenas MIPTEX não compactado
    if(!span(pos,disk,size) || disk<40)continue;
    if(!span(pos,40,size))continue;
    const quint32 w=u32(b,pos+16), h=u32(b,pos+20), off0=u32(b,pos+24),off3=u32(b,pos+36);
    if(w<1||h<1||w>4096||h>4096 || quint64(w)*h>16777216)continue;
    const quint64 p0=quint64(pos)+off0, mip3=quint64(w/8)*quint64(h/8), paletteAt=quint64(pos)+off3+mip3;
    if(off0<40 || off3<40 || off0>=disk || off3>=disk || p0+quint64(w)*h>quint64(pos)+disk || paletteAt>quint64(pos)+disk || !span(qsizetype(paletteAt),2,size))continue;
    const quint16 colors=u16(b,qsizetype(paletteAt));
    if(colors<256 || paletteAt+2+quint64(colors)*3>quint64(pos)+disk)continue;
    const QByteArray rawName=b.mid(e+16,16);QString name=QString::fromLatin1(rawName.constData(),int(rawName.indexOf('\0')<0?16:rawName.indexOf('\0')));
    const bool transparent=name.startsWith('{');
    QImage img(int(w),int(h),QImage::Format_RGBA8888);
    for(quint32 y=0;y<h;++y){auto* dst=img.scanLine(int(y));for(quint32 x=0;x<w;++x){
      const auto index=uchar(b[qsizetype(p0)+qsizetype(y)*w+x]);
      const qsizetype cp=qsizetype(paletteAt+2)+qsizetype(index)*3;
      dst[x*4]=uchar(b[cp]);dst[x*4+1]=uchar(b[cp+1]);dst[x*4+2]=uchar(b[cp+2]);dst[x*4+3]=(transparent&&index==255)?0:255;
    }}
    Texture tex{name,img};
    tex.originalImage=img.copy();
    tex.originalLump=b.mid(pos,disk);
    r.textures.push_back(std::move(tex));
  }
  if(r.textures.isEmpty())r.error="Nenhuma textura MIPTEX compatível encontrada.";
  return r;
}
bool writeWad3(const QString& path,const QVector<Texture>& textures,QString* error){
  auto fail=[&](const QString& message){if(error)*error=message;return false;};
  if(textures.isEmpty())return fail("Não há texturas para exportar.");
  if(textures.size()>100000)return fail("Quantidade de texturas excedida.");
  QByteArray out("WAD3",4);put32(out,quint32(textures.size()));put32(out,0);
  struct Entry{quint32 pos,size;QByteArray name;};QVector<Entry> entries; QSet<QString> names;
  for(const auto& t:textures){
    if(t.image.isNull())return fail("Imagem inválida.");
    QString name=normalizedName(t.name);
    if(names.contains(name))return fail("Nome duplicado: "+name);
    names.insert(name);
    // Reutiliza dados indexados, quatro mipmaps e paleta originais se os pixels não mudaram.
    // Renomear uma textura não altera sua paleta nem os pixels.
    if(!t.originalLump.isEmpty() && !t.originalImage.isNull() && t.image==t.originalImage){
      QByteArray lump=t.originalLump;
      if(lump.size()<40)return fail("MIPTEX original inválido.");
      lump.replace(0,16,name16(name));
      if(quint64(out.size())+quint64(lump.size())>std::numeric_limits<quint32>::max())return fail("Arquivo excede 4 GiB.");
      entries.push_back({quint32(out.size()),quint32(lump.size()),name16(name)});
      out+=lump;
      continue;
    }
    const int w=nextMultiple16(t.image.width()),h=nextMultiple16(t.image.height());
    const QImage img=t.image.scaled(w,h,Qt::IgnoreAspectRatio,Qt::FastTransformation).convertToFormat(QImage::Format_RGBA8888);
    // Paleta adaptativa: preserva cores exatas quando há até 255 cores.
    // Em imagens complexas, seleciona as cores mais frequentes e aproxima
    // as demais pela menor distância RGB. Índice 255 reservado para alpha.
    QHash<QRgb,int> frequencies;
    for(int y=0;y<img.height();++y)for(int x=0;x<img.width();++x){
      const QColor c=img.pixelColor(x,y);
      if(name.startsWith('{')&&c.alpha()<128)continue;
      ++frequencies[qRgb(c.red(),c.green(),c.blue())];
    }
    QVector<QPair<QRgb,int>> ranked;
    ranked.reserve(frequencies.size());
    for(auto it=frequencies.cbegin();it!=frequencies.cend();++it)
      ranked.push_back({it.key(),it.value()});
    std::sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){
      return a.second!=b.second?a.second>b.second:a.first<b.first;
    });
    QVector<QRgb> colors;
    colors.reserve(255);
    for(int i=0;i<std::min(255,int(ranked.size()));++i)colors.push_back(ranked[i].first);
    if(colors.isEmpty())colors.push_back(qRgb(0,0,0));
    QByteArray palette(256*3,'\0');
    for(int i=0;i<colors.size();++i){
      palette[i*3]=char(qRed(colors[i]));
      palette[i*3+1]=char(qGreen(colors[i]));
      palette[i*3+2]=char(qBlue(colors[i]));
    }
    // Transparência GoldSrc: índice 255, tipicamente azul puro.
    palette[255*3]=0;palette[255*3+1]=0;palette[255*3+2]=char(255);
    QHash<QRgb,int> colorIndices;
    for(int i=0;i<colors.size();++i)colorIndices.insert(colors[i],i);
    auto closest=[&](QRgb color)->int{
      auto existing=colorIndices.constFind(color);
      if(existing!=colorIndices.cend())return existing.value();
      int best=0;int bestDistance=std::numeric_limits<int>::max();
      for(int i=0;i<colors.size();++i){
        const int dr=qRed(color)-qRed(colors[i]),dg=qGreen(color)-qGreen(colors[i]),db=qBlue(color)-qBlue(colors[i]);
        const int d=dr*dr+dg*dg+db*db;
        if(d<bestDistance){bestDistance=d;best=i;if(d==0)break;}
      }
      colorIndices.insert(color,best);
      return best;
    };
    QVector<QByteArray> levels;
    for(int level=0;level<4;++level){
      const int lw=w>>level, lh=h>>level; QImage m=img.scaled(lw,lh,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
      QByteArray pixels;pixels.reserve(lw*lh);
      for(int y=0;y<lh;++y){const uchar* row=m.constScanLine(y);for(int x=0;x<lw;++x){const uchar* p=row+x*4;
        const bool alpha=name.startsWith('{')&&p[3]<128;
        const int idx=alpha?255:closest(qRgb(p[0],p[1],p[2]));
        pixels.append(char(idx));
      }}levels.push_back(pixels);
    }
    QByteArray lump=name16(name);put32(lump,w);put32(lump,h);
    quint32 offset=40;for(const auto& lev:levels){put32(lump,offset);offset+=quint32(lev.size());}
    for(const auto& lev:levels)lump+=lev;
    put16(lump,256);lump+=palette;
    if(quint64(out.size())+quint64(lump.size())>std::numeric_limits<quint32>::max())return fail("Arquivo excede 4 GiB.");
    entries.push_back({quint32(out.size()),quint32(lump.size()),name16(name)});out+=lump;
  }
  if(quint64(out.size())+quint64(entries.size())*32>std::numeric_limits<quint32>::max())return fail("Diretório muito grande.");
  set32(out,8,quint32(out.size()));
  for(const auto& e:entries){put32(out,e.pos);put32(out,e.size);put32(out,e.size);out.append(char(0x43));out.append(char(0));out.append(2,'\0');out+=e.name;}
  QFile file(path);if(!file.open(QIODevice::WriteOnly))return fail("Não foi possível gravar arquivo.");
  if(file.write(out)!=out.size())return fail("Falha ao escrever WAD3.");return true;
}
}
