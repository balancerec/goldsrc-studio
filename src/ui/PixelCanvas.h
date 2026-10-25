#pragma once
#include <QWidget>
#include <QImage>
#include <QColor>
#include <QVector>
#include <functional>

class PixelCanvas final : public QWidget {
public:
 enum class Tool { Pencil, Eraser, Eyedropper, Fill };
 explicit PixelCanvas(QWidget* parent=nullptr);
 void setImage(const QImage& image);
 const QImage& image() const {return image_;}
 void setTool(Tool tool) {tool_=tool;}
 void setColor(const QColor& color) {color_=color;}
 QColor color() const {return color_;}
 void setZoom(int zoom);
 int zoom() const {return zoom_;}
 void setGrid(bool visible) {grid_=visible;update();}
 bool canUndo() const {return !undo_.isEmpty();}
 bool canRedo() const {return !redo_.isEmpty();}
 void undo();
 void redo();
 std::function<void(const QImage&)> imageChanged;
 std::function<void(const QColor&)> colorPicked;
 std::function<void(bool,bool)> historyChanged;
protected:
 void paintEvent(QPaintEvent*) override;
 void mousePressEvent(QMouseEvent*) override;
 void mouseMoveEvent(QMouseEvent*) override;
 void mouseReleaseEvent(QMouseEvent*) override;
private:
 void updateCanvasSize();
 void beginChange();
 void endChange();
 void apply(const QPoint&);
 void floodFill(const QPoint&);
 void notifyHistory();
 QImage image_;
 QColor color_{Qt::white};
 Tool tool_{Tool::Pencil};
 int zoom_{4};
 bool grid_{true};
 bool drawing_{false};
 bool changed_{false};
 QImage before_;
 QVector<QImage> undo_;
 QVector<QImage> redo_;
 static constexpr int HistoryLimit=15;
};
