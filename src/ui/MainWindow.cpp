#include "MainWindow.h"
#include "PixelCanvas.h"
#include <QComboBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QAction>
#include <QMenuBar>
#include <QApplication>
#include <QMenu>
#include <QPixmap>
#include <QIcon>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyle>
#include <QKeySequence>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent){setupUi();setDarkTheme();}
void MainWindow::setupUi(){
 setWindowTitle("GoldSrc Studio — Editor WAD3");resize(1360,820);setMinimumSize(760,540);
 auto* toolbar=addToolBar("Ferramentas");toolbar->setMovable(false);toolbar->setIconSize(QSize(20,20));
 auto add=[&](const QString& text, QStyle::StandardPixmap ico,auto fn){auto* a=toolbar->addAction(style()->standardIcon(ico),text);connect(a,&QAction::triggered,this,fn);return a;};
 auto* aOpen=add("Abrir WAD3",QStyle::SP_DialogOpenButton,[this]{openWad();});aOpen->setShortcut(QKeySequence::Open);
 add("Importar imagens",QStyle::SP_FileIcon,[this]{importImages();});
 auto* aExport=add("Exportar WAD3",QStyle::SP_DialogSaveButton,[this]{exportWad();});aExport->setShortcut(QKeySequence::Save);
 toolbar->addSeparator();add("Duplicar",QStyle::SP_FileDialogNewFolder,[this]{duplicateTexture();});add("Excluir",QStyle::SP_TrashIcon,[this]{removeTexture();});
 auto* menu=menuBar()->addMenu("&Arquivo");menu->addAction(aOpen);menu->addAction(aExport);menu->addSeparator();menu->addAction("Sair",this,&QWidget::close);
 auto* help=menuBar()->addMenu("&Ajuda");help->addAction("Sobre",this,[this]{QMessageBox::about(this,"GoldSrc Studio","GoldSrc Studio 0.1.0\nEditor WAD3 nativo — C++20 + Qt 6\nProjeto inicial em desenvolvimento.");});
 auto* browserDock=new QDockWidget("Biblioteca de texturas",this);browserDock->setObjectName("Biblioteca");browserDock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
 auto* browser=new QWidget;auto* browserLayout=new QVBoxLayout(browser);auto* search=new QLineEdit;search->setPlaceholderText("Pesquisar textura...");browserLayout->addWidget(search);
 list_=new QListWidget;list_->setIconSize(QSize(64,64));list_->setSpacing(4);browserLayout->addWidget(list_);browserDock->setWidget(browser);addDockWidget(Qt::LeftDockWidgetArea,browserDock);
 connect(search,&QLineEdit::textChanged,this,[this](const QString& s){for(int i=0;i<list_->count();++i)list_->item(i)->setHidden(!list_->item(i)->text().contains(s,Qt::CaseInsensitive));});
 connect(list_,&QListWidget::currentRowChanged,this,[this](int index){selectTexture(index);});
 auto* central=new QWidget;auto* centerLayout=new QVBoxLayout(central);centerLayout->setContentsMargins(18,18,18,18);
 auto* title=new QLabel("EDITOR DE TEXTURAS");title->setObjectName("sectionTitle");centerLayout->addWidget(title);
 auto* tools=new QHBoxLayout;
 auto* mode=new QComboBox;mode->addItems({"Pincel","Borracha","Conta-gotas"});tools->addWidget(mode);
 colorButton_=new QPushButton("Cor do pincel");tools->addWidget(colorButton_);
 auto* zoom=new QSpinBox;zoom->setRange(1,32);zoom->setValue(4);zoom->setSuffix("×");zoom->setToolTip("Zoom de 1 a 32 vezes");tools->addWidget(zoom);
 auto* grid=new QCheckBox("Grade");grid->setChecked(true);tools->addWidget(grid);tools->addStretch();centerLayout->addLayout(tools);
 auto* scroll=new QScrollArea;scroll->setWidgetResizable(false);scroll->setAlignment(Qt::AlignCenter);scroll->setFrameShape(QFrame::NoFrame);
 canvas_=new PixelCanvas;canvas_->resize(320,320);scroll->setWidget(canvas_);centerLayout->addWidget(scroll,1);
 connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){canvas_->setTool(static_cast<PixelCanvas::Tool>(i));});
 connect(zoom,qOverload<int>(&QSpinBox::valueChanged),canvas_,&PixelCanvas::setZoom);
 connect(grid,&QCheckBox::toggled,canvas_,&PixelCanvas::setGrid);
 connect(colorButton_,&QPushButton::clicked,this,[this]{
  QColor c=QColorDialog::getColor(canvas_->color(),this,"Cor do pincel",QColorDialog::ShowAlphaChannel);
  if(c.isValid())canvas_->setColor(c);
 });
 canvas_->colorPicked=[this,mode](const QColor& c){
  canvas_->setColor(c);mode->setCurrentIndex(0);
  statusBar()->showMessage(QString("Cor selecionada: %1").arg(c.name(QColor::HexArgb)));
 };
 canvas_->imageChanged=[this](const QImage& image) {
  const int i=list_->currentRow();if(i<0||i>=textures_.size())return;
  textures_[i].image=image;
  // Recria apenas a miniatura selecionada, mantendo a posição na lista.
  if(auto* item=list_->item(i))
   item->setIcon(QIcon(QPixmap::fromImage(image).scaled(64,64,Qt::KeepAspectRatio,Qt::FastTransformation)));
  statusBar()->showMessage("Textura modificada — exporte para salvar em WAD3.");
 };
 summary_=new QLabel("Nenhuma textura carregada");centerLayout->addWidget(summary_);setCentralWidget(central);
 auto* propertiesDock=new QDockWidget("Inspetor",this);propertiesDock->setObjectName("Inspetor");propertiesDock->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
 auto* props=new QWidget;auto* propsLayout=new QVBoxLayout(props);auto* group=new QGroupBox("Propriedades da textura");auto* form=new QFormLayout(group);
 name_=new QLineEdit;name_->setMaxLength(15);name_->setPlaceholderText("NOME_TEXTURA");form->addRow("Nome",name_);
 width_=new QSpinBox;width_->setRange(16,4096);width_->setSingleStep(16);form->addRow("Largura",width_);
 height_=new QSpinBox;height_->setRange(16,4096);height_->setSingleStep(16);form->addRow("Altura",height_);
 propsLayout->addWidget(group);
 auto* rename=new QPushButton("Aplicar nome");propsLayout->addWidget(rename);connect(rename,&QPushButton::clicked,this,[this]{renameTexture();});
 auto* resizeBtn=new QPushButton("Redimensionar textura");propsLayout->addWidget(resizeBtn);connect(resizeBtn,&QPushButton::clicked,this,[this]{resizeTexture();});
 auto* note=new QLabel("GoldSrc: nomes de até 15 caracteres nesta versão. Texturas exportadas em dimensões múltiplas de 16 com quatro mipmaps e paleta de 256 cores.");note->setWordWrap(true);note->setObjectName("smallNote");propsLayout->addWidget(note);propsLayout->addStretch();propertiesDock->setWidget(props);addDockWidget(Qt::RightDockWidgetArea,propertiesDock);
 statusBar()->showMessage("Pronto para importar PNG, JPG ou WAD3.");
}
void MainWindow::setDarkTheme(){
 qApp->setStyleSheet(R"(
 QMainWindow,QDockWidget,QWidget{background:#11151b;color:#e7ebf2;font-family:'Segoe UI',Arial;font-size:12px}
 QMenuBar,QToolBar{background:#191f28;border-bottom:1px solid #303844;padding:5px;spacing:8px}
 QMenuBar::item:selected,QMenu::item:selected{background:#962d36}
 QMenu{background:#212732;border:1px solid #374050}
 QDockWidget::title{background:#202733;padding:11px;font-weight:700}
 QListWidget,QLineEdit,QSpinBox,QScrollArea{background:#171d25;border:1px solid #344050;border-radius:6px;padding:6px}
 QListWidget::item{padding:8px;border-radius:5px}
 QListWidget::item:selected{background:#822a32;color:#fff}
 QPushButton{background:#9c303a;border:1px solid #b54852;border-radius:7px;padding:11px;font-weight:600}
 QPushButton:hover{background:#b83d48}
 QGroupBox{border:1px solid #344050;border-radius:8px;margin-top:13px;padding:16px 8px 8px}
 QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 4px}
 QStatusBar{background:#1b222b;color:#adbdcd}
 QLabel#sectionTitle{font-size:14px;font-weight:700;letter-spacing:2px}
 QComboBox,QCheckBox{background:#171d25;color:#e7ebf2;padding:5px}
 QLabel#smallNote{color:#9ba9bb;line-height:1.5}
 )");
}
void MainWindow::refreshList(int index){
 const int wanted=index>=0?index:list_->currentRow();list_->blockSignals(true);list_->clear();
 for(const auto& t:textures_){auto* item=new QListWidgetItem(QIcon(QPixmap::fromImage(t.image).scaled(64,64,Qt::KeepAspectRatio,Qt::FastTransformation)),t.name);item->setToolTip(QString("%1 × %2").arg(t.image.width()).arg(t.image.height()));list_->addItem(item);}
 list_->blockSignals(false);list_->setCurrentRow(std::clamp(wanted,0,std::max(0,int(textures_.size())-1)));
 if(textures_.isEmpty())selectTexture(-1);
}
void MainWindow::selectTexture(int index){
 updating_=true;const bool valid=index>=0&&index<textures_.size();
 name_->setEnabled(valid);width_->setEnabled(valid);height_->setEnabled(valid);
 if(!valid){canvas_->setImage(QImage());name_->clear();summary_->setText("Nenhuma textura selecionada");updating_=false;return;}
 const auto& t=textures_.at(index);name_->setText(t.name);width_->setValue(t.image.width());height_->setValue(t.image.height());
 canvas_->setImage(t.image);
 summary_->setText(QString("%1  •  %2 × %3 px  •  %4 textura(s) no pacote").arg(t.name).arg(t.image.width()).arg(t.image.height()).arg(textures_.size()));updating_=false;
}
void MainWindow::openWad(){const QString path=QFileDialog::getOpenFileName(this,"Abrir arquivo WAD3",{},"Arquivos WAD3 (*.wad)");if(path.isEmpty())return;
 const auto r=goldsrc::readWad3(path);if(!r.ok()){QMessageBox::warning(this,"Erro ao abrir",r.error);return;}textures_=r.textures;openedPath_=path;refreshList(0);statusBar()->showMessage(QString("%1 texturas carregadas de %2").arg(textures_.size()).arg(QFileInfo(path).fileName()));}
void MainWindow::importImages(){const QStringList paths=QFileDialog::getOpenFileNames(this,"Importar imagens",{},"Imagens (*.png *.jpg *.jpeg *.bmp)");int added=0;for(const QString& path:paths){QImage img(path);if(img.isNull())continue;textures_.push_back({goldsrc::normalizedName(QFileInfo(path).fileName()),img});++added;}if(added){refreshList(textures_.size()-1);statusBar()->showMessage(QString("%1 imagem(ns) importada(s)").arg(added));}else if(!paths.isEmpty())QMessageBox::warning(this,"Falha","Nenhuma imagem válida foi importada.");}
void MainWindow::exportWad(){if(textures_.isEmpty()){QMessageBox::information(this,"Sem texturas","Importe imagens antes de exportar.");return;}
 const QString suggested=openedPath_.isEmpty()?"texturas.wad":QFileInfo(openedPath_).absolutePath()+"/texturas_exportadas.wad";
 const QString path=QFileDialog::getSaveFileName(this,"Exportar WAD3",suggested,"Arquivos WAD3 (*.wad)");if(path.isEmpty())return;QString err;
 if(!goldsrc::writeWad3(path,textures_,&err)){QMessageBox::critical(this,"Erro de exportação",err);return;}statusBar()->showMessage("Arquivo WAD3 exportado: "+path);}
void MainWindow::removeTexture(){int i=list_->currentRow();if(i<0)return;textures_.removeAt(i);refreshList(std::min(i,textures_.size()-1));}
void MainWindow::duplicateTexture(){int i=list_->currentRow();if(i<0)return;auto t=textures_[i];t.name=goldsrc::normalizedName(t.name.left(10)+"_COPY");textures_.insert(i+1,t);refreshList(i+1);}
void MainWindow::resizeTexture(){int i=list_->currentRow();if(i<0)return;int w=width_->value(),h=height_->value();if(w*h>16777216){QMessageBox::warning(this,"Dimensões inválidas","Imagem grande demais.");return;}textures_[i].image=textures_[i].image.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);refreshList(i);}
void MainWindow::renameTexture(){int i=list_->currentRow();if(i<0)return;textures_[i].name=goldsrc::normalizedName(name_->text());refreshList(i);}
