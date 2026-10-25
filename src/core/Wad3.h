#pragma once
#include <QImage>
#include <QByteArray>
#include <QString>
#include <QVector>
namespace goldsrc {
struct Texture {
 QString name;
 QImage image;
 // Conteúdo MIPTEX e imagem de referência para reexportação sem perdas.
 QByteArray originalLump;
 QImage originalImage;
 quint8 type=0x43;
};
struct Result { QVector<Texture> textures; QString error; bool ok() const { return error.isEmpty(); } };
Result readWad3(const QString& path);
bool writeWad3(const QString& path, const QVector<Texture>& textures, QString* error = nullptr);
QString normalizedName(const QString& input);
}
