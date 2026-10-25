#pragma once
#include <QWidget>
#include <QImage>
#include <QColor>
#include <functional>

class PixelCanvas final : public QWidget {
public:
 enum class Tool { Pencil, Eraser, Eyedropper };
 explicit PixelCanvas(QWidget* parent=nullptr);
 void setImage(const QImage& image);
 const QImage& image() const { return image_; }
 void setTool(Tool tool) { tool_=tool; }
 Tool tool() const { return tool_; }
 void setColor(const QColor& color) { color_=color; }
 QColor color() const { return color_; }
 void setZoom(int zoom);
 int zoom() const { return zoom_; }
 void setGrid(bool enabled) { grid_=enabled; update(); }
 bool grid() const { return grid_; }
 std::function<void(const QImage&)> imageChanged;
 std::function<void(const QColor&)> colorPicked;
protected:
 void paintEvent(QPaintEvent*) override;
 void mousePressEvent(QMouseEvent*) override;
 void mouseMoveEvent(QMouseEvent*) override;
 void mouseReleaseEvent(QMouseEvent*) override;
private:
 void apply(const QPoint& position);
 void updateCanvasSize();
 QImage image_;
 QColor color_{Qt::white};
 Tool tool_{Tool::Pencil};
 int zoom_{4};
 bool grid_{true};
 bool drawing_{false};
};
