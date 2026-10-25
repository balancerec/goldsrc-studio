#pragma once
#include "Wad3.h"
namespace goldsrc {
inline constexpr int SprayMaxBytes=20480;
struct Spray { QByteArray bytes; QImage preview; QString error; bool ok() const { return error.isEmpty()&&!bytes.isEmpty(); } };
Spray prepareSpray(const QImage& source, bool conservative=false);
bool validateSpray(const QByteArray& bytes, QString* error=nullptr);
bool saveSpray(const QString& path,const QByteArray& bytes,QString* error=nullptr);
bool installSpray(const QString& cstrike,const QByteArray& bytes,QString* backup,QString* error=nullptr);
bool gameRunning(const QString& cstrike);
}
