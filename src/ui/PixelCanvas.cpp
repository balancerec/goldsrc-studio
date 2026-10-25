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
 drawing_=false;changed_=false;before_=QImage();undo_.clear();redo_.clear();clearSelection();
 updateCanvasSize();notifyHistory();
}
void PixelCanvas::replaceImage(const QImage& image){
 if(image.isNull() || drawing_ || image==image_)return;
 undo_.push_back(image_.copy());
 if(undo_.size()>HistoryLimit)undo_.removeFirst();
 redo_.clear();image_=image.convertToFormat(QImage::Format_RGBA8888);clearSelection();
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
 if(selecting_){
  p.setPen(QPen(QColor("#c7a3ff"),0,Qt::DashLine));
  p.setBrush(QColor(142,91,220,28));
  p.drawRect(QRect(start_*zoom_,dragEnd_*zoom_).normalized());
  p.setBrush(Qt::NoBrush);
 }
 if(!selected_.isEmpty()){
  p.setPen(QPen(QColor("#b18cff"),0,Qt::DashLine));
  int w=image_.width(),h=image_.height();
  for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(selected_[y*w+x]){
   if(x==0||!selected_[y*w+x-1])p.drawLine(x*zoom_,y*zoom_,x*zoom_,(y+1)*zoom_);
   if(x==w-1||!selected_[y*w+x+1])p.drawLine((x+1)*zoom_,y*zoom_,(x+1)*zoom_,(y+1)*zoom_);
   if(y==0||!selected_[(y-1)*w+x])p.drawLine(x*zoom_,y*zoom_,(x+1)*zoom_,y*zoom_);
   if(y==h-1||!selected_[(y+1)*w+x])p.drawLine(x*zoom_,(y+1)*zoom_,(x+1)*zoom_,(y+1)*zoom_);
  }
 }
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
 if(tool_==Tool::RectangleSelect||tool_==Tool::MagicWand)return;
 const QColor paint=tool_==Tool::Eraser?QColor(0,0,0,0):color_;
 if(image_.pixelColor(at)==paint)return;
 image_.setPixelColor(at,paint);changed_=true;
 update(QRect(at.x()*zoom_,at.y()*zoom_,zoom_,zoom_));
 if(imageChanged)imageChanged(image_);
}
void PixelCanvas::mousePressEvent(QMouseEvent* event){
 if(event->button()!=Qt::LeftButton || image_.isNull())return;
 if(tool_==Tool::Eyedropper){apply(event->position().toPoint());return;}
 if(tool_==Tool::MagicWand){
  const QPoint start=event->position().toPoint()/zoom_;
  if(!image_.rect().contains(start))return;
  const QColor target=image_.pixelColor(start);int w=image_.width(),h=image_.height();
  selected_=QVector<quint8>(w*h,0);QVector<QPoint> q;q.push_back(start);selected_[start.y()*w+start.x()]=1;
  auto matches=[&](int x,int y){auto col=image_.pixelColor(x,y);return std::max({qAbs(target.red()-col.red()),qAbs(target.green()-col.green()),qAbs(target.blue()-col.blue()),qAbs(target.alpha()-col.alpha())})<=tolerance_;};
  for(qsizetype i=0;i<q.size();++i){auto p=q.at(i);auto add=[&](int x,int y){if(x<0||y<0||x>=w||y>=h)return;int k=y*w+x;if(!selected_[k]&&matches(x,y)){selected_[k]=1;q.push_back(QPoint(x,y));}};add(p.x()-1,p.y());add(p.x()+1,p.y());add(p.x(),p.y()-1);add(p.x(),p.y()+1);}
  update();return;
 }
 if(tool_==Tool::RectangleSelect){clearSelection();selecting_=true;start_=event->position().toPoint()/zoom_;start_.setX(qBound(0,start_.x(),image_.width()-1));start_.setY(qBound(0,start_.y(),image_.height()-1));dragEnd_=start_;update();return;}
 beginChange();drawing_=true;apply(event->position().toPoint());
 if(tool_==Tool::Fill){drawing_=false;endChange();}
}
void PixelCanvas::mouseMoveEvent(QMouseEvent* event){
 if(selecting_ && (event->buttons()&Qt::LeftButton)){
  dragEnd_=event->position().toPoint()/zoom_;
  dragEnd_.setX(qBound(0,dragEnd_.x(),image_.width()-1));dragEnd_.setY(qBound(0,dragEnd_.y(),image_.height()-1));update();return;
 }
 if(drawing_ && (event->buttons()&Qt::LeftButton))apply(event->position().toPoint());
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent* event){
 if(event->button()!=Qt::LeftButton)return;
 if(selecting_){
  selecting_=false;
  QPoint end=event->position().toPoint()/zoom_;
  QRect area=QRect(start_,end).normalized().intersected(image_.rect());
  selected_=QVector<quint8>(image_.width()*image_.height(),0);
  for(int y=area.top();y<=area.bottom();++y)for(int x=area.left();x<=area.right();++x)selected_[y*image_.width()+x]=1;
  update();return;
 }
 if(!drawing_)return;
 drawing_=false;endChange();
}

void PixelCanvas::clearSelection(){selected_.clear();selecting_=false;update();}
void PixelCanvas::cropSelection(){
 if(image_.isNull()||selected_.isEmpty())return;
 int l=image_.width(),t=image_.height(),r=-1,b=-1,w=image_.width();
 for(int y=0;y<image_.height();++y)for(int x=0;x<w;++x)if(selected_[y*w+x]){l=std::min(l,x);t=std::min(t,y);r=std::max(r,x);b=std::max(b,y);}
 if(r>=l)replaceImage(image_.copy(QRect(QPoint(l,t),QPoint(r,b))));
}
void PixelCanvas::eraseSelection(){
 if(image_.isNull()||selected_.isEmpty())return;
 QImage result=image_.copy();bool changed=false;int w=result.width();
 for(int y=0;y<result.height();++y)for(int x=0;x<w;++x)if(selected_[y*w+x]&&result.pixelColor(x,y).alpha()){result.setPixelColor(x,y,QColor(0,0,0,0));changed=true;}
 if(changed)replaceImage(result);
}
