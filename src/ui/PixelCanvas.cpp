#include "PixelCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSizePolicy>
#include <algorithm>

PixelCanvas::PixelCanvas(QWidget* parent):QWidget(parent) {
 setMouseTracking(true);
 setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 setCursor(Qt::CrossCursor);
}
void PixelCanvas::updateCanvasSize() {
 if(image_.isNull()) {resize(320,320);update();return;}
 resize(image_.width()*zoom_,image_.height()*zoom_);
 updateGeometry();update();
}
void PixelCanvas::setImage(const QImage& input) {
 image_=input.convertToFormat(QImage::Format_RGBA8888);
 drawing_=false;updateCanvasSize();
}
void PixelCanvas::setZoom(int zoom) {
 zoom_=std::clamp(zoom,1,32);
 updateCanvasSize();
}
void PixelCanvas::paintEvent(QPaintEvent*) {
 QPainter p(this);
 p.fillRect(rect(),QColor("#242a32"));
 if(image_.isNull())return;
 const int cell=16*zoom_;
 for(int y=0;y<height();y+=cell)
 for(int x=0;x<width();x+=cell)
 if(((x/cell)+(y/cell))%2==0)p.fillRect(x,y,cell,cell,QColor("#303843"));
 p.setRenderHint(QPainter::SmoothPixmapTransform,false);
 p.drawImage(rect(),image_);
 if(grid_ && zoom_>=6) {
  p.setPen(QPen(QColor(255,255,255,35),0));
  for(int x=0;x<=image_.width();++x)p.drawLine(x*zoom_,0,x*zoom_,height());
  for(int y=0;y<=image_.height();++y)p.drawLine(0,y*zoom_,width(),y*zoom_);
 }
}
void PixelCanvas::apply(const QPoint& position) {
 if(image_.isNull()||zoom_<=0)return;
 const QPoint cell(position.x()/zoom_,position.y()/zoom_);
 if(!image_.rect().contains(cell))return;
 if(tool_==Tool::Eyedropper){
  const QColor picked=image_.pixelColor(cell);
  color_=picked;
  if(colorPicked)colorPicked(picked);
  return;
 }
 const QColor next=tool_==Tool::Eraser?QColor(0,0,0,0):color_;
 if(image_.pixelColor(cell)==next)return;
 image_.setPixelColor(cell,next);update(QRect(cell.x()*zoom_,cell.y()*zoom_,zoom_,zoom_));
 if(imageChanged)imageChanged(image_);
}
void PixelCanvas::mousePressEvent(QMouseEvent* event) {
 if(event->button()!=Qt::LeftButton)return;
 drawing_=true;apply(event->position().toPoint());
}
void PixelCanvas::mouseMoveEvent(QMouseEvent* event) {
 if(drawing_ && (event->buttons() & Qt::LeftButton))apply(event->position().toPoint());
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent* event) {
 if(event->button()==Qt::LeftButton)drawing_=false;
}
