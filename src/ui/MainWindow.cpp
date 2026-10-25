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
 auto* bar=addToolBar("Ações principais");bar->setObjectName("mainToolbar");bar->setMovable(false);bar->setIconSize(QSize(17,17));bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
 auto action=[&](const QString& label,QStyle::StandardPixmap icon,auto callback){
  auto* a=bar->addAction(style()->standardIcon(icon),label);
  connect(a,&QAction::triggered,this,callback);return a;};
 auto* open=action("Abrir WAD3",QStyle::SP_DialogOpenButton,[this]{openWad();});open->setShortcut(QKeySequence::Open);
 action("Importar imagens",QStyle::SP_FileIcon,[this]{importImages();});
 auto* save=action("Exportar WAD3",QStyle::SP_DialogSaveButton,[this]{exportWad();});save->setShortcut(QKeySequence::Save);
 bar->addSeparator();
 action("Duplicar",QStyle::SP_FileDialogNewFolder,[this]{duplicateTexture();});
 action("Excluir",QStyle::SP_TrashIcon,[this]{removeTexture();});
 auto* undoAction=new QAction("Desfazer",this);
 undoAction->setShortcut(QKeySequence::Undo);undoAction->setEnabled(false);
 auto* redoAction=new QAction("Refazer",this);
 redoAction->setShortcut(QKeySequence::Redo);redoAction->setEnabled(false);
 addAction(undoAction);addAction(redoAction);
 connect(undoAction,&QAction::triggered,this,[this]{canvas_->undo();});
 connect(redoAction,&QAction::triggered,this,[this]{canvas_->redo();});
 bar->addSeparator();bar->addAction(undoAction);bar->addAction(redoAction);
 auto* edit=menuBar()->addMenu("&Editar");edit->addAction(undoAction);edit->addAction(redoAction);
 auto* file=menuBar()->addMenu("&Arquivo");file->addAction(open);file->addAction(save);file->addSeparator();file->addAction("Sair",this,&QWidget::close);
 auto* view=menuBar()->addMenu("&Exibir");
 auto* help=menuBar()->addMenu("&Ajuda");
 help->addAction("Sobre",this,[this]{QMessageBox::about(this,"GoldSrc Studio","GoldSrc Studio — Qt 6 / C++20\nFerramentas de textura para GoldSrc.\nCompilação de desenvolvimento.");});
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
 auto* eyebrow=new QLabel("TEXTURE LAB  /  GOLDSRC ENGINE");eyebrow->setObjectName("eyebrow");titleColumn->addWidget(eyebrow);
 auto* title=new QLabel("Editor de texturas");title->setObjectName("pageTitle");titleColumn->addWidget(title);
 header->addLayout(titleColumn);header->addStretch();
 auto* engine=new QLabel("●  GOLD SRC     WAD3 · 8-BIT");engine->setObjectName("chip");header->addWidget(engine);cl->addLayout(header);
 auto* toolpanel=new QFrame;toolpanel->setObjectName("toolPanel");auto* tools=new QHBoxLayout(toolpanel);tools->setContentsMargins(12,9,12,9);tools->setSpacing(10);
 tools->addWidget(new QLabel("FERRAMENTA"));
 auto* mode=new QComboBox;mode->addItems({"Pincel","Borracha","Conta-gotas","Preenchimento"});mode->setMinimumWidth(155);tools->addWidget(mode);
 colorButton_=new QPushButton("●  Cor");colorButton_->setObjectName("colorButton");tools->addWidget(colorButton_);
 auto* toolSeparator=new QFrame;toolSeparator->setFrameShape(QFrame::VLine);toolSeparator->setObjectName("toolDivider");tools->addWidget(toolSeparator);
 tools->addWidget(new QLabel("ZOOM"));
 auto* zoom=new QSpinBox;zoom->setRange(1,32);zoom->setValue(4);zoom->setSuffix("×");zoom->setToolTip("Escala de 1 a 32 vezes");zoom->setFixedWidth(80);tools->addWidget(zoom);
 auto* grid=new QCheckBox("Grade de pixels");grid->setChecked(true);tools->addWidget(grid);tools->addStretch();
 cl->addWidget(toolpanel);
 auto* stage=new QFrame;stage->setObjectName("workspaceStage");auto* stageLayout=new QVBoxLayout(stage);stageLayout->setContentsMargins(0,0,0,0);
 auto* scroll=new QScrollArea;scroll->setObjectName("canvasScroll");scroll->setWidgetResizable(false);scroll->setAlignment(Qt::AlignCenter);scroll->setFrameShape(QFrame::NoFrame);
 canvas_=new PixelCanvas;canvas_->resize(320,320);scroll->setWidget(canvas_);
 stack_=new QStackedWidget;stack_->setObjectName("canvasStack");
 auto* empty=new QWidget;empty->setObjectName("emptyPage");
 auto* emptyLayout=new QVBoxLayout(empty);emptyLayout->setAlignment(Qt::AlignCenter);emptyLayout->setSpacing(13);
 auto* emptySymbol=new QLabel("◇");emptySymbol->setObjectName("emptySymbol");emptySymbol->setAlignment(Qt::AlignCenter);emptyLayout->addWidget(emptySymbol);
 auto* emptyTitle=new QLabel("Seu espaço criativo começa aqui");emptyTitle->setObjectName("emptyTitle");emptyTitle->setAlignment(Qt::AlignCenter);emptyLayout->addWidget(emptyTitle);
 auto* emptyDescription=new QLabel("Abra um pacote WAD3 existente ou importe imagens para criar suas texturas GoldSrc.");emptyDescription->setObjectName("emptyDescription");emptyDescription->setWordWrap(true);emptyDescription->setAlignment(Qt::AlignCenter);emptyDescription->setMaximumWidth(430);emptyLayout->addWidget(emptyDescription,0,Qt::AlignHCenter);
 auto* emptyButtons=new QHBoxLayout;emptyButtons->setAlignment(Qt::AlignCenter);emptyButtons->setSpacing(12);
 auto* emptyOpen=new QPushButton("Abrir WAD3");emptyOpen->setObjectName("primaryButton");emptyButtons->addWidget(emptyOpen);
 auto* emptyImport=new QPushButton("Importar imagens");emptyImport->setObjectName("ghostButton");emptyButtons->addWidget(emptyImport);
 emptyLayout->addSpacing(5);emptyLayout->addLayout(emptyButtons);
 connect(emptyOpen,&QPushButton::clicked,this,[this]{openWad();});
 connect(emptyImport,&QPushButton::clicked,this,[this]{importImages();});
 stack_->addWidget(empty);stack_->addWidget(scroll);stack_->setCurrentIndex(0);stageLayout->addWidget(stack_,1);
 cl->addWidget(stage,1);
 auto* footer=new QHBoxLayout;
 summary_=new QLabel("Nenhuma textura carregada");summary_->setObjectName("workspaceFooter");footer->addWidget(summary_);
 footer->addStretch();auto* scaleHint=new QLabel("Ctrl+Z: desfazer  •  Ctrl+Y: refazer  •  Até 15 alterações por textura");scaleHint->setObjectName("workspaceFooter");footer->addWidget(scaleHint);
 cl->addLayout(footer);setCentralWidget(center);
 connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){canvas_->setTool(static_cast<PixelCanvas::Tool>(index));});
 connect(zoom,qOverload<int>(&QSpinBox::valueChanged),canvas_,&PixelCanvas::setZoom);
 connect(grid,&QCheckBox::toggled,canvas_,&PixelCanvas::setGrid);
 connect(colorButton_,&QPushButton::clicked,this,[this]{
  QColor c=QColorDialog::getColor(canvas_->color(),this,"Escolher cor",QColorDialog::ShowAlphaChannel);
  if(c.isValid()) {canvas_->setColor(c);colorButton_->setStyleSheet(QString("background-color:%1;color:%2;").arg(c.name(),c.lightness()<128?"white":"black"));}
 });
 canvas_->historyChanged=[undoAction,redoAction](bool undo,bool redo){
  undoAction->setEnabled(undo);redoAction->setEnabled(redo);
 };
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
 qApp->setStyleSheet(R"QSS(
 QWidget{background:#111116;color:#eeeeF5;font-family:'Segoe UI',Arial;font-size:12px}
 QMainWindow{background:#0d0d12}
 QMenuBar{background:#15151d;padding:6px 13px;border-bottom:1px solid #282735}
 QMenuBar::item{padding:7px 12px;border-radius:6px}
 QMenuBar::item:selected{background:#332647;color:#efe3ff}
 QMenu{background:#1b1b25;border:1px solid #383347;border-radius:7px;padding:6px}
 QMenu::item{padding:8px 16px}
 QMenu::item:selected{background:#7248bc;color:white}
 QToolBar#mainToolbar{background:#16161f;border-bottom:1px solid #292837;padding:12px 19px;spacing:10px}
 QToolBar#mainToolbar QToolButton{background:#24232f;color:#e9e6ef;padding:10px 13px;border:1px solid #353247;border-radius:9px;font-weight:600}
 QToolBar#mainToolbar QToolButton:hover{background:#382c50;border-color:#9260ea}
 QToolBar#mainToolbar QToolButton:pressed{background:#67429f}
 QToolBar::separator{background:#383341;width:1px;margin:7px 9px}
 QDockWidget{background:#16161e;color:#eae7ef;border:0}
 QDockWidget::title{background:#1e1d29;border-bottom:1px solid #343042;padding:13px;font-weight:600;color:#c5b7dd}
 QDockWidget::close-button,QDockWidget::float-button{border:0}
 QLabel#eyebrow{color:#ad8cf0;font-size:10px;font-weight:bold;letter-spacing:1.5px}
 QLabel#pageTitle{font-size:26px;font-weight:700;color:#faf9fd}
 QLabel#chip{background:#242033;border:1px solid #433554;border-radius:14px;padding:9px 13px;color:#c6a5ff;font-weight:600}
 QLineEdit,QSpinBox,QComboBox{background:#23222d;color:#f0edf7;border:1px solid #393649;border-radius:9px;padding:9px;selection-background-color:#8350cd}
 QLineEdit:focus,QSpinBox:focus,QComboBox:focus{border:1px solid #9b71ed}
 QLineEdit:disabled,QSpinBox:disabled{color:#65616f;background:#1d1c26}
 QComboBox::drop-down{border:0;width:20px}
 QListWidget{background:#16161e;border:1px solid #302e3c;border-radius:12px;padding:8px}
 QListWidget::item{padding:10px 8px;margin:2px 0;border-radius:9px}
 QListWidget::item:selected{background:#362848;color:#eee0ff;border:1px solid #7d54bf}
 QListWidget::item:hover{background:#252231}
 QFrame#toolPanel{background:#1b1a25;border:1px solid #353142;border-radius:12px}
 QFrame#toolDivider{background:#3a354a;max-width:1px}
 QFrame#workspaceStage{background:#14141c;border:1px solid #333044;border-radius:13px}
 QScrollArea#canvasScroll{background:#14141c;border:0}
 QWidget#emptyPage{background:#14141c}
 QLabel#emptySymbol{font-size:70px;color:#9569df;background:transparent}
 QLabel#emptyTitle{font-size:24px;font-weight:700;color:#eeebf6;background:transparent}
 QLabel#emptyDescription{font-size:13px;color:#a9a1b8;background:transparent}
 QGroupBox{border:1px solid #343044;border-radius:10px;margin-top:16px;padding:14px 8px 10px;font-weight:600}
 QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#cec3e2}
 QPushButton{background:#7850bd;color:#fff;border:1px solid #926bde;border-radius:9px;padding:11px 13px;font-weight:600}
 QPushButton:hover{background:#9467e2;border-color:#bb98ff}
 QPushButton:pressed{background:#624096}
 QPushButton#ghostButton{background:#262433;border:1px solid #40374f;color:#e1d9ed}
 QPushButton#ghostButton:hover{background:#332a44;border-color:#8260ae}
 QPushButton#primaryButton{background:#8657d5;border:1px solid #a078ed}
 QPushButton#colorButton{background:#7651bd}
 QCheckBox{spacing:7px;color:#c9c1d4}
 QLabel#smallNote{color:#a6a0b6;font-size:12px}
 QLabel#workspaceFooter{color:#88839d;font-size:11px}
 QStatusBar{background:#181721;color:#a49bb6;border-top:1px solid #353042;padding:5px}
 QScrollBar:vertical{background:#1a1923;width:11px}
 QScrollBar::handle:vertical{background:#494054;border-radius:5px;min-height:30px}
 QScrollBar:horizontal{background:#1a1923;height:11px}
 QScrollBar::handle:horizontal{background:#494054;border-radius:5px;min-width:30px}
 )QSS");
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
 if(!valid){stack_->setCurrentIndex(0);canvas_->setImage(QImage());name_->clear();summary_->setText("Nenhuma textura selecionada");updating_=false;return;}
 stack_->setCurrentIndex(1);const auto& t=textures_.at(index);name_->setText(t.name);width_->setValue(t.image.width());height_->setValue(t.image.height());
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
