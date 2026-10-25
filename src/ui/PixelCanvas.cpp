#include "PixelCanvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QVector>
#include <algorithm>

PixelCanvas::PixelCanvas(QWidget* parent):QWidget(parent) {
 setMouseTracking(true);setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);setCursor(Qt::CrossCursor);
}
void PixelCanvas::updateCanvasSize(){
 if(image_.isNull()){resize(320,320);update();return;}
 resize(image_.width()*zoom_,image_.height()*zoom_);
 updateGeometry();update();
}
void PixelCanvas::notifyHistory(){if(historyChanged)historyChanged(canUndo(),canRedo());}
void PixelCanvas::setImage(const QImage& image){
 image_=image.isNull()?QImage():image.convertToFormat(QImage::Format_RGBA8888);
 drawing_=false;changed_=false;before_=QImage();undo_.clear();redo_.clear();
 updateCanvasSize();notifyHistory();
}
void PixelCanvas::replaceImage(const QImage& image){
 if(image.isNull() || drawing_ || image==image_)return;
 undo_.push_back(image_.copy());
 if(undo_.size()>HistoryLimit)undo_.removeFirst();
 redo_.clear();image_=image.convertToFormat(QImage::Format_RGBA8888);
 updateCanvasSize();notifyHistory();
 if(imageChanged)imageChanged(image_);
}
void PixelCanvas::setZoom(int zoom){zoom_=std::clamp(zoom,1,32);updateCanvasSize();}
void PixelCanvas::paintEvent(QPaintEvent*){
 QPainter p(this);p.fillRect(rect(),QColor("#1b1922"));
 if(image_.isNull())return;
 const int cell=16*zoom_;
 for(int y=0;y<height();y+=cell)for(int x=0;x<width();x+=cell)
  if(((x/cell)+(y/cell))%2==0)p.fillRect(x,y,cell,cell,QColor("#292632"));
 p.setRenderHint(QPainter::SmoothPixmapTransform,false);
 p.drawImage(rect(),image_);
 if(grid_ && zoom_>=6){
  p.setPen(QPen(QColor(214,198,237,42),0));
  for(int x=0;x<=image_.width();++x)p.drawLine(x*zoom_,0,x*zoom_,height());
  for(int y=0;y<=image_.height();++y)p.drawLine(0,y*zoom_,width(),y*zoom_);
 }
}
void PixelCanvas::beginChange(){
 if(image_.isNull())return;
 before_=image_.copy();changed_=false;
}
void PixelCanvas::endChange(){
 if(changed_ && !before_.isNull()){
  undo_.push_back(before_);
  if(undo_.size()>HistoryLimit)undo_.removeFirst();
  redo_.clear();notifyHistory();
 }
 before_=QImage();changed_=false;
}
void PixelCanvas::undo(){
 if(!canUndo()||drawing_)return;
 redo_.push_back(image_.copy());image_=undo_.takeLast();
 update();notifyHistory();if(imageChanged)imageChanged(image_);
}
void PixelCanvas::redo(){
 if(!canRedo()||drawing_)return;
 undo_.push_back(image_.copy());image_=redo_.takeLast();
 update();notifyHistory();if(imageChanged)imageChanged(image_);
}
void PixelCanvas::floodFill(const QPoint& start){
 if(!image_.rect().contains(start))return;
 const QRgb old=image_.pixel(start);
 const QRgb replacement=color_.rgba();
 if(old==replacement)return;
 // Varredura iterativa: evita estouro de pilha recursiva.
 QVector<QPoint> queue;queue.reserve(1024);queue.push_back(start);
 image_.setPixel(start.x(),start.y(),replacement);
 qsizetype head=0;
 const int w=image_.width(),h=image_.height();
 while(head<queue.size()){
  const QPoint p=queue.at(head++);
  const int x=p.x(),y=p.y();
  auto visit=[&](int xx,int yy){
   if(xx>=0 && yy>=0 && xx<w && yy<h && image_.pixel(xx,yy)==old){
    image_.setPixel(xx,yy,replacement);queue.push_back(QPoint(xx,yy));
   }
  };
  visit(x-1,y);visit(x+1,y);visit(x,y-1);visit(x,y+1);
 }
 changed_=true;update();if(imageChanged)imageChanged(image_);
}
void PixelCanvas::apply(const QPoint& position){
 if(image_.isNull())return;
 const QPoint at(position.x()/zoom_,position.y()/zoom_);
 if(!image_.rect().contains(at))return;
 if(tool_==Tool::Eyedropper){
  color_=image_.pixelColor(at);if(colorPicked)colorPicked(color_);return;
 }
 if(tool_==Tool::Fill){floodFill(at);return;}
 const QColor paint=tool_==Tool::Eraser?QColor(0,0,0,0):color_;
 if(image_.pixelColor(at)==paint)return;
 image_.setPixelColor(at,paint);changed_=true;
 update(QRect(at.x()*zoom_,at.y()*zoom_,zoom_,zoom_));
 if(imageChanged)imageChanged(image_);
}
void PixelCanvas::mousePressEvent(QMouseEvent* event){
 if(event->button()!=Qt::LeftButton || image_.isNull())return;
 if(tool_==Tool::Eyedropper){apply(event->position().toPoint());return;}
 beginChange();drawing_=true;apply(event->position().toPoint());
 if(tool_==Tool::Fill){drawing_=false;endChange();}
}
void PixelCanvas::mouseMoveEvent(QMouseEvent* event){
 if(drawing_ && (event->buttons()&Qt::LeftButton))apply(event->position().toPoint());
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent* event){
 if(event->button()!=Qt::LeftButton||!drawing_)return;
 drawing_=false;endChange();
}
