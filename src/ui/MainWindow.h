#pragma once
#include <QMainWindow>
#include <QVector>
#include "core/Wad3.h"
class QListWidget;
class QLabel;
class QLineEdit;
class QSpinBox;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class MainWindow final : public QMainWindow {
public:
 explicit MainWindow(QWidget* parent=nullptr);
private:
 QVector<goldsrc::Texture> textures_;
 QListWidget* list_{};
 QLabel* preview_{};
 QLabel* summary_{};
 QLineEdit* name_{};
 QSpinBox* width_{};
 QSpinBox* height_{};
 QString openedPath_;
 bool updating_=false;
 void setupUi();
 void refreshList(int index=-1);
 void selectTexture(int index);
 void openWad();
 void importImages();
 void exportWad();
 void removeTexture();
 void duplicateTexture();
 void resizeTexture();
 void renameTexture();
 void setDarkTheme();
};
