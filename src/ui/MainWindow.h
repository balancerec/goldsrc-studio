#pragma once
#include <QMainWindow>
#include <QPoint>
#include <QPointer>
#include <QVector>
#include "core/Wad3.h"
class QListWidget;
class QLabel;
class PixelCanvas;
class QComboBox;
class QCheckBox;
class QPushButton;
class QStackedWidget;
class QLineEdit;
class QSpinBox;
class QScrollArea;
class QScrollBar;
class QEvent;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class MainWindow final : public QMainWindow {
public:
 explicit MainWindow(QWidget* parent=nullptr);
protected:
 bool eventFilter(QObject* watched,QEvent* event) override;
 void dragEnterEvent(QDragEnterEvent* event) override;
 void dropEvent(QDropEvent* event) override;
private:
 QVector<goldsrc::Texture> textures_;
 QListWidget* list_{};
 PixelCanvas* canvas_{};
 QStackedWidget* stack_{};
 QPushButton* colorButton_{};
 QLabel* summary_{};
 QLineEdit* name_{};
 QSpinBox* width_{};
 QSpinBox* height_{};
 QString openedPath_;
 bool updating_=false;
 QScrollArea* scrollArea_{};
 QSpinBox* zoomControl_{};
 bool panning_=false;
 QPoint panAnchor_;
 int panX_=0,panY_=0;
 void importPaths(const QStringList& paths);
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
