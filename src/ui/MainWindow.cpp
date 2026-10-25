#include "MainWindow.h"
#include "PixelCanvas.h"
#include <QComboBox>
#include <QFrame>
#include <QShortcut>
#include <QStackedWidget>
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
 setWindowTitle("GoldSrc Studio  |  Editor profissional WAD3");
 resize(1520,920);setMinimumSize(940,640);
 setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks);
 auto* bar=addToolBar("Ações principais");bar->setObjectName("mainToolbar");bar->setMovable(false);bar->setIconSize(QSize(18,18));
 auto action=[&](const QString& label,QStyle::StandardPixmap icon,auto callback){
  auto* a=bar->addAction(style()->standardIcon(icon),label);
  connect(a,&QAction::triggered,this,callback);return a;};
 auto* open=action("ABRIR WAD3",QStyle::SP_DialogOpenButton,[this]{openWad();});open->setShortcut(QKeySequence::Open);
 action("IMPORTAR IMAGENS",QStyle::SP_FileIcon,[this]{importImages();});
 auto* save=action("EXPORTAR WAD3",QStyle::SP_DialogSaveButton,[this]{exportWad();});save->setShortcut(QKeySequence::Save);
 bar->addSeparator();
 action("DUPLICAR",QStyle::SP_FileDialogNewFolder,[this]{duplicateTexture();});
 action("EXCLUIR",QStyle::SP_TrashIcon,[this]{removeTexture();});
 auto* file=menuBar()->addMenu("&Arquivo");file->addAction(open);file->addAction(save);file->addSeparator();file->addAction("Sair",this,&QWidget::close);
 auto* view=menuBar()->addMenu("&Exibir");
 auto* help=menuBar()->addMenu("&Ajuda");
 help->addAction("Sobre",this,[this]{QMessageBox::about(this,"GoldSrc Studio","GoldSrc Studio — Qt 6 / C++20\\nFerramentas de textura para GoldSrc.\\nCompilação de desenvolvimento.");});
 auto* library=new QDockWidget("BIBLIOTECA DE TEXTURAS",this);library->setObjectName("Biblioteca");library->setMinimumWidth(250);
 auto* side=new QWidget;auto* sl=new QVBoxLayout(side);sl->setContentsMargins(12,16,12,12);sl->setSpacing(12);
 auto* libCaption=new QLabel("PACOTE ATUAL");libCaption->setObjectName("eyebrow");sl->addWidget(libCaption);
 auto* search=new QLineEdit;search->setPlaceholderText("Pesquisar por nome...");search->setClearButtonEnabled(true);sl->addWidget(search);
 list_=new QListWidget;list_->setIconSize(QSize(66,66));list_->setSpacing(5);list_->setAlternatingRowColors(false);sl->addWidget(list_,1);
 auto* addTexture=new QPushButton("+  Importar texturas");addTexture->setObjectName("ghostButton");sl->addWidget(addTexture);
 connect(addTexture,&QPushButton::clicked,this,[this]{importImages();});
 library->setWidget(side);addDockWidget(Qt::LeftDockWidgetArea,library);
 connect(search,&QLineEdit::textChanged,this,[this](const QString& q){for(int i=0;i<list_->count();++i)list_->item(i)->setHidden(!list_->item(i)->text().contains(q,Qt::CaseInsensitive));});
 connect(list_,&QListWidget::currentRowChanged,this,[this](int index){selectTexture(index);});
 auto* center=new QWidget;auto* cl=new QVBoxLayout(center);cl->setContentsMargins(20,18,20,14);cl->setSpacing(13);
 auto* header=new QHBoxLayout;
 auto* titleColumn=new QVBoxLayout;titleColumn->setSpacing(2);
 auto* eyebrow=new QLabel("GOLDSRC  /  WORKSPACE");eyebrow->setObjectName("eyebrow");titleColumn->addWidget(eyebrow);
 auto* title=new QLabel("Editor de texturas");title->setObjectName("pageTitle");titleColumn->addWidget(title);
 header->addLayout(titleColumn);header->addStretch();
 auto* engine=new QLabel("WAD3  •  MIPTEX  •  256 CORES");engine->setObjectName("chip");header->addWidget(engine);cl->addLayout(header);
 auto* toolpanel=new QFrame;toolpanel->setObjectName("toolPanel");auto* tools=new QHBoxLayout(toolpanel);tools->setContentsMargins(12,9,12,9);tools->setSpacing(10);
 tools->addWidget(new QLabel("FERRAMENTA"));
 auto* mode=new QComboBox;mode->addItems({"✎  Pincel","◌  Borracha","◎  Conta-gotas"});mode->setMinimumWidth(155);tools->addWidget(mode);
 colorButton_=new QPushButton("●  Cor");colorButton_->setObjectName("colorButton");tools->addWidget(colorButton_);
 auto* toolSeparator=new QFrame;toolSeparator->setFrameShape(QFrame::VLine);toolSeparator->setObjectName("toolDivider");tools->addWidget(toolSeparator);
 tools->addWidget(new QLabel("ZOOM"));
 auto* zoom=new QSpinBox;zoom->setRange(1,32);zoom->setValue(4);zoom->setSuffix("×");zoom->setToolTip("Escala de 1 a 32 vezes");zoom->setFixedWidth(80);tools->addWidget(zoom);
 auto* grid=new QCheckBox("Grade de pixels");grid->setChecked(true);tools->addWidget(grid);tools->addStretch();
 cl->addWidget(toolpanel);
 auto* stage=new QFrame;stage->setObjectName("workspaceStage");auto* stageLayout=new QVBoxLayout(stage);stageLayout->setContentsMargins(0,0,0,0);
 auto* scroll=new QScrollArea;scroll->setObjectName("canvasScroll");scroll->setWidgetResizable(false);scroll->setAlignment(Qt::AlignCenter);scroll->setFrameShape(QFrame::NoFrame);
 canvas_=new PixelCanvas;canvas_->resize(320,320);scroll->setWidget(canvas_);stageLayout->addWidget(scroll,1);
 cl->addWidget(stage,1);
 auto* footer=new QHBoxLayout;
 summary_=new QLabel("Nenhuma textura carregada");summary_->setObjectName("workspaceFooter");footer->addWidget(summary_);
 footer->addStretch();auto* scaleHint=new QLabel("Roda do mouse: rolagem  •  Edição: clique e arraste");scaleHint->setObjectName("workspaceFooter");footer->addWidget(scaleHint);
 cl->addLayout(footer);setCentralWidget(center);
 connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){canvas_->setTool(static_cast<PixelCanvas::Tool>(index));});
 connect(zoom,qOverload<int>(&QSpinBox::valueChanged),canvas_,&PixelCanvas::setZoom);
 connect(grid,&QCheckBox::toggled,canvas_,&PixelCanvas::setGrid);
 connect(colorButton_,&QPushButton::clicked,this,[this]{
  QColor c=QColorDialog::getColor(canvas_->color(),this,"Escolher cor",QColorDialog::ShowAlphaChannel);
  if(c.isValid()) {canvas_->setColor(c);colorButton_->setStyleSheet(QString("background-color:%1;color:%2;").arg(c.name(),c.lightness()<128?"white":"black"));}
 });
 canvas_->colorPicked=[this,mode](const QColor& c){canvas_->setColor(c);mode->setCurrentIndex(0);statusBar()->showMessage("Cor capturada: "+c.name(QColor::HexArgb));};
 canvas_->imageChanged=[this](const QImage& im){int i=list_->currentRow();if(i<0||i>=textures_.size())return;textures_[i].image=im;
  if(auto* it=list_->item(i))it->setIcon(QIcon(QPixmap::fromImage(im).scaled(66,66,Qt::KeepAspectRatio,Qt::FastTransformation)));
  statusBar()->showMessage("Textura modificada. Exporte o WAD3 para salvar.");};
 auto* props=new QDockWidget("INSPETOR",this);props->setObjectName("Inspetor");props->setMinimumWidth(275);
 auto* inspector=new QWidget;auto* il=new QVBoxLayout(inspector);il->setContentsMargins(14,16,14,14);il->setSpacing(12);
 auto* details=new QLabel("PROPRIEDADES DA TEXTURA");details->setObjectName("eyebrow");il->addWidget(details);
 auto* panel=new QGroupBox("Identificação e dimensões");auto* form=new QFormLayout(panel);form->setSpacing(11);form->setContentsMargins(12,20,12,14);
 name_=new QLineEdit;name_->setMaxLength(15);name_->setPlaceholderText("NOME_TEXTURA");form->addRow("Nome",name_);
 width_=new QSpinBox;width_->setRange(16,4096);width_->setSingleStep(16);form->addRow("Largura",width_);
 height_=new QSpinBox;height_->setRange(16,4096);height_->setSingleStep(16);form->addRow("Altura",height_);
 il->addWidget(panel);
 auto* rename=new QPushButton("Aplicar novo nome");il->addWidget(rename);connect(rename,&QPushButton::clicked,this,[this]{renameTexture();});
 auto* resize=new QPushButton("Aplicar dimensões");resize->setObjectName("ghostButton");il->addWidget(resize);connect(resize,&QPushButton::clicked,this,[this]{resizeTexture();});
 auto* goldsrcLabel=new QLabel("REGRAS GOLDSRC");goldsrcLabel->setObjectName("eyebrow");il->addSpacing(10);il->addWidget(goldsrcLabel);
 auto* note=new QLabel("Nomes de até 15 caracteres nesta versão. A exportação gera quatro mipmaps e usa paleta indexada de 256 cores. Texturas originais intactas mantêm seus dados MIPTEX.");note->setWordWrap(true);note->setObjectName("smallNote");il->addWidget(note);il->addStretch();
 props->setWidget(inspector);addDockWidget(Qt::RightDockWidgetArea,props);
 view->addAction(library->toggleViewAction());view->addAction(props->toggleViewAction());
 statusBar()->showMessage("PRONTO  •  Abra WAD3 ou importe imagens para começar");
}
void MainWindow::setDarkTheme(){
 qApp->setStyle("Fusion");
 qApp->setStyleSheet(R"(
 QWidget{background:#0f1218;color:#e4e9f2;font-family:'Segoe UI',Arial;font-size:12px}
 QMainWindow{background:#0c0f14}
 QMenuBar{background:#131924;padding:6px 8px;border-bottom:1px solid #29323f}
 QMenuBar::item{padding:7px 12px}
 QMenuBar::item:selected,QMenu::item:selected{background:#a82f42;border-radius:4px}
 QMenu{background:#1b2330;border:1px solid #303a4a}
 QToolBar#mainToolbar{background:#141b25;padding:11px 14px;spacing:8px;border-bottom:1px solid #2c3441}
 QToolBar#mainToolbar QToolButton{color:#e6edf5;background:#222b38;padding:9px 12px;border:1px solid #344151;border-radius:7px;font-weight:650}
 QToolBar#mainToolbar QToolButton:hover{background:#323d4d;border-color:#d34b60}
 QDockWidget{background:#111720;border:0}
 QDockWidget::title{background:#1e2734;padding:12px;font-weight:700;letter-spacing:1px}
 QLabel#eyebrow{color:#d9576c;font-size:10px;letter-spacing:2px;font-weight:700}
 QLabel#pageTitle{font-size:24px;font-weight:700;color:#f3f5f8}
 QLabel#chip{background:#252f3b;border:1px solid #3b4656;border-radius:13px;padding:7px 12px;color:#bbc9d9;font-weight:600}
 QLineEdit,QSpinBox,QComboBox{background:#1b2531;color:#f1f4fa;border:1px solid #3a4658;border-radius:7px;padding:10px;selection-background-color:#9d3042}
 QLineEdit:focus,QSpinBox:focus,QComboBox:focus{border:1px solid #dc566a}
 QListWidget{background:#131a24;border:1px solid #344051;border-radius:8px;padding:6px}
 QListWidget::item{padding:9px 7px;border-radius:7px;margin-bottom:3px}
 QListWidget::item:selected{background:#77303d;border:1px solid #b94b60}
 QListWidget::item:hover{background:#293342}
 QFrame#toolPanel{background:#1b2330;border:1px solid #344050;border-radius:10px}
 QFrame#toolDivider{background:#3b4655;max-width:1px}
 QFrame#workspaceStage{background:#151b25;border:1px solid #344050;border-radius:10px}
 QScrollArea#canvasScroll{background:#151b25;border:0}
 QScrollArea#canvasScroll > QWidget > QWidget{background:#151b25}
 QGroupBox{border:1px solid #344153;border-radius:9px;margin-top:14px;padding:13px 8px 10px;font-weight:650}
 QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 5px;color:#dee6f1}
 QPushButton{background:#a83246;color:white;padding:12px;border:1px solid #c84a5c;border-radius:8px;font-weight:650}
 QPushButton:hover{background:#c13b50}
 QPushButton#ghostButton{background:#253041;border:1px solid #405066}
 QPushButton#ghostButton:hover{background:#34435a}
 QPushButton#colorButton{background:#a83246}
 QCheckBox{spacing:8px}
 QCheckBox::indicator{width:15px;height:15px}
 QLabel#smallNote{color:#9cabbd;font-size:12px;line-height:1.5}
 QLabel#workspaceFooter{color:#94a4b9;font-size:11px}
 QStatusBar{background:#191f2a;color:#a9bbca;border-top:1px solid #313b49;padding:4px}
 QScrollBar:vertical{background:#19212b;width:11px}
 QScrollBar::handle:vertical{background:#455365;border-radius:4px;min-height:28px}
 QScrollBar:horizontal{background:#19212b;height:11px}
 QScrollBar::handle:horizontal{background:#455365;border-radius:4px;min-width:28px}
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
void MainWindow::removeTexture(){int i=list_->currentRow();if(i<0)return;textures_.removeAt(i);refreshList(std::min(i, static_cast<int>(textures_.size()) - 1));}
void MainWindow::duplicateTexture(){int i=list_->currentRow();if(i<0)return;auto t=textures_[i];t.name=goldsrc::normalizedName(t.name.left(10)+"_COPY");textures_.insert(i+1,t);refreshList(i+1);}
void MainWindow::resizeTexture(){int i=list_->currentRow();if(i<0)return;int w=width_->value(),h=height_->value();if(w*h>16777216){QMessageBox::warning(this,"Dimensões inválidas","Imagem grande demais.");return;}textures_[i].image=textures_[i].image.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);refreshList(i);}
void MainWindow::renameTexture(){int i=list_->currentRow();if(i<0)return;textures_[i].name=goldsrc::normalizedName(name_->text());refreshList(i);}
