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
 if(transforming_)cancelTransform();
 image_=image.isNull()?QImage():image.convertToFormat(QImage::Format_RGBA8888);
 drawing_=false;changed_=false;before_=QImage();undo_.clear();redo_.clear();clearSelection();
 updateCanvasSize();notifyHistory();
}
void PixelCanvas::replaceImage(const QImage& image){
 if(image.isNull() || drawing_ || image==image_)return;
 if(transforming_)cancelTransform();
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
 p.drawImage(rect(),transforming_?renderTransform():image_);
 if(selectionDrag_==SelectionDrag::Create){
  p.setPen(QPen(QColor("#c7a3ff"),0,Qt::DashLine));
  p.setBrush(QColor(142,91,220,28));
  p.drawRect(QRect(QPoint(std::min(start_.x(),dragEnd_.x())*zoom_,std::min(start_.y(),dragEnd_.y())*zoom_),QSize((std::abs(start_.x()-dragEnd_.x())+1)*zoom_,(std::abs(start_.y()-dragEnd_.y())+1)*zoom_)).adjusted(0,0,-1,-1));
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
 if((rectangleSelection_ && hasSelection()) || transforming_){
  const QRect bounds=transforming_?transformRect_:activeRect();
  if(!bounds.isEmpty()){
   p.setPen(QPen(QColor("#ba8cff"),1,Qt::DashLine));
   p.setBrush(Qt::NoBrush);
   const QRect display(bounds.left()*zoom_,bounds.top()*zoom_,bounds.width()*zoom_,bounds.height()*zoom_);
   p.drawRect(display.adjusted(0,0,-1,-1));
   p.setBrush(QColor("#eee7ff"));
   p.setPen(QPen(QColor("#7950b7"),1));
   const QPoint corners[]={display.topLeft(),display.topRight(),display.bottomLeft(),display.bottomRight()};
   for(const QPoint& point:corners)p.drawRect(QRect(point-QPoint(4,4),QSize(9,9)));
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
 if(transforming_)return;
 if(!canUndo()||drawing_)return;
 redo_.push_back(image_.copy());image_=undo_.takeLast();
 update();notifyHistory();if(imageChanged)imageChanged(image_);
}
void PixelCanvas::redo(){
 if(transforming_)return;
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
 if(transforming_){
  const QPoint pixel=event->position().toPoint()/zoom_;
  transformHandle_=hitRect(transformRect_,pixel);
  if(transformHandle_!=SelectionDrag::None){
   transformDragging_=true;transformDragAnchor_=pixel;transformDragOriginal_=transformRect_;
  }
  return;
 }
 if(tool_==Tool::Eyedropper){apply(event->position().toPoint());return;}
 if(tool_==Tool::MagicWand){
  const QPoint start=event->position().toPoint()/zoom_;
  if(!image_.rect().contains(start))return;
  const QColor target=image_.pixelColor(start);int w=image_.width(),h=image_.height();
  rectangleSelection_=false;selectionBounds_=QRect();
 selected_=QVector<quint8>(w*h,0);QVector<QPoint> q;q.push_back(start);selected_[start.y()*w+start.x()]=1;
  auto matches=[&](int x,int y){auto col=image_.pixelColor(x,y);return std::max({qAbs(target.red()-col.red()),qAbs(target.green()-col.green()),qAbs(target.blue()-col.blue()),qAbs(target.alpha()-col.alpha())})<=tolerance_;};
  for(qsizetype i=0;i<q.size();++i){auto p=q.at(i);auto add=[&](int x,int y){if(x<0||y<0||x>=w||y>=h)return;int k=y*w+x;if(!selected_[k]&&matches(x,y)){selected_[k]=1;q.push_back(QPoint(x,y));}};add(p.x()-1,p.y());add(p.x()+1,p.y());add(p.x(),p.y()-1);add(p.x(),p.y()+1);}
  update();return;
 }
 if(tool_==Tool::RectangleSelect){
  const QPoint hit=event->position().toPoint()/zoom_;
  selectionDrag_=hitSelection(hit);
  if(selectionDrag_==SelectionDrag::None){
   clearSelection();
   selectionDrag_=SelectionDrag::Create;
   start_=QPoint(qBound(0,hit.x(),image_.width()-1),qBound(0,hit.y(),image_.height()-1));
   dragEnd_=start_;
  }else{
   dragOriginal_=activeRect();
   dragAnchor_=hit;
  }
  selecting_=true;update();return;
 }
 beginChange();drawing_=true;apply(event->position().toPoint());
 if(tool_==Tool::Fill){drawing_=false;endChange();}
}
void PixelCanvas::mouseMoveEvent(QMouseEvent* event){
 if(transforming_ && transformDragging_ && (event->buttons()&Qt::LeftButton)){
  const QPoint pixel=event->position().toPoint()/zoom_;
  const QPoint delta=pixel-transformDragAnchor_;
  QRect r=transformDragOriginal_;
  if(transformHandle_==SelectionDrag::Move)r.translate(delta);
  else {
   QPoint a=r.topLeft(),b=r.bottomRight();
   if(transformHandle_==SelectionDrag::ResizeTL)a+=delta;
   else if(transformHandle_==SelectionDrag::ResizeTR){a.ry()+=delta.y();b.rx()+=delta.x();}
   else if(transformHandle_==SelectionDrag::ResizeBL){a.rx()+=delta.x();b.ry()+=delta.y();}
   else if(transformHandle_==SelectionDrag::ResizeBR)b+=delta;
   r=QRect(a,b).normalized();
  }
  transformRect_=r;
  update();return;
 }

 if(selecting_ && (event->buttons()&Qt::LeftButton)){
  const QPoint pos=event->position().toPoint()/zoom_;
  if(selectionDrag_==SelectionDrag::Create){
   dragEnd_=QPoint(qBound(0,pos.x(),image_.width()-1),qBound(0,pos.y(),image_.height()-1));
  }else if(selectionDrag_!=SelectionDrag::None){
   const QPoint d=pos-dragAnchor_;
   QRect rect=dragOriginal_;
   if(selectionDrag_==SelectionDrag::Move){
    rect.moveTopLeft(QPoint(qBound(0,rect.left()+d.x(),image_.width()-rect.width()),
                            qBound(0,rect.top()+d.y(),image_.height()-rect.height())));
   }else{
    QPoint a=dragOriginal_.topLeft(),b=dragOriginal_.bottomRight();
    if(selectionDrag_==SelectionDrag::ResizeTL)a+=d;
    if(selectionDrag_==SelectionDrag::ResizeTR){a.ry()+=d.y();b.rx()+=d.x();}
    if(selectionDrag_==SelectionDrag::ResizeBL){a.rx()+=d.x();b.ry()+=d.y();}
    if(selectionDrag_==SelectionDrag::ResizeBR)b+=d;
    a.setX(qBound(0,a.x(),image_.width()-1));a.setY(qBound(0,a.y(),image_.height()-1));
    b.setX(qBound(0,b.x(),image_.width()-1));b.setY(qBound(0,b.y(),image_.height()-1));
    rect=QRect(a,b).normalized();
   }
   setRectangleSelection(rect);
  }
  update();return;
 }
 if(tool_==Tool::RectangleSelect && !selecting_ && image_.rect().contains(event->position().toPoint()/zoom_)){
  const auto hit=hitSelection(event->position().toPoint()/zoom_);
  setCursor(hit==SelectionDrag::Move?Qt::SizeAllCursor:hit!=SelectionDrag::None?Qt::SizeFDiagCursor:Qt::CrossCursor);
 }
 if(drawing_ && (event->buttons()&Qt::LeftButton))apply(event->position().toPoint());
}
void PixelCanvas::mouseReleaseEvent(QMouseEvent* event){
 if(event->button()!=Qt::LeftButton)return;
 if(transforming_){transformDragging_=false;transformHandle_=SelectionDrag::None;return;}
 if(selecting_){
  selecting_=false;
  if(selectionDrag_==SelectionDrag::Create){
   const QPoint end=event->position().toPoint()/zoom_;
   const QPoint bounded(qBound(0,end.x(),image_.width()-1),qBound(0,end.y(),image_.height()-1));
   setRectangleSelection(QRect(start_,bounded).normalized().intersected(image_.rect()));
  }
  selectionDrag_=SelectionDrag::None;
  update();return;
 }
 if(!drawing_)return;
 drawing_=false;endChange();
}

void PixelCanvas::clearSelection(){
 if(transforming_)cancelTransform();
 selected_.clear();selecting_=false;rectangleSelection_=false;selectionBounds_=QRect();
 selectionDrag_=SelectionDrag::None;update();
}
QRect PixelCanvas::activeRect() const{return selectionBounds_;}
void PixelCanvas::setRectangleSelection(const QRect& incoming){
 const QRect r=incoming.normalized().intersected(image_.rect());
 if(r.isEmpty())return;
 selected_=QVector<quint8>(image_.width()*image_.height(),0);
 for(int y=r.top();y<=r.bottom();++y)for(int x=r.left();x<=r.right();++x)
  selected_[y*image_.width()+x]=1;
 selectionBounds_=r;rectangleSelection_=true;update();
}
PixelCanvas::SelectionDrag PixelCanvas::hitSelection(const QPoint& p) const{
 if(!rectangleSelection_ || !hasSelection())return SelectionDrag::None;
 const QRect r=activeRect();
 const int radius=std::max(1,6/zoom_);
 const auto near=[&](QPoint a){return std::abs(p.x()-a.x())<=radius && std::abs(p.y()-a.y())<=radius;};
 if(near(r.topLeft()))return SelectionDrag::ResizeTL;
 if(near(r.topRight()))return SelectionDrag::ResizeTR;
 if(near(r.bottomLeft()))return SelectionDrag::ResizeBL;
 if(near(r.bottomRight()))return SelectionDrag::ResizeBR;
 return r.contains(p)?SelectionDrag::Move:SelectionDrag::None;
}
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

PixelCanvas::SelectionDrag PixelCanvas::hitRect(const QRect& rect,const QPoint& p) const{
 const int radius=std::max(1,6/zoom_);
 const auto near=[&](const QPoint& a){return std::abs(p.x()-a.x())<=radius && std::abs(p.y()-a.y())<=radius;};
 if(near(rect.topLeft()))return SelectionDrag::ResizeTL;
 if(near(rect.topRight()))return SelectionDrag::ResizeTR;
 if(near(rect.bottomLeft()))return SelectionDrag::ResizeBL;
 if(near(rect.bottomRight()))return SelectionDrag::ResizeBR;
 return rect.contains(p)?SelectionDrag::Move:SelectionDrag::None;
}
bool PixelCanvas::beginTransform(){
 if(transforming_||image_.isNull()||selected_.isEmpty())return false;
 int left=image_.width(),top=image_.height(),right=-1,bottom=-1;
 for(int y=0;y<image_.height();++y)for(int x=0;x<image_.width();++x)
  if(selected_[y*image_.width()+x]){
   left=std::min(left,x);top=std::min(top,y);
   right=std::max(right,x);bottom=std::max(bottom,y);
  }
 if(right<left)return false;
 const QRect bounds(QPoint(left,top),QPoint(right,bottom));
 transformBase_=image_.copy();
 transformPatch_=QImage(bounds.size(),QImage::Format_ARGB32);
 transformPatch_.fill(Qt::transparent);
 for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x)
  if(selected_[y*image_.width()+x]){
   transformPatch_.setPixelColor(x-left,y-top,image_.pixelColor(x,y));
   transformBase_.setPixelColor(x,y,QColor(0,0,0,0));
  }
 transformRect_=bounds;transforming_=true;transformDragging_=false;
 rectangleSelection_=false;selected_.clear();
 if(transformChanged)transformChanged(true);
 update();return true;
}
QImage PixelCanvas::renderTransform() const{
 if(!transforming_)return image_;
 QImage out=transformBase_.copy();
 QPainter painter(&out);
 painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
 painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
 painter.drawImage(transformRect_,transformPatch_);
 return out;
}
void PixelCanvas::confirmTransform(){
 if(!transforming_||transformDragging_)return;
 QImage result=renderTransform();
 transforming_=false;transformPatch_=QImage();transformBase_=QImage();
 transformRect_=QRect();transformHandle_=SelectionDrag::None;
 if(transformChanged)transformChanged(false);
 replaceImage(result);
 update();
}
void PixelCanvas::cancelTransform(){
 if(!transforming_)return;
 transforming_=false;transformDragging_=false;transformPatch_=QImage();
 transformBase_=QImage();transformRect_=QRect();transformHandle_=SelectionDrag::None;
 if(transformChanged)transformChanged(false);
 update();
}
