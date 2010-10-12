/*
 * This file is part of Maemo 5 Office UI for KOffice
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Boudewijn Rempt <boud@kogmbh.com>
 *
 * Contact: Manikandaprasad N C <manikandaprasad.chandrasekar@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Common.h"
#include "ZoomDialog.h"
#include "HildonMenu.h"
#include "NotifyDialog.h"
#include "AboutDialog.h"
#include "PresentationTool.h"
#include "MainWindowAdaptor.h"
#include "FoCellToolFactory.h"
#include "FoExternalEditor.h"
#include "FoImageSelectionWidget.h"
#ifdef HAVE_OPENGL
#include "GlPresenter.h"
#endif
#include "FoCellTool.h"
#include "VirtualKeyBoard.h"
#include "ConfirmationDialog.h"
#include "PreviewDialog.h"

#include <QFileDialog>
#include <QUrl>
#include <QDebug>
#include <QLineEdit>
#include <QCheckBox>
#include <QScrollBar>
#include <QTimer>
#include <QIcon>
#include <QPushButton>
#include <QSize>
#include <QTextDocument>
#include <QTextCursor>
#include <QPair>
#include <QMenuBar>
#include <QX11Info>
#include <QShortcut>
#include <QProcess>
#include <QAction>
#include <QLabel>
#include <QTextBlock>
#include <QTextList>
#include <QGridLayout>
#include <QDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QFontComboBox>
#include <QColor>
#include <QColorDialog>
#include <QFrame>
#include <QPalette>
#include <QListWidget>

#ifdef Q_WS_MAEMO_5
#include <QtMaemo5/QMaemo5InformationBox>
#include "Accelerator.h"
#endif

#include <kfileitem.h>

#include <KoView.h>
#include <KoCanvasBase.h>
#include <kdemacros.h>
#include <KoCanvasControllerWidget.h>
#include <KoZoomMode.h>
#include <KoZoomController.h>
#include <KoToolProxy.h>
#include <KoToolBase.h>
#include <KoResourceManager.h>
#include <KoToolManager.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoShapeUserData.h>
#include <KoSelection.h>
#include <KoPADocument.h>
#include <KoPAPageBase.h>
#include <KoPACanvasBase.h>
#include <KoTextEditor.h>
#include <KoTextDocument.h>
#include <KoTextShapeData.h>
#include <KoPAView.h>
#include <KoStore.h>
#include <KoCanvasBase.h>
#include <KoToolRegistry.h>
#include <KoShapeCreateCommand.h>
#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>
#include <KWDocument.h>
#include <KWView.h>
#include <KoShapeLayer.h>
#include <styles/KoParagraphStyle.h>
#include <styles/KoListLevelProperties.h>
#include <KoList.h>
#include <kundostack.h>
#include <Map.h>
#include <Doc.h>
#include <part/View.h>
#include <Sheet.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
//#define Q_WS_MAEMO_5 1

#define FORMATRAME_XCORDINATE_VALUE 465
#define FORMATFRAME_YCORDINATE_VALUE 150
#define FORMATFRAME_WIDTH 335
#define FORMATFRAME_HEIGHT 210

#define FONTSTYLEFRAME_XCORDINATE_VALUE 280
#define FONTSTYLEFRAME_YCORDINATE_VALUE 150
#define FONTSTYLEFRAME_WIDTH 520
#define FONTSTYLEFRAME_HEIGHT 210

#define SHEETINFOFRAME_XCORDINATE_VALUE 100
#define SHEETINFOFRAME_YCORDINATE_VALUE 290
#define SHEETINFOFRAME_WIDTH 520
#define SHEETINFOFRAME_HEIGHT 70


#ifdef Q_WS_MAEMO_5

bool MainWindow::enableAccelerator=false;
bool MainWindow::enableScrolling=false;
bool MainWindow::stopAcceleratorScrolling=true;
#endif

MainWindow::MainWindow(Splash *aSplash, QWidget *parent)
        : KoAbstractApplication(parent),
        m_xcordinate(0),
        m_ycordinate(0),
        m_fontSizeDialog(0),
        m_fontSizeDialogLayout(0),
        m_fontSizeLineEdit(0),
        m_fontSizeList(0),
        m_formatframe(0),
        m_formatframelayout(0),
        m_alignleft(0),
        m_alignright(0),
        m_aligncenter(0),
        m_numberedlist(0),
        m_bulletlist(0),
        m_alignjustify(0),
        m_fontstyleframe(0),
        m_fontstyleframelayout(0),
        m_fontcombobox(0),
        m_fontsizebutton(0),
        m_textcolor(0),
        m_textbackgroundcolor(0),
        m_superscript(0),
        m_subscript(0),
        m_bold(0),
        m_italic(0),
        m_underline(0),
        m_docdialog(0),
        m_docdialoglayout(0),
        m_templateWidget(0),
        m_addAction(0),
        m_subtractAction(0),
        m_multiplyAction(0),
        m_divideAction(0),
        m_percentageAction(0),
        m_equalsAction(0),
        m_signalMapper(0),
        m_sheetInfoFrame(0),
        m_sheetInfoFrameLayout(0),
        m_addSheet(0),
        m_removeSheet(0),
        m_sheetName(0),
        m_search(0),
        m_vPage(0),
        m_hPage(0),
        m_pressed(false),
        m_isViewToolBar(true),
        m_fsTimer(0),
        m_fsButton(0),
        m_fsIcon(FS_BUTTON_PATH),
        m_fsPPTBackButton(0),
        m_fsPPTForwardButton(0),
        m_fsPPTDrawPenButton(0),
        m_fsPPTDrawHighlightButton(0),
        m_dbus( new MainWindowAdaptor(this)),
        m_panningCount(0),
        m_slideNotesButton(0),
        m_slideNotesIcon(VIEW_NOTES_PIXMAP)
#ifdef Q_WS_MAEMO_5
        ,m_foDocumentRdf(0),
        m_rdfShortcut(0)
#endif
{
    setSplashScreen(aSplash);
    m_digitalSignatureDialog = 0;
    m_closetemp = 0;
    m_collab = 0;
    m_collabDialog = 0;
    m_collabEditor = 0;
    m_document = 0;
    m_go = 0;
    m_presenter = 0;
    m_slidingmotiondialog = 0;
    m_spreadEditToolBar = 0;
    m_spreadsheet = 0;
    m_tempdialoglayout = 0;
    m_templatepreview = 0;
    m_tempselectiondialog = 0;
    m_ui = new Ui::MainWindow;
    m_notesDialog = 0;
#ifdef Q_WS_MAEMO_5
    m_fsAccButton = 0;
#endif
    m_virtualKeyBoard = 0;
    init();
}

void MainWindow::init()
{
    m_ui->setupUi(this);

    QDBusConnection::sessionBus().registerObject("/presentation/view", this);
    m_shortcutForVirtualKeyBoard = new QShortcut(QKeySequence(("Ctrl+K")),this);
    Q_CHECK_PTR(m_shortcutForVirtualKeyBoard);
    m_shortcutForVirtualKeyBoard->setEnabled(true);
    connect(m_shortcutForVirtualKeyBoard, SIGNAL(activated()), this, SLOT(toggleVirtualKeyboardVisibility()));

    m_spaceHandlerShortcutForVirtualKeyBoard = new QShortcut(Qt::Key_Space,this);
    Q_CHECK_PTR(m_spaceHandlerShortcutForVirtualKeyBoard);
    m_spaceHandlerShortcutForVirtualKeyBoard->setEnabled(true);
    connect(m_spaceHandlerShortcutForVirtualKeyBoard, SIGNAL(activated()), this, SLOT(spaceHandlerForVirtualKeyboard()));

#ifdef Q_WS_MAEMO_5
    shortcutForAccelerator = new QShortcut(QKeySequence(("Ctrl+G")),this);
    Q_CHECK_PTR(shortcutForAccelerator);
    shortcutForAccelerator->setEnabled(true);
#endif

    QMenuBar* menu = menuBar();
    menu->addAction(m_ui->actionOpen);
    menu->addAction(m_ui->actionNew);
    menu->addAction(m_ui->actionSave);
    menu->addAction(m_ui->actionSaveAs);
    menu->addAction(m_ui->actionPresentation);
    menu->addAction(m_ui->actionClose);
    menu->addAction(m_ui->actionAbout);
   // menu->addAction(m_ui->actionCollaborate);
    // false here means that they are not plugins
    m_ui->actionOpen->setData(QVariant(false));
    m_ui->actionAbout->setData(QVariant(false));
    m_ui->actionNew->setData(QVariant(false));
    m_ui->actionSave->setData(QVariant(false));
    m_ui->actionSaveAs->setData(QVariant(false));
    m_ui->actionClose->setData(QVariant(false));
    m_ui->actionPresentation->setData(QVariant(false));
    const QDir pluginDir("/usr/lib/freoffice/");
    const QStringList plugins = pluginDir.entryList(QDir::Files);

    for (int i = 0; i < plugins.size(); ++i) {
        QPluginLoader test(pluginDir.absoluteFilePath(plugins.at(i)));
        QObject *plug = test.instance();
        plug->setParent(this);
        if (plug != 0)
        {
            OfficeInterface* inter = qobject_cast<OfficeInterface*>(plug);
            const QString plugName = inter->pluginName();
            loadedPlugins[plugName] = inter;
            connect(plug, SIGNAL(openDocument(bool, const QString&)), this, SLOT(pluginOpen(bool, const QString&)));
            QAction *action = new QAction(inter->pluginTitle(), this);

            // True states that this action is a plugin
            action->setData(QVariant(true));
            menu->addAction(action);
        }
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(menuClicked(QAction*)));
    m_search = new QLineEdit(this);
    m_search->setInputMethodHints(Qt::ImhNoAutoUppercase);
    m_ui->SearchToolBar->insertWidget(m_ui->actionSearchOption, m_search);
    m_exactMatchCheckBox = new QCheckBox(i18n("Exact Match"), this);
    m_ui->SearchToolBar->insertWidget(m_ui->actionSearchOption, m_exactMatchCheckBox);
    m_ui->SearchToolBar->hide();

    connect(m_ui->actionNew,SIGNAL(triggered()),this,SLOT(chooseDocumentType()));
    connect(m_ui->actionSave, SIGNAL(triggered()), this, SLOT(saveDocument()));
    connect(m_ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveDocumentAs()));
    connect(m_ui->actionClose,SIGNAL(triggered()), this, SLOT(closeDocument()));
    connect(m_ui->actionFormat,SIGNAL(triggered()),SLOT(openFormatFrame()));
    connect(m_ui->actionStyle,SIGNAL(triggered()),SLOT(openFontStyleFrame()));
    m_ui->actionMathOp->setCheckable(true);
#ifdef Q_WS_MAEMO_5
    connect(m_ui->actionClose,SIGNAL(triggered()),this,SLOT(closeAcceleratorSettings()));
#endif

    connect(m_ui->actionMathOp,SIGNAL(toggled(bool)),this,SLOT(startMathMode(bool)));
#ifdef HAVE_OPENGL
    connect(m_ui->actionPresentation,SIGNAL(triggered()),this,SLOT(glPresenter()));
#endif

    connect(m_search, SIGNAL(returnPressed()), SLOT(goToNextWord()));
    connect(m_search, SIGNAL(textEdited(QString)), SLOT(startSearch()));

    connect(m_ui->actionEdit, SIGNAL(toggled(bool)), this, SLOT(setEditingMode(bool)));
    connect(m_ui->actionSearch, SIGNAL(toggled(bool)), this, SLOT(toggleToolBar(bool)));

    connect(m_ui->actionUndo,SIGNAL(triggered()),this,SLOT(doUndo()));
    connect(m_ui->actionRedo,SIGNAL(triggered()),this,SLOT(doRedo()));

    connect(m_ui->actionCopy,SIGNAL(triggered()),this,SLOT(copy()));
    connect(m_ui->actionCut,SIGNAL(triggered()),this,SLOT(cut()));
    connect(m_ui->actionPaste,SIGNAL(triggered()),this,SLOT(paste()));

    connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(openDocument()));
    connect(m_ui->actionAbout, SIGNAL(triggered()), this, SLOT(openAboutDialog()));
    connect(m_ui->actionFullScreen, SIGNAL(triggered()), this, SLOT(fullScreen()));
    connect(m_ui->actionSlidingMotion, SIGNAL(triggered()), this, SLOT(slideTransitionDialog()));
    connect(m_ui->actionCollaborate, SIGNAL(triggered()), this, SLOT(collaborateDialog()));

    m_ui->actionZoomIn->setShortcuts(QKeySequence::ZoomIn);
    m_ui->actionZoomIn->setShortcutContext(Qt::ApplicationShortcut);
    m_ui->actionZoomOut->setShortcuts(QKeySequence::ZoomOut);
    m_ui->actionZoomOut->setShortcutContext(Qt::ApplicationShortcut);
    connect(m_ui->actionZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));
    connect(m_ui->actionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));
    connect(m_ui->actionZoomLevel, SIGNAL(triggered()), this, SLOT(zoom()));

    connect(m_ui->actionNextPage, SIGNAL(triggered()), this, SLOT(goToNextPage()));
    connect(m_ui->actionPrevPage, SIGNAL(triggered()), this, SLOT(goToPreviousPage()));

    connect(m_ui->actionPrevWord, SIGNAL(triggered()), this, SLOT(goToPreviousWord()));
    connect(m_ui->actionNextWord, SIGNAL(triggered()), this, SLOT(goToNextWord()));
    connect(m_exactMatchCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(searchOptionChanged(int)));

    m_fsTimer = new QTimer(this);
    Q_CHECK_PTR(m_fsTimer);
    connect(m_fsTimer, SIGNAL(timeout()), this, SLOT(fsTimer()));

    m_fsButton = new QPushButton(this);
    Q_CHECK_PTR(m_fsButton);
    m_fsButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
    m_fsButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
    m_fsButton->setIcon(m_fsIcon);
    m_fsButton->hide();
    connect(m_fsButton, SIGNAL(clicked()), SLOT(fsButtonClicked()));
    qApp->installEventFilter(this);

    updateActions();

    /* taking care of Zoom buttons : starts */
    unsigned long val = 1;
    Atom atom = XInternAtom(QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False);
    XChangeProperty(QX11Info::display(), winId(), atom, XA_INTEGER, 32,
                    PropModeReplace,
                    (unsigned char *) &val, 1);
    /* taking care of Zoom buttons : ends */

    m_cutShortcut = new QShortcut(QKeySequence::Cut, this);
    connect(m_cutShortcut, SIGNAL(activated()), this, SLOT(cut()));

    m_copyShortcut = new QShortcut(QKeySequence::Copy, this);
    connect(m_copyShortcut, SIGNAL(activated()), this, SLOT(copy()));

    m_pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    connect(m_pasteShortcut, SIGNAL(activated()), this, SLOT(paste()));

    m_undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(m_undoShortcut,SIGNAL(activated()),this,SLOT(doUndo()));

    m_redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(m_redoShortcut,SIGNAL(activated()),this,SLOT(doRedo()));

    // Toolbar should be shown only when we open a document
    m_ui->viewToolBar->hide();
    m_ui->EditToolBar->hide();
    connect(m_ui->actionPageNumber,SIGNAL(triggered()),this,SLOT(showPreviewDialog()));
    m_viewNumber = 0;
    /*
     * Default show time is 7000 milli seconds
     */
    gl_showtime=7000;
    gl_style=0;

    connect(m_ui->actionSelect, SIGNAL(triggered()), this, SLOT(enableSelectTool()));
    connect(m_ui->actionShowCCP, SIGNAL(triggered()), this, SLOT(showCCP()));
    connect(m_ui->actionInsert, SIGNAL(triggered()), this, SLOT(insertButtonClicked()));
    connect(m_ui->actionInsertImage, SIGNAL(triggered()), this, SLOT(insertImage()));
    connect(m_ui->actionInsertTextShape, SIGNAL(triggered()), this, SLOT(insertNewTextShape()));
    connect(m_ui->actionInsertTable, SIGNAL(triggered()), this, SLOT(insertNewTable()));
    m_ui->actionInsertImage->setVisible(false);
    m_ui->actionInsertTextShape->setVisible(false);
    m_ui->actionInsertTable->setVisible(false);
    showCCP();
    insertButtonClicked();
}

MainWindow::~MainWindow()
{
    QMapIterator<QString, OfficeInterface*> i(loadedPlugins);
    while (i.hasNext())
    {
        i.next();
        delete i.value();
    }
    delete m_ui;
    m_ui = 0;
    delete m_collab;
    delete m_collabDialog;
    delete m_collabEditor;
    delete m_virtualKeyBoard;
}

void MainWindow::spaceHandlerForVirtualKeyboard()
{
    textEditor()->insertText(" ");
}

void MainWindow::setVirtualKeyboardVisible(bool set)
{
    if (set && !m_virtualKeyBoard) {
        m_virtualKeyBoard = new VirtualKeyBoard;
    }
    if (m_virtualKeyBoard)
        m_virtualKeyBoard->ShowVirtualKeyBoard(this, textEditor()); // switches
}

bool MainWindow::isVirtualKeyboardVisible() const
{
    return m_virtualKeyBoard && m_virtualKeyBoard->isVisible();
}

void MainWindow::showPreviewDialog()
{
    if (documentType() == PresentationDocument) {
        storeButtonPreview()->showDialog(currentPage());
    }
    //add spreadsheet sheet information frame
    if (documentType() == SpreadsheetDocument) {
        spreadSheetInfo();
    }
}

void MainWindow::spreadSheetInfo()
{
    if(m_sheetInfoFrame) {
        m_sheetInfoFrame->hide();
        disconnect(m_addSheet,SIGNAL(clicked()),this,SLOT(addSheet()));
        disconnect(m_removeSheet,SIGNAL(clicked()),this,SLOT(removeSheet()));
        delete m_sheetInfoFrame;
        m_sheetInfoFrame=0;
        m_sheetInfoFrameLayout=0;
        m_addSheet=0;
        m_removeSheet=0;
        m_sheetName=0;
        return;
    }
    m_sheetInfoFrame=new QFrame(this);
    m_addSheet=new QPushButton(QString("+"), this);
    m_removeSheet=new QPushButton(QString("-"), this);
    m_sheetName=new QPushButton(this);

    m_sheetInfoFrameLayout=new QGridLayout(m_sheetInfoFrame);
    m_sheetInfoFrameLayout->addWidget(m_removeSheet,0,0);
    m_sheetInfoFrameLayout->addWidget(m_sheetName,0,2,0,2);
    m_sheetInfoFrameLayout->addWidget(m_addSheet,0,5);
    m_sheetInfoFrameLayout->setHorizontalSpacing(0);
    m_sheetInfoFrameLayout->setVerticalSpacing(0);

    m_sheetInfoFrame->setGeometry(SHEETINFOFRAME_XCORDINATE_VALUE,
                               SHEETINFOFRAME_YCORDINATE_VALUE,
                               SHEETINFOFRAME_WIDTH,
                               SHEETINFOFRAME_HEIGHT);

    m_sheetInfoFrame->setLayout(m_sheetInfoFrameLayout);
    m_sheetInfoFrame->show();
    connect(m_addSheet,SIGNAL(clicked()),this,SLOT(addSheet()));
    connect(m_removeSheet,SIGNAL(clicked()),this,SLOT(removeSheet()));
    m_sheetName->setText(currentSheetName());
}

void MainWindow::openFormatFrame()
{
    if(m_fontstyleframe)
        m_fontstyleframe->hide();

    if(m_formatframe && m_formatframe->isVisible()) {
        m_formatframe->hide();
        return;
    } else if(m_formatframe){
        if (documentType() == TextDocument)
            activeFormatOptionCheck();
        m_formatframe->show();
        return;
    }

    m_formatframe=new QFrame(this);
    Q_CHECK_PTR(m_formatframe);
    m_formatframe->setFrameStyle(QFrame::Sunken);

    m_formatframelayout = new QGridLayout;
    Q_CHECK_PTR(m_formatframelayout);
    m_formatframelayout->setVerticalSpacing(0);
    m_formatframelayout->setHorizontalSpacing(0);

    m_alignleft=addFormatFrameComponent(i18n("AlignLeft"));
    m_alignright=addFormatFrameComponent(i18n("AlignRight"));
    m_aligncenter=addFormatFrameComponent(i18n("AlignCenter"));
    m_bulletlist=addFormatFrameComponent(i18n("Bullets"));
    m_numberedlist=addFormatFrameComponent(i18n("Number"));
    m_alignjustify=addFormatFrameComponent(i18n("AlignJustify"));

    m_bulletlist->setCheckable(false);
    m_numberedlist->setCheckable(false);

    m_formatframelayout->addWidget(m_alignleft,0,0);
    m_formatframelayout->addWidget(m_alignright,0,1);
    m_formatframelayout->addWidget(m_aligncenter,1,0);
    m_formatframelayout->addWidget(m_alignjustify,1,1);
    m_formatframelayout->addWidget(m_numberedlist,2,0);
    m_formatframelayout->addWidget(m_bulletlist,2,1);

    m_formatframe->setGeometry(FORMATRAME_XCORDINATE_VALUE,
                               FORMATFRAME_YCORDINATE_VALUE,
                               FORMATFRAME_WIDTH,
                               FORMATFRAME_HEIGHT);
    m_formatframe->setLayout(m_formatframelayout);
    if (documentType() == TextDocument)
        activeFormatOptionCheck();
    m_formatframe->show();

    connect(m_alignjustify,SIGNAL(clicked()),this,SLOT(doJustify()));
    connect(m_alignleft,SIGNAL(clicked()),this,SLOT(doLeftAlignment()));
    connect(m_alignright,SIGNAL(clicked()),this,SLOT(doRightAlignment()));
    connect(m_aligncenter,SIGNAL(clicked()),this,SLOT(doCenterAlignment()));
    connect(m_numberedlist,SIGNAL(clicked()),this,SLOT(doNumberList()));
    connect(m_bulletlist,SIGNAL(clicked()),this,SLOT(doBulletList()));
}

void MainWindow::openFontStyleFrame()
{
    if(m_formatframe)
        m_formatframe->hide();

    if(m_fontstyleframe && m_fontstyleframe->isVisible()) {
        m_fontstyleframe->hide();
        return;
    } else if(m_fontstyleframe){
        if (documentType() == TextDocument || documentType() == SpreadsheetDocument)
            activeFontOptionCheck();
        m_fontstyleframe->show();
        return;
    }
    m_fontstyleframe=new QFrame(this);
    Q_CHECK_PTR(m_fontstyleframe);

    m_fontstyleframelayout=new QGridLayout;
    Q_CHECK_PTR(m_fontstyleframelayout);
    m_fontstyleframelayout->setVerticalSpacing(0);
    m_fontstyleframelayout->setHorizontalSpacing(0);

    m_fontsizebutton=new QPushButton(this);
    Q_CHECK_PTR(m_fontsizebutton);
    m_fontsizebutton->setMinimumSize(100,73);

    m_bold=addFontStyleFrameComponent(i18n("Bold"));
    m_italic=addFontStyleFrameComponent(i18n("Italic"));
    m_underline=addFontStyleFrameComponent(i18n("UnderLine"));
    m_textcolor=addFontStyleFrameComponent(i18n("TextColor"));
    m_textbackgroundcolor=addFontStyleFrameComponent(i18n("TextBackgroundColor"));
    m_subscript=addFontStyleFrameComponent(i18n("SubScript"));
    m_superscript=addFontStyleFrameComponent(i18n("SuperScript"));

    m_textcolor->setCheckable(false);
    m_textbackgroundcolor->setCheckable(false);

    m_fontcombobox=new QFontComboBox(this);
    Q_CHECK_PTR(m_fontcombobox);
    m_fontcombobox->setMinimumSize(230,73);
    m_fontcombobox->setFont(QFont("Nokia Sans",20,QFont::Normal));
    m_fontcombobox->setEditable(false);

    m_fontstyleframelayout->addWidget(m_fontcombobox,0,0,1,2);
    m_fontstyleframelayout->addWidget(m_fontsizebutton,0,2);
    m_fontstyleframelayout->addWidget(m_bold,0,3,1,2);
    m_fontstyleframelayout->addWidget(m_textcolor,1,0);
    m_fontstyleframelayout->addWidget(m_textbackgroundcolor,1,1,1,3);
    m_fontstyleframelayout->addWidget(m_italic,1,4);
    m_fontstyleframelayout->addWidget(m_superscript,2,0);
    m_fontstyleframelayout->addWidget(m_subscript,2,1,1,3);
    m_fontstyleframelayout->addWidget(m_underline,2,4);

    m_fontstyleframe->setLayout(m_fontstyleframelayout);
    m_fontstyleframe->setGeometry(FONTSTYLEFRAME_XCORDINATE_VALUE,
                             FONTSTYLEFRAME_YCORDINATE_VALUE,
                             FONTSTYLEFRAME_WIDTH,
                             FONTSTYLEFRAME_HEIGHT);
    if (documentType() == TextDocument || documentType() == SpreadsheetDocument)
        activeFontOptionCheck();
    m_fontstyleframe->show();

    connect(m_fontsizebutton,SIGNAL(clicked()),SLOT(showFontSizeDialog()));
    connect(m_fontcombobox,SIGNAL(activated(int)),SLOT(selectFontType()));
    connect(m_textcolor,SIGNAL(clicked()),SLOT(selectTextColor()));
    connect(m_textbackgroundcolor,SIGNAL(clicked()),SLOT(selectTextBackGroundColor()));
    connect(m_subscript,SIGNAL(clicked()),SLOT(doSubScript()));
    connect(m_superscript,SIGNAL(clicked()),SLOT(doSuperScript()));
    connect(m_bold,SIGNAL(clicked()),this,SLOT(doBold()));
    connect(m_italic,SIGNAL(clicked()),this,SLOT(doItalic()));
    connect(m_underline,SIGNAL(clicked()),this,SLOT(doUnderLine()));
}

void  MainWindow::showFontSizeDialog()
{
    if (m_fontstyleframe) {
        m_fontstyleframe->hide();
    }
    m_fontSizeDialog = new QDialog(this);
    Q_ASSERT(m_fontSizeDialog);
    m_fontSizeLineEdit=new QLineEdit(m_fontSizeDialog);
    Q_ASSERT(m_fontSizeLineEdit);
    m_fontSizeList= new QListWidget(m_fontSizeDialog);
    Q_ASSERT(m_fontSizeList);
    m_fontSizeDialogLayout= new QVBoxLayout();
    m_fontSizeDialogLayout->addWidget(m_fontSizeLineEdit);
    m_fontSizeDialogLayout->addWidget(m_fontSizeList);
    m_fontSizeDialog->setWindowTitle(i18n("Font Size"));
    m_fontSizeDialog->setLayout(m_fontSizeDialogLayout);
    m_fontSizeLineEdit->setInputMethodHints(Qt::ImhDigitsOnly);

    int i;
    for(i=4;i<=40;i++)
    {
        QString f_size;
        m_fontSizeList->addItem(f_size.setNum(i));
    }
    int currentFont= m_fontsizebutton->text().toInt();
    if(currentFont>=4 && currentFont<=40) {
        m_fontSizeList->setCurrentRow(currentFont-4);
    }

    m_fontSizeLineEdit->setText(m_fontsizebutton->text());
    m_fontSizeLineEdit->selectAll();
    connect(m_fontSizeList,SIGNAL(itemDoubleClicked (QListWidgetItem *)),SLOT(fontSizeRowSelected( QListWidgetItem * )));
    connect(m_fontSizeLineEdit,SIGNAL(returnPressed()),SLOT(fontSizeEntered()));
    m_fontSizeDialog->exec();
}

void MainWindow::fontSizeRowSelected(QListWidgetItem *item)
{
     int row=(item->text()).toInt();
     selectFontSize(row+4);
     m_fontSizeDialog->accept();
}

void MainWindow::fontSizeEntered()
{
    selectFontSize(m_fontSizeLineEdit->text().toInt());
}

void MainWindow:: openMathOpFrame() {

}

void MainWindow::addMathematicalOperator(QString mathSymbol)
{
    Q_ASSERT(cellTool()->externalEditor());
    cellTool()->externalEditor()->insertOperator(mathSymbol);
}

///////////////////////
// Styling functions //
///////////////////////

void MainWindow::doSubScript()
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == PresentationDocument) {
           KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
           m_kopaview->kopaCanvas()->toolProxy()->actions()["format_sub"]->trigger();
    }
    if (setSubScript(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatSubScript);

    setDocumentModified(true);
}

bool MainWindow::setSubScript(KoTextEditor *editor) {
    if (editor){
        if (editor->charFormat().verticalAlignment() == QTextCharFormat::AlignSubScript ){
            editor->setVerticalTextAlignment(Qt::AlignVCenter);
        } else {
            editor->setVerticalTextAlignment(Qt::AlignBottom);
        }
        return true;
    }
    return false;
}

void MainWindow::doSuperScript()
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == PresentationDocument) {
           KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
           m_kopaview->kopaCanvas()->toolProxy()->actions()["format_super"]->trigger();
    }
    if (setSuperScript(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatSuperScript);

    setDocumentModified(true);
}

bool MainWindow::setSuperScript(KoTextEditor *editor) {
    if (editor) {
        if (editor->charFormat().verticalAlignment() == QTextCharFormat::AlignSuperScript ) {
            editor->setVerticalTextAlignment(Qt::AlignVCenter);
        } else {
            editor->setVerticalTextAlignment(Qt::AlignTop);
        }
        return true;
    }
    return false;
}

void MainWindow::selectFontSize(int size)
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == SpreadsheetDocument) {
        cellTool()->selectFontSize(size);
    } else {
        if (setFontSize(size, textEditor()) && m_collab)
            m_collab->sendFontSize(textEditor()->selectionStart(), textEditor()->selectionEnd(), size);
    }
    setDocumentModified(true);
    m_fontSizeDialog->hide();
}

bool MainWindow::setFontSize(int size, KoTextEditor *editor) {
    if (editor) {
        editor->setFontSize(size);
        return true;
    }
    return false;
}

void MainWindow::selectFontType()
{
    QString selectedfont=m_fontcombobox->currentText();
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == SpreadsheetDocument) {
        cellTool()->selectFontType(selectedfont);
    } else {
        if (setFontType(selectedfont, textEditor()) && m_collab)
            m_collab->sendFontType(textEditor()->selectionStart(), textEditor()->selectionEnd(), selectedfont);
    }
    setDocumentModified(true);
}

bool MainWindow::setFontType(const QString &font, KoTextEditor *editor) {
    if (editor) {
        editor->setFontFamily(font);
        return true;
    }
    return false;
}

void MainWindow::selectTextColor()
{
    QColor color = QColorDialog::getColor(Qt::white,this);
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == SpreadsheetDocument) {
        cellTool()->selectTextColor(color);
    } else {
        if (setTextColor(color, textEditor()) && m_collab)
            m_collab->sendTextColor(textEditor()->selectionStart(), textEditor()->selectionEnd(), color.rgb());
    }
    setDocumentModified(true);
}

bool MainWindow::setTextColor(const QColor &color, KoTextEditor *editor) {
    if (editor) {
        editor->setTextColor(color);
        return true;
    }
    return false;
}

void MainWindow::selectTextBackGroundColor()
{
    QColor color = QColorDialog::getColor(Qt::white,this);
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == SpreadsheetDocument) {
        cellTool()->selectTextBackgroundColor(color);
    } else {
        if (setTextBackgroundColor(color, textEditor()) && m_collab)
            m_collab->sendTextColor(textEditor()->selectionStart(), textEditor()->selectionEnd(), color.rgb());
    }
    setDocumentModified(true);
}

bool MainWindow::setTextBackgroundColor(const QColor &color, KoTextEditor* editor) {
    if (editor) {
        editor->setTextBackgroundColor(color);
        return true;
    }
    return false;
}

void MainWindow::doBold()
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == PresentationDocument) {
           KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
           m_kopaview->kopaCanvas()->toolProxy()->actions()["format_bold"]->trigger();
    }
    if (documentType() == SpreadsheetDocument) {
        controller()->canvas()->toolProxy()->actions()["bold"]->trigger();
    } else {
        if (setBold(textEditor()) && m_collab)
            m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatBold);
    }
    setDocumentModified(true);
}

bool MainWindow::setBold(KoTextEditor *editor) {
    if(editor) {
        QTextCharFormat textchar = editor->charFormat();
        if (textchar.fontWeight()==QFont::Bold) {
            editor->bold(false);
        } else {
            editor->bold(true);
        }
        return true;
    }
    return false;
}

void MainWindow::doItalic()
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == PresentationDocument) {
           KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
           m_kopaview->kopaCanvas()->toolProxy()->actions()["format_italic"]->trigger();
    }
    if (documentType() == SpreadsheetDocument) {
        controller()->canvas()->toolProxy()->actions()["italic"]->trigger();
    } else {
        if (setItalic(textEditor()) && m_collab)
            m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatItalic);
    }
    setDocumentModified(true);
}

bool MainWindow::setItalic(KoTextEditor *editor) {
    if(editor) {
        QTextCharFormat textchar = editor->charFormat();
        if (textchar.fontItalic()) {
            editor->italic(false);
        } else {
            editor->italic(true);
        }
        return true;
    }
    return false;
}

void MainWindow::doUnderLine()
{
    if (m_fontstyleframe)
        m_fontstyleframe->hide();
    if (documentType() == PresentationDocument) {
           KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
           m_kopaview->kopaCanvas()->toolProxy()->actions()["format_underline"]->trigger();
    }
    if (documentType() == SpreadsheetDocument) {
        controller()->canvas()->toolProxy()->actions()["underline"]->trigger();
    } else {
        if (setUnderline(textEditor()) && m_collab)
            m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatUnderline);
    }
    setDocumentModified(true);
}

bool MainWindow::setUnderline(KoTextEditor *editor) {
    if(editor) {
        QTextCharFormat textchar = editor->charFormat();
        if(textchar.property(KoCharacterStyle::UnderlineType).toBool()) {
            editor->underline(false);
        } else {
            editor->underline(true);
        }
        return true;
    }
    return false;
}

void MainWindow::doLeftAlignment()
{
    if (m_formatframe) {
        m_formatframe->hide();
    }
    if (documentType() == PresentationDocument) {
            KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
            m_kopaview->kopaCanvas()->toolProxy()->actions()["format_alignleft"]->trigger();
    }
    if (setLeftAlign(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatAlignLeft);
}

bool MainWindow::setLeftAlign(KoTextEditor *editor)
{
    if (editor) {
        editor->setHorizontalTextAlignment(Qt::AlignLeft);
        setDocumentModified(true);
        return true;
    }
    return false;
}

void MainWindow::doJustify()
{
    if (m_formatframe)
        m_formatframe->hide();
    if (documentType() == PresentationDocument) {
            KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
            m_kopaview->kopaCanvas()->toolProxy()->actions()["format_alignblock"]->trigger();
    }
    if (setJustify(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatAlignJustify);
}

bool MainWindow::setJustify(KoTextEditor *editor) {
    if (editor) {
        editor->setHorizontalTextAlignment(Qt::AlignJustify);
        setDocumentModified(true);
        return true;
    }
    return false;
}

void MainWindow::doRightAlignment()
{
    if (m_formatframe)
        m_formatframe->hide();
    if (documentType() == PresentationDocument) {
            KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
            m_kopaview->kopaCanvas()->toolProxy()->actions()["format_alignright"]->trigger();
    }
    if (setRightAlign(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatAlignRight);
}

bool MainWindow::setRightAlign(KoTextEditor *editor) {
    if (editor) {
        editor->setHorizontalTextAlignment(Qt::AlignRight);
        setDocumentModified(true);
        return true;
    }
    return false;
}

void MainWindow::doCenterAlignment()
{
    if (m_formatframe)
        m_formatframe->hide();
    if (documentType() == PresentationDocument) {
            KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
            m_kopaview->kopaCanvas()->toolProxy()->actions()["format_aligncenter"]->trigger();
    }
    if (setCenterAlign(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatAlignCenter);
}

bool MainWindow::setCenterAlign(KoTextEditor *editor) {
    if (editor) {
        editor->setHorizontalTextAlignment(Qt::AlignHCenter);
        setDocumentModified(true);
        return true;
    }
    return true;
}

void MainWindow::activeFormatOptionCheck()
{
    if (documentType() == PresentationDocument) {
        KoCanvasBase *canvasForChecking = controller()->canvas();
        Q_CHECK_PTR(canvasForChecking);
        if (canvasForChecking->toolProxy()->selection()->hasSelection()) {
            m_kopaview = qobject_cast<KoPAView *>(view());
            KoShape *currentShapeSelected = m_kopaview->shapeManager()->selection()->firstSelectedShape(KoFlake::StrippedSelection);
            KoTextShapeData *currentSelectedTextShapeData = qobject_cast<KoTextShapeData*>(currentShapeSelected->userData());
            QTextDocument *documentForCurrentShape = currentSelectedTextShapeData->document();
            m_pEditor = new KoTextEditor(documentForCurrentShape);
            KoTextDocument(documentForCurrentShape).setUndoStack(undoStack());
            KoTextDocument(documentForCurrentShape).setTextEditor(m_pEditor.data());
            QTextBlockFormat blk = m_pEditor->blockFormat();
            Qt::Alignment textblock_align = blk.alignment();
            switch(textblock_align) {
                case Qt::AlignLeft :
                                m_alignleft->setChecked(true);
                                m_alignjustify->setChecked(false);
                                m_alignright->setChecked(false);
                                m_aligncenter->setChecked(false);
                                break;
                case Qt::AlignRight :
                                m_alignright->setChecked(true);
                                m_alignjustify->setChecked(false);
                                m_alignleft->setChecked(false);
                                m_aligncenter->setChecked(false);
                                break;
                case Qt::AlignCenter :
                                m_aligncenter->setChecked(true);
                                m_alignjustify->setChecked(false);
                                m_alignright->setChecked(false);
                                m_alignleft->setChecked(false);
                                break;
                case Qt::AlignJustify :
                                m_alignjustify->setChecked(true);
                                m_alignleft->setChecked(false);
                                m_alignright->setChecked(false);
                                m_aligncenter->setChecked(false);
                                break;
            }
        }
    }
    else {
        QTextBlockFormat blk = textEditor()->blockFormat();
        Qt::Alignment textblock_align = blk.alignment();
        switch(textblock_align) {
            case Qt::AlignLeft :
                            m_alignleft->setChecked(true);
                            m_alignjustify->setChecked(false);
                            m_alignright->setChecked(false);
                            m_aligncenter->setChecked(false);
                            break;
            case Qt::AlignRight :
                            m_alignright->setChecked(true);
                            m_alignjustify->setChecked(false);
                            m_alignleft->setChecked(false);
                            m_aligncenter->setChecked(false);
                            break;
            case Qt::AlignCenter :
                            m_aligncenter->setChecked(true);
                            m_alignjustify->setChecked(false);
                            m_alignright->setChecked(false);
                            m_alignleft->setChecked(false);
                            break;
            case Qt::AlignJustify :
                            m_alignjustify->setChecked(true);
                            m_alignleft->setChecked(false);
                            m_alignright->setChecked(false);
                            m_aligncenter->setChecked(false);
                            break;
        }
    }
}

void MainWindow::activeFontOptionCheck()
{
    if (documentType() == PresentationDocument) {
        KoCanvasBase *canvasForChecking = controller()->canvas();
        Q_CHECK_PTR(canvasForChecking);
        if(canvasForChecking->toolProxy()->selection()->hasSelection())
        {
          m_kopaview = qobject_cast<KoPAView *>(view());
          KoShape *currentShapeSelected = m_kopaview->shapeManager()->selection()->firstSelectedShape(KoFlake::StrippedSelection);
          KoTextShapeData *currentSelectedTextShapeData = qobject_cast<KoTextShapeData*>(currentShapeSelected->userData());
          QTextDocument *documentForCurrentShape = currentSelectedTextShapeData->document();
          m_pEditor = new KoTextEditor(documentForCurrentShape);
          KoTextDocument(documentForCurrentShape).setUndoStack(undoStack());
          KoTextDocument(documentForCurrentShape).setTextEditor(m_pEditor.data());
          if(m_superscript) {

            QTextCharFormat textchar = m_pEditor.data()->charFormat();
            if(textchar.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
                m_superscript->setChecked(true);
            } else {
                m_superscript->setChecked(false);
            }
        }

        if(m_subscript) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            if(textchar.verticalAlignment() == QTextCharFormat::AlignSubScript) {
                m_subscript->setChecked(true);
            } else {
                m_subscript->setChecked(false);
            }
        }

        if(m_fontsizebutton) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            QFont font=textchar.font();
            m_fontsizebutton->setText(QString().setNum(font.pointSize()));
        }

        if(m_fontcombobox) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            QString fonttype = textchar.fontFamily();
            m_fontcombobox->setCurrentFont(QFont(fonttype));
        }

        if(m_bold) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            if (textchar.fontWeight()==QFont::Bold) {
                m_bold->setChecked(true);
            } else {
                m_bold->setChecked(false);
            }
        }

        if(m_italic) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            if (textchar.fontItalic()) {
                m_italic->setChecked(true);
            } else {
                m_italic->setChecked(false);
            }
        }

        if(m_underline) {
            QTextCharFormat textchar = m_pEditor.data()->cursor()->charFormat();
            if (textchar.property(KoCharacterStyle::UnderlineType).toBool()) {
                m_underline->setChecked(true);
            } else {
                m_underline->setChecked(false);
            }
       }
    }
    }

    if (documentType() == TextDocument) {
        if(m_superscript) {
            QTextCharFormat textchar = textEditor()->charFormat();
            if (textEditor()->charFormat().verticalAlignment() == QTextCharFormat::AlignSuperScript) {
                m_superscript->setChecked(true);
            } else {
                m_superscript->setChecked(false);
            }
        }

        if(m_subscript) {
            QTextCharFormat textchar = textEditor()->charFormat();
            if (textEditor()->charFormat().verticalAlignment() == QTextCharFormat::AlignSubScript) {
                m_subscript->setChecked(true);
            } else {
                m_subscript->setChecked(false);
            }
        }

        if(m_fontsizebutton) {
            QTextCharFormat textchar = textEditor()->charFormat();
            QFont font=textchar.font();
            m_fontsizebutton->setText(QString().setNum(font.pointSize()));
        }

        if(m_fontcombobox) {
            QTextCharFormat textchar = textEditor()->charFormat();
            QString fonttype = textchar.fontFamily();
            m_fontcombobox->setCurrentFont(QFont(fonttype));
        }

        if(m_bold) {
            QTextCharFormat textchar = textEditor()->charFormat();
            if (textchar.fontWeight()==QFont::Bold) {
                m_bold->setChecked(true);
            } else {
                m_bold->setChecked(false);
            }
        }

        if(m_italic) {
            QTextCharFormat textchar = textEditor()->charFormat();
            if (textchar.fontItalic()) {
                m_italic->setChecked(true);
            } else {
                m_italic->setChecked(false);
            }
        }

        if(m_underline) {
            QTextCharFormat textchar = textEditor()->charFormat();
            if(textchar.property(KoCharacterStyle::UnderlineType).toBool()) {
                m_underline->setChecked(true);
            } else {
                m_underline->setChecked(false);
            }
        }
    }

    if (documentType() == SpreadsheetDocument) {
        if(m_fontsizebutton) {
           QString fontsize;
           fontsize.setNum(cellTool()->getFontSize());
           m_fontsizebutton->setText(fontsize);
        }

        if(m_fontcombobox) {
            m_fontcombobox->setCurrentFont(QFont(cellTool()->getFontType()));
        }

        if(m_bold) {
            if (cellTool()->isFontBold()) {
                m_bold->setChecked(true);
            } else {
                m_bold->setChecked(false);
            }
        }

        if(m_italic) {
            if (cellTool()->isFontItalic()) {
                m_italic->setChecked(true);
            } else {
                m_italic->setChecked(false);
            }
        }

        if(m_underline) {
            if(cellTool()->isFontUnderline()) {
                m_underline->setChecked(true);
            } else {
                m_underline->setChecked(false);
            }
        }
    }
}

void MainWindow::doNumberList()
{
    if(m_formatframe->isVisible())
        m_formatframe->hide();
    if (documentType() == PresentationDocument) {
        m_kopaview = qobject_cast<KoPAView *>(view());
        KoShape *currentShapeSelected = m_kopaview->shapeManager()->selection()->firstSelectedShape();
        KoTextShapeData *currentSelectedTextShapeData = qobject_cast<KoTextShapeData*>(currentShapeSelected->userData());
        QTextDocument *documentForCurrentShape = currentSelectedTextShapeData->document();
        m_pEditor = new KoTextEditor(documentForCurrentShape);
        KoTextDocument(documentForCurrentShape).setTextEditor(m_pEditor.data());
        setNumberList(m_pEditor);
    }
    if (setNumberList(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatListNumber);
}

bool MainWindow::setNumberList(KoTextEditor *editor) {
    if (editor) {
        doStyle(KoListStyle::DecimalItem, editor);
        return true;
    }
    return false;
}

void MainWindow::doBulletList()
{
    if (m_formatframe->isVisible())
        m_formatframe->hide();
    if (setBulletList(textEditor()) && m_collab)
        m_collab->sendFormat(textEditor()->selectionStart(), textEditor()->selectionEnd(), Collaborate::FormatListBullet);
    if (documentType() == PresentationDocument) {
        m_kopaview = qobject_cast<KoPAView *>(view());
        KoShape *currentShapeSelected = m_kopaview->shapeManager()->selection()->firstSelectedShape();
        KoTextShapeData *currentSelectedTextShapeData = qobject_cast<KoTextShapeData*>(currentShapeSelected->userData());
        QTextDocument *documentForCurrentShape = currentSelectedTextShapeData->document();
        m_pEditor = new KoTextEditor(documentForCurrentShape);
        KoTextDocument(documentForCurrentShape).setTextEditor(m_pEditor.data());
        setBulletList(m_pEditor);
    }
}

bool MainWindow::setBulletList(KoTextEditor *editor) {
    if (editor) {
        doStyle(KoListStyle::DiscItem, editor);
        return true;
    }
    return false;
}

void MainWindow::doStyle(KoListStyle::Style style, KoTextEditor *editor)
{
    KoParagraphStyle *parag = new KoParagraphStyle();
    KoListStyle *list = new KoListStyle(parag);
    KoListLevelProperties llp = list->levelProperties(0);
    llp.setStyle(style);
    list->setLevelProperties(llp);
    parag->setListStyle(list);
    editor->setStyle(parag);
    setDocumentModified(true);
}

QString MainWindow::showGetOpenFileNameDialog(const QString& caption, const QString& dir, const QString& filter)
{
    return QFileDialog::getOpenFileName(this, caption, dir, filter, 0, QFileDialog::DontUseNativeDialog);
}

QString MainWindow::showGetSaveFileNameDialog(const QString& caption, const QString& dir, const QString& filter)
{
    return QFileDialog::getSaveFileName(this, caption, dir, filter, 0, QFileDialog::DontUseNativeDialog);
}

void MainWindow::chooseDocumentType()
{
    m_docdialog = new QDialog(this);
    Q_CHECK_PTR(m_docdialog);

    m_docdialoglayout = new QGridLayout;
    Q_CHECK_PTR(m_docdialoglayout);

    m_document = addNewDocument("Document");
    m_presenter = addNewDocument("Presenter");
    m_spreadsheet = addNewDocument("SpreadSheet");

    m_docdialoglayout->addWidget(m_document,0,0);
    m_docdialoglayout->addWidget(m_presenter,0,1);
    m_docdialoglayout->addWidget(m_spreadsheet,0,2);

    m_docdialog->setLayout(m_docdialoglayout);
    m_docdialog->show();

    connect(m_document,SIGNAL(clicked()),this,SLOT(openNewDoc()));
    connect(m_presenter,SIGNAL(clicked()),this,SLOT(openNewPresenter()));
    connect(m_spreadsheet,SIGNAL(clicked()),this,SLOT(openNewSpreadSheet()));
}

void MainWindow::openNewDoc()
{
    m_docdialog->close();
    openDocument(NEW_WORDDOC, true);
}

void MainWindow::openNewPresenter()
{
    m_docdialog->close();
    templateSelectionDialog();
}

void MainWindow::openNewSpreadSheet()
{
   m_docdialog->close();
   openDocument(NEW_SPREADSHEET,true);
}

void MainWindow::templateSelectionDialog()
{
    if(m_tempselectiondialog){
        m_tempselectiondialog->show();
        return;
    }
    const QDir temppath(NEW_PRESENTER);
    m_temptitle <<temppath.entryList(QDir::Files);

    m_tempselectiondialog = new QDialog(this);
    Q_CHECK_PTR(m_tempselectiondialog);
    m_tempselectiondialog->setWindowTitle(i18n("Select Presentation Template Preview"));

    m_tempdialoglayout = new QGridLayout;
    Q_CHECK_PTR(m_tempdialoglayout);
    m_tempdialoglayout->setVerticalSpacing(0);
    m_tempdialoglayout->setHorizontalSpacing(0);

    m_templateWidget = new QListWidget(this);
    Q_CHECK_PTR(m_templateWidget);
    QStringList tempNames;
    tempNames<<m_temptitle;
    tempNames.replaceInStrings("_"," ");
    tempNames.replaceInStrings(".odp","");
    m_templateWidget->addItems(tempNames);
    m_templateWidget->setMinimumSize(500,200);

    m_go = new QPushButton("Load",this);
    Q_CHECK_PTR(m_go);

    m_closetemp = new QPushButton("Cancel",this);
    Q_CHECK_PTR(m_closetemp);

    m_templatepreview = new QLabel(this);
    Q_CHECK_PTR(m_templatepreview);

    m_tempdialoglayout->addWidget(m_templateWidget,0,0,1,2);
    m_tempdialoglayout->addWidget(m_templatepreview,0,2);
    m_tempdialoglayout->addWidget(m_go,1,0);
    m_tempdialoglayout->addWidget(m_closetemp,1,1);

    m_tempselectiondialog->setLayout(m_tempdialoglayout);
    m_tempselectiondialog->show();

    connect(m_templateWidget,SIGNAL(currentRowChanged(int)),SLOT(selectedTemplatePreview(int)));
    connect(m_go,SIGNAL(clicked()),SLOT(openSelectedTemplate()));
    connect(m_closetemp,SIGNAL(clicked()),SLOT(closeTempSelectionDialog()));
}

void MainWindow::selectedTemplatePreview(int number)
{
    QString path = NEW_PRESENTER+m_temptitle.at(number);
    KoStore::Backend backend = KoStore::Auto;
    KoStore * store = KoStore::createStore(path, KoStore::Read, "", backend);
    if(store->open("/Thumbnails/thumbnail.png")) {
        QByteArray content=store->device()->readAll();
        QPixmap thumbnail;
        thumbnail.loadFromData(content);
        store->close();
        m_templatepreview->setPixmap(thumbnail);
    }
    delete store;
}

void MainWindow::openSelectedTemplate()
{
    newpresenter = NEW_PRESENTER+m_temptitle.at(m_templateWidget->currentRow());
    m_tempselectiondialog->accept();
    openDocument(newpresenter, true);
}

void MainWindow::closeTempSelectionDialog()
{
    if(m_tempselectiondialog)
        m_tempselectiondialog->hide();
}

QPushButton * MainWindow::addFormatFrameComponent(const QString &imagepath)
{
    QPushButton * btn = new QPushButton(this);
    Q_CHECK_PTR(btn);
    btn->setCheckable(true);
    btn->setIcon(QIcon(":/images/48x48/Edittoolbaricons/"+imagepath+".png"));
    btn->setMaximumSize(165,75);
    return btn;
}

QPushButton * MainWindow::addFontStyleFrameComponent(const QString &imagepath)
{
    QPushButton * btn = new QPushButton(this);
    Q_CHECK_PTR(btn);
    btn->setCheckable(true);
    btn->setIcon(QIcon(":/images/48x48/Edittoolbaricons/"+imagepath+".png"));
    btn->setMinimumSize(165,70);
    btn->setMaximumSize(165,70);
    return btn;
}

QToolButton * MainWindow ::addNewDocument(const QString &docname)
{
    QToolButton * toolbutton = new QToolButton(this);
    Q_CHECK_PTR(toolbutton);
    toolbutton->setIcon(QIcon(":/images/48x48/"+docname+".png"));
    toolbutton->setToolTip(docname);
    return toolbutton;
}

void MainWindow::openAboutDialog(void)
{
    QList<HildonMenu *> all_dlg = findChildren<HildonMenu *>();
    foreach(HildonMenu *menu, all_dlg)
    menu->close();
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::toggleToolBar(bool show)
{
    KoToolManager::instance()->switchToolRequested(PanTool_ID);
    if (documentType() == SpreadsheetDocument) {
        KoToolManager::instance()->switchToolRequested(CellTool_ID);
        KSpread::Doc *kspreadDoc = qobject_cast<KSpread::Doc*>(document());
        kspreadDoc->map()->setReadWrite(!show);
    }
    document()->setReadWrite(!show);
    m_ui->viewToolBar->setVisible(!show);
    m_ui->SearchToolBar->setVisible(show);
    m_isViewToolBar = !show;
    if (show) {
        m_search->setFocus();
        m_search->selectAll();
    }
    else {
        m_search->clearFocus();
    }
}

bool MainWindow::setEditingMode(bool set)
{
    if (!KoAbstractApplication::setEditingMode(set))
        return false;

    if (set) {
        //set the icon to be checked
        m_ui->actionEdit->blockSignals(true);
        if(! m_ui->actionEdit->isChecked()) {
            m_ui->actionEdit->setChecked(true);
        }
        m_ui->actionEdit->blockSignals(false);

        switch (documentType()) {
        case SpreadsheetDocument:
            KoToolManager::instance()->switchToolRequested(CellTool_ID);
            m_ui->viewToolBar->hide();
            if(m_spreadEditToolBar) {
                m_spreadEditToolBar->show();
            }
            m_ui->EditToolBar->hide();
            break;
        default:
            KoToolManager::instance()->switchToolRequested(TextTool_ID);
            m_ui->viewToolBar->hide();
            m_ui->EditToolBar->show();
            if(m_spreadEditToolBar) {
                m_spreadEditToolBar->hide();
            }
            m_spaceHandlerShortcutForVirtualKeyBoard->setEnabled(true);
            m_shortcutForVirtualKeyBoard->setEnabled(true);
        }
    }
    else {
        KoToolManager::instance()->switchToolRequested(PanTool_ID);
        m_ui->EditToolBar->hide();
        m_ui->viewToolBar->show();
        if(m_spreadEditToolBar) {
            m_spreadEditToolBar->hide();
        }
        m_isViewToolBar = true;

        m_spaceHandlerShortcutForVirtualKeyBoard->setEnabled(false);
        m_shortcutForVirtualKeyBoard->setEnabled(false);

        m_ui->actionEdit->blockSignals(true);
        if(m_ui->actionEdit->isChecked()) {
            m_ui->actionEdit->setChecked(false);
        }
        m_ui->actionEdit->blockSignals(false);
    }

    if(m_formatframe)
        m_formatframe->hide();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();
    return true;
}

void MainWindow::doUndo()
{
    if(m_formatframe)
        m_formatframe->hide();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();

    if (undoStack() && undoStack()->canUndo()) {
        undoStack()->undo();
    }
}

void MainWindow::doRedo()
{
    if(m_formatframe)
        m_formatframe->hide();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();

    if (undoStack() && undoStack()->canRedo()) {
        undoStack()->redo();
    }
}

void MainWindow::copy()
{
    if(   documentType() != PresentationDocument && documentType() != SpreadsheetDocument
       && !controller()->canvas()->toolProxy()->selection()->hasSelection())
        return;
    controller()->canvas()->toolProxy()->copy();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();
    if(m_formatframe)
        m_formatframe->hide();
}

void MainWindow::cut()
{
    if(   documentType() != PresentationDocument && documentType() != SpreadsheetDocument
       && !controller()->canvas()->toolProxy()->selection()->hasSelection())
        return;
    controller()->canvas()->toolProxy()->copy();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();
    if(m_formatframe)
        m_formatframe->hide();
}

void MainWindow::paste()
{
    controller()->canvas()->toolProxy()->paste();

    if(m_fontstyleframe)
        m_fontstyleframe->hide();
    if(m_formatframe)
        m_formatframe->hide();
}

void MainWindow::showFullScreenPresentationIcons(void)
{
    if (!controller())
        return;
    int vScrlbarWidth = 0;
    int hScrlbarHeight = 0;
    QSize size(frameSize());

    if (controller()->verticalScrollBar()->isVisible()) {
        QSize vScrlbar = controller()->verticalScrollBar()->size();
        vScrlbarWidth = vScrlbar.width();
    }

    if (controller()->horizontalScrollBar()->isVisible()) {
        QSize hScrlbar = controller()->horizontalScrollBar()->size();
        hScrlbarHeight = hScrlbar.height();
    }

    if ( ! m_fsPPTBackButton && ( presentationTool() ) && ( ! presentationTool()->toolsActivated() ) ) {
        m_fsPPTBackButton = new QPushButton(this);
        m_fsPPTBackButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
        m_fsPPTBackButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
        m_fsPPTBackButton->setIcon(QIcon(FS_PPT_BACK_BUTTON_PATH));
        connect(m_fsPPTBackButton, SIGNAL(clicked()), this, SLOT(goToPreviousPage()));
        m_fsPPTBackButton->move(size.width() - FS_BUTTON_SIZE*3 - vScrlbarWidth,
                                size.height() - FS_BUTTON_SIZE - hScrlbarHeight);
    }

    if (! m_fsPPTForwardButton && ( presentationTool() ) && ( ! presentationTool()->toolsActivated() ) ) {
        m_fsPPTForwardButton = new QPushButton(this);
        m_fsPPTForwardButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
        m_fsPPTForwardButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
        m_fsPPTForwardButton->setIcon(QIcon(FS_PPT_FORWARD_BUTTON_PATH));
        connect(m_fsPPTForwardButton, SIGNAL(clicked()), this, SLOT(goToNextPage()));
        m_fsPPTForwardButton->move(size.width() - FS_BUTTON_SIZE*2 - vScrlbarWidth,
                                   size.height() - FS_BUTTON_SIZE - hScrlbarHeight);
    }

    if ( ( ! m_fsPPTDrawPenButton ) && ( presentationTool() ) ) {
        m_fsPPTDrawPenButton = new QPushButton(this);
        m_fsPPTDrawPenButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
        m_fsPPTDrawPenButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
        m_fsPPTDrawPenButton->setIcon(QIcon(":/images/64x64/PresentationDrawTool/pen.png"));
        m_fsPPTDrawPenButton->move(736 ,284);
        connect(m_fsPPTDrawPenButton,SIGNAL(clicked()),presentationTool(),SLOT(togglePenTool()));
    }

    m_fsPPTDrawPenButton->show();
    m_fsPPTDrawPenButton->raise();

    if ( ( ! m_fsPPTDrawHighlightButton ) && ( presentationTool() ) ) {
        m_fsPPTDrawHighlightButton = new QPushButton(this);
        m_fsPPTDrawHighlightButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
        m_fsPPTDrawHighlightButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
        m_fsPPTDrawHighlightButton->setIcon(QIcon(":/images/64x64/PresentationDrawTool/highlight.png"));
        m_fsPPTDrawHighlightButton->move(736,350);
        connect(m_fsPPTDrawHighlightButton,SIGNAL(clicked()),presentationTool(),SLOT(toggleHighlightTool()));
    }

    m_fsPPTDrawHighlightButton->show();
    m_fsPPTDrawHighlightButton->raise();

    if (currentPage() < document()->pageCount() && ( presentationTool() ) && ( ! presentationTool()->toolsActivated() ) ) {
        m_fsPPTForwardButton->move(size.width() - FS_BUTTON_SIZE*2 - vScrlbarWidth,
                                   size.height() - FS_BUTTON_SIZE - hScrlbarHeight);
        m_fsPPTForwardButton->show();
        m_fsPPTForwardButton->raise();
    }

    if ( currentPage() <= document()->pageCount() && currentPage() != 1 && ( presentationTool() ) && (! presentationTool()->toolsActivated() ) ) {
        m_fsPPTBackButton->move(size.width() - FS_BUTTON_SIZE*3 - vScrlbarWidth,
                                size.height() - FS_BUTTON_SIZE - hScrlbarHeight);
        m_fsPPTBackButton->show();
        m_fsPPTBackButton->raise();
    }
    if(documentType() == PresentationDocument) {
        if (!m_slideNotesButton) {
            m_slideNotesButton = new QPushButton(this);
            Q_CHECK_PTR(m_slideNotesButton);
            m_slideNotesButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
            m_slideNotesButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
            m_slideNotesButton->setIcon(m_slideNotesIcon);
            connect(m_slideNotesButton, SIGNAL(clicked()), SLOT(slideNotesButtonClicked()));
            m_slideNotesButton->move(736,222);
        }
        m_slideNotesButton->show();
        m_slideNotesButton->raise();

#ifdef Q_WS_MAEMO_5
        if ((!m_fsAccButton)&& (presentationTool())) {
             m_fsAccButton = new QPushButton(this);
             m_fsAccButton->setStyleSheet(FS_BUTTON_STYLE_SHEET);
             m_fsAccButton->resize(FS_BUTTON_SIZE, FS_BUTTON_SIZE);
             m_fsAccButton->setIcon(QIcon(":/images/64x64/Acceleration/swingoff.png"));
             m_fsAccButton->move(736,156);
             connect(m_fsAccButton, SIGNAL(clicked(bool)), SLOT(switchToSlid()));
        }
        m_fsAccButton->show();
        m_fsAccButton->raise();
#endif
    }
}
////////////////////////////////////////////////acc/////////////////////////////
#ifdef Q_WS_MAEMO_5
void MainWindow::switchToSlid()
{

     enableAccelerator=!enableAccelerator;
     if(!enableAccelerator) {
           m_fsAccButton->setIcon(QIcon(":/images/64x64/Acceleration/swingoff.png"));
           acceleratorForScrolAndSlide.stopRecognition();
           disconnect(&acceleratorForScrolAndSlide,SIGNAL(next()),m_fsPPTForwardButton,SLOT(click()));
           disconnect(&acceleratorForScrolAndSlide,SIGNAL(previous()),m_fsPPTBackButton,SLOT(click()));

     }
     else {

           m_fsAccButton->setIcon(QIcon(":/images/64x64/Acceleration/swingon.png"));
           acceleratorForScrolAndSlide.startRecognitionSlide();
           disconnect(&acceleratorForScrolAndSlide,SIGNAL(next()),m_fsPPTForwardButton,SLOT(click()));
           disconnect(&acceleratorForScrolAndSlide,SIGNAL(previous()),m_fsPPTBackButton,SLOT(click()));
           connect(&acceleratorForScrolAndSlide,SIGNAL(next()),m_fsPPTForwardButton,SLOT(click()));
           connect(&acceleratorForScrolAndSlide,SIGNAL(previous()),m_fsPPTBackButton,SLOT(click()));
      }

 }
void MainWindow::enableDisableScrollingOption()
{

    if(enableScrolling) {
        switchToScroll();
}
    if(stopAcceleratorScrolling) {
        stopAcceleratorScrolling=false;

    }
    else {
        stopAcceleratorScrolling=true;

    }
}


void MainWindow::switchToScroll()
{
   enableScrolling=!enableScrolling;

   if(enableScrolling) {
       acceleratorForScrolAndSlide.startRecognitionScroll();
       connect(&acceleratorForScrolAndSlide,SIGNAL(change()),this,SLOT(scrollAction()));

   }
   else {
       disconnect(&acceleratorForScrolAndSlide,SIGNAL(change()),this,SLOT(scrollAction()));
       acceleratorForScrolAndSlide.stopRecognitionScroll();
   }
}

void MainWindow::scrollAction()
{
    controller()->verticalScrollBar()->
                  setSliderPosition (
                    (controller()->verticalScrollBar()->sliderPosition()
                    +acceleratorForScrolAndSlide.getVerticalScrollValue())
                  );
    controller()->horizontalScrollBar()->
                  setSliderPosition (
                    (controller()->horizontalScrollBar()->sliderPosition()
                    +acceleratorForScrolAndSlide.getHorizontalScrollValue())
                  );
    acceleratorForScrolAndSlide.resetScrollValues();
}

void MainWindow::closeAcceleratorSettings()
{
    if(enableAccelerator) {
        m_fsAccButton->setIcon(QIcon(":/images/64x64/Acceleration/swingoff.png"));
        disconnect(&acceleratorForScrolAndSlide,SIGNAL(next()),m_fsPPTForwardButton,SLOT(click()));
        disconnect(&acceleratorForScrolAndSlide,SIGNAL(previous()),m_fsPPTBackButton,SLOT(click()));

        acceleratorForScrolAndSlide.stopRecognition();
        enableAccelerator=!enableAccelerator;
    }
    if(enableScrolling) {
        disconnect(&acceleratorForScrolAndSlide,SIGNAL(change()),this,SLOT(scrollAction()));
        acceleratorForScrolAndSlide.stopRecognitionScroll();
        enableScrolling=false;
    }
    disconnect(shortcutForAccelerator,SIGNAL(activated()),this,SLOT(enableDisableScrollingOption()));
    disconnect(&acceleratorForScrolAndSlide,SIGNAL(StopTheAccelerator()),this,SLOT(switchToScroll()));
}

#endif
//////////////////////////////////////////////////////////////////////////////

void MainWindow::slideTransitionDialog(){
    m_slidingmotiondialog = new SlidingMotionDialog;
    m_slidingmotiondialog->m_select = gl_style ;
    m_slidingmotiondialog->m_time = gl_showtime / 1000;
    m_slidingmotiondialog->showDialog(this);
#ifdef HAVE_OPENGL
    connect(m_slidingmotiondialog, SIGNAL(startglshow(int,int)), SLOT(glPresenterSet(int,int)));
#endif
#ifdef Q_WS_MAEMO_5
   connect(m_slidingmotiondialog->m_opengl, SIGNAL(clicked()),&acceleratorForScrolAndSlide,SLOT(startSlideSettings()));
   connect(m_slidingmotiondialog->m_acceleration,SIGNAL(clicked()),&acceleratorForScrolAndSlide,SLOT(startScrollSettings()));
#endif

}

void MainWindow::slideNotesButtonClicked()
{
    if(m_notesDialog) {
        disconnect(m_notesDialog,SIGNAL(gotoPage(int)),this,SLOT(gotoPage(int)));
        disconnect(m_notesDialog,SIGNAL(moveSlide(bool)),this,SLOT(moveSlideFromNotesSlide(bool)));
        delete m_notesDialog;
    }
    m_notesDialog = new NotesDialog(document(), m_viewNumber, storeButtonPreview()->thumbnailList());
    m_notesDialog->show();
    connect(m_notesDialog,SIGNAL(moveSlide(bool)),this,SLOT(moveSlideFromNotesSlide(bool)));
    connect(m_notesDialog,SIGNAL(gotoPage(int)),this,SLOT(gotoPage(int)));

    m_notesDialog->show();
    m_notesDialog->showNotesDialog(currentPage());
}

void MainWindow::moveSlideFromNotesSlide(bool flag)
{
    if (flag) {
        goToNextPage();
    } else {
        goToPreviousPage();
    }
}

bool MainWindow::openDocument()
{
    QList<HildonMenu *> all_dlg = findChildren<HildonMenu *>();
    foreach(HildonMenu *menu, all_dlg)
        menu->close();

    return KoAbstractApplication::openDocument();
}

bool MainWindow::openDocument(const QString &fileName, bool isNewDocument)
{
    return KoAbstractApplication::openDocument(fileName, isNewDocument);
}

// Reimplemented
void MainWindow::closeDocument()
{
    KoAbstractApplication::closeDocument();

#ifdef Q_WS_MAEMO_5
    if (m_foDocumentRdf && documentType() == TextDocument) {
        disconnect(m_rdfShortcut,SIGNAL(activated()),m_foDocumentRdf,SLOT(highlightRdf()));
        delete m_rdfShortcut;
        delete m_foDocumentRdf;
        m_foDocumentRdf=0;
        m_rdfShortcut=0;
    }
#endif
    if (m_digitalSignatureDialog && documentType() == TextDocument) {
        disconnect(m_ui->actionDigitalSignature,SIGNAL(triggered()),this,SLOT(showDigitalSignatureInfo()));
        QMenuBar* menu = menuBar();
        menu->removeAction(m_ui->actionDigitalSignature);
        delete m_digitalSignatureDialog;
        m_digitalSignatureDialog = 0;
    }
    switch (documentType()) {
    case PresentationDocument:
        if (presentationTool()) {
            disconnect(m_fsButton, SIGNAL(clicked()), presentationTool(), SLOT(deactivateTool()));
        }
        break;
    case SpreadsheetDocument:
        m_spreadEditToolBar->hide();
        resetSpreadEditorToolBar();
        if (m_sheetInfoFrame && m_sheetInfoFrame->isVisible()) {
            spreadSheetInfo();
        }
        break;
    default:
        break;
    }

    if (m_ui->EditToolBar)
        m_ui->EditToolBar->hide();

    if(m_ui->viewToolBar)
        m_ui->viewToolBar->hide();

    if(m_ui->SearchToolBar)
        m_ui->SearchToolBar->hide();

    m_ui->actionZoomLevel->setText(i18n("%1 %", 100));
    m_ui->actionPageNumber->setText(i18n("%1 of %2", 0, 0));

    fontStyleFrameDestructor();
    formatFrameDestructor();

    if (m_slidingmotiondialog) {
        delete m_slidingmotiondialog;
        m_slidingmotiondialog = 0;
    }

    m_viewNumber++;
}

void MainWindow::formatFrameDestructor() {
    if(m_formatframe) {
        m_formatframe->hide();

        delete m_alignright;
        m_alignright = 0;
        delete m_alignleft;
        m_alignleft = 0;
        delete m_alignjustify;
        m_alignjustify = 0;
        delete m_aligncenter;
        m_aligncenter = 0;
        delete m_numberedlist;
        m_numberedlist = 0;
        delete m_bulletlist;
        m_bulletlist = 0;
        delete m_formatframe ;
        m_formatframe = 0;
     }
}

void MainWindow::fontStyleFrameDestructor() {
    if(m_fontstyleframe) {
        m_fontstyleframe->close();
        delete m_bold;
        m_bold = 0;
        delete m_italic;
        m_italic = 0;
        delete m_underline;
        m_underline = 0;
        delete m_subscript;
        m_subscript = 0;
        delete m_superscript;
        m_superscript = 0;
        delete m_fontcombobox;
        m_fontcombobox = 0;
        delete m_fontsizebutton;
        m_fontsizebutton = 0;
        delete m_textcolor;
        m_textcolor = 0;
        delete m_textbackgroundcolor;
        m_textbackgroundcolor = 0;
        delete m_fontstyleframe;
        m_fontstyleframe = 0;
    }
}

void MainWindow::raiseWindow(void)
{
    Display *display = QX11Info::display();
    XEvent e;
    Window root = RootWindow(display, DefaultScreen(display));
    memset(&e, 0, sizeof(e));
    e.xclient.type         = ClientMessage;
    e.xclient.window       = effectiveWinId();
    e.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    e.xclient.format       = 32;
    XSendEvent(display, root, False, SubstructureRedirectMask, &e);
    XFlush(display);
}

// reimplemented
bool MainWindow::doOpenDocument()
{
    if (!KoAbstractApplication::doOpenDocument())
        return false;

    m_search->setText(QString());

#ifdef Q_WS_MAEMO_5
    connect(shortcutForAccelerator,SIGNAL(activated()),this,SLOT(enableDisableScrollingOption()));
    connect(&acceleratorForScrolAndSlide,SIGNAL(StopTheAccelerator()),this,SLOT(switchToScroll()));
#endif
    return true;
}

void MainWindow::documentPageSetupChanged()
{
    KoAbstractApplication::documentPageSetupChanged();

    if (!view() || !m_ui)
        return;

    QString pageNo = i18n("pg%1 - pg%2", 0, 0);
    int factor = 100;

    if (document()->pageCount() > 0) {
        factor = view()->zoomController()->zoomAction()->effectiveZoom() * 100;
        pageNo = i18n("pg%1 - pg%2", 1, QString::number(document()->pageCount()));
    }

    if (documentType() == SpreadsheetDocument) {
        KSpread::Doc *kspreadDoc = qobject_cast<KSpread::Doc*>(document());
        if (kspreadDoc->map()->count() > 0)
            pageNo = i18n("pg%1 - pg%2", 1, QString::number(kspreadDoc->map()->count()));
    }

    m_ui->actionZoomLevel->setText(i18n("%1 %", QString::number(factor)));
    m_ui->actionPageNumber->setText(pageNo);

    m_vPage = controller()->verticalScrollBar()->pageStep();
    m_hPage = controller()->horizontalScrollBar()->pageStep();
}

void MainWindow::fullScreen()
{
    if (!m_ui)
        return;
    int vScrlbarWidth = 0;
    int hScrlbarHeight = 0;
    m_ui->viewToolBar->hide();
    m_ui->SearchToolBar->hide();
    m_ui->EditToolBar->hide();
    if (documentType() == PresentationDocument) {
        emit presentationStarted();
    }
    showFullScreen();
    QSize size(frameSize());

    if (controller()) {
        if (controller()->verticalScrollBar()->isVisible()) {
            QSize vScrlbar = controller()->verticalScrollBar()->size();
            vScrlbarWidth = vScrlbar.width();
        }
        if (controller()->horizontalScrollBar()->isVisible()) {
            QSize hScrlbar = controller()->horizontalScrollBar()->size();
            hScrlbarHeight = hScrlbar.height();
        }
    }

    m_fsButton->move(size.width() - FS_BUTTON_SIZE - vScrlbarWidth,
                     size.height() - FS_BUTTON_SIZE - hScrlbarHeight);
    m_fsButton->show();
    m_fsButton->raise();
    m_fsTimer->start(3000);

    if (documentType() == PresentationDocument) {
        if((!presentationTool()))
        {
            setPresentationTool( new PresentationTool(this, controller()) );
            connect(m_fsButton, SIGNAL(clicked()), presentationTool(), SLOT(deactivateTool()));
        }
        showFullScreenPresentationIcons();
    }
}

void MainWindow::zoomIn()
{
    if (!view() || !m_ui)
        return;
    KoZoomAction *zAction =  view()->zoomController()->zoomAction();
    int factor = zAction->effectiveZoom() * 100;
    if(factor<199)
    {
        zAction->zoomIn();
    }
    m_ui->actionZoomLevel->setText(i18n("%1 %", QString::number(factor)));
}

void MainWindow::zoomOut()
{
    if (!view() || !m_ui)
        return;
    KoZoomAction *zAction = view()->zoomController()->zoomAction();
    int factor = zAction->effectiveZoom() * 100;
    if(factor>70)
    {
        zAction->zoomOut();
    }
    m_ui->actionZoomLevel->setText(i18n("%1 %", QString::number(factor)));
}

void MainWindow::zoom()
{
    if (documentType() == SpreadsheetDocument) {
        ZoomDialog dlg(this);
        connect(&dlg, SIGNAL(fitPage()), SLOT(zoomToPage()));
        connect(&dlg, SIGNAL(fitPageWidth()), SLOT(zoomToPageWidth()));
        dlg.exec();
    }
}

void MainWindow::zoomToPage()
{
    if (!view() || !m_ui)
        return;
    view()->zoomController()->setZoomMode(KoZoomMode::ZOOM_PAGE);
    if (documentType() == PresentationDocument)
        m_ui->actionZoomLevel->setText("48%");
    else
        m_ui->actionZoomLevel->setText("29%");
}

void MainWindow::zoomToPageWidth()
{
    if (!view() || !m_ui)
        return;
    view()->zoomController()->setZoomMode(KoZoomMode::ZOOM_WIDTH);
    if (documentType() == PresentationDocument)
        m_ui->actionZoomLevel->setText("84%");
    else
        m_ui->actionZoomLevel->setText("100%");
}

/*void MainWindow::prevPage()
{
    if (!controller())
        return;
    if ((m_doc->pageCount() > 0) && triggerAction("page_previous"))
        return;
    m_vPage = controller()->verticalScrollBar()->pageStep();
    controller()->pan(QPoint(0, -m_vPage));
}

void MainWindow::nextPage()
{
    if (!controller())
        return;
    if ((m_doc->pageCount() > 0) && triggerAction("page_next"))
        return;
    m_vPage = controller()->verticalScrollBar()->pageStep();
    controller()->pan(QPoint(0, m_vPage));
}*/

void MainWindow::fsTimer()
{
    if (!m_pressed) {
        m_fsTimer->stop();
        m_fsButton->hide();
        if (m_fsPPTBackButton && m_fsPPTBackButton->isVisible())
            m_fsPPTBackButton->hide();
        if (m_fsPPTForwardButton && m_fsPPTForwardButton->isVisible())
            m_fsPPTForwardButton->hide();
        if (m_fsPPTDrawHighlightButton && m_fsPPTDrawHighlightButton->isVisible() )
            m_fsPPTDrawHighlightButton->hide();
        if (m_fsPPTDrawPenButton && m_fsPPTDrawPenButton->isVisible() )
            m_fsPPTDrawPenButton->hide();
        if (m_slideNotesButton && m_slideNotesButton->isVisible())
            m_slideNotesButton->hide();
#ifdef Q_WS_MAEMO_5
        if (m_fsAccButton && m_fsAccButton->isVisible())
            m_fsAccButton->hide();
#endif
    }
}

void MainWindow::fsButtonClicked()
{
    if (!m_ui)
        return;

    if (controller()) {
        controller()->show();
    }

    if (documentType() == PresentationDocument) {
        emit presentationStopped();
    }

    m_fsButton->hide();

    if (m_fsPPTBackButton && m_fsPPTBackButton->isVisible())
        m_fsPPTBackButton->hide();

    if (m_fsPPTForwardButton && m_fsPPTForwardButton->isVisible())
        m_fsPPTForwardButton->hide();

    if (m_fsPPTDrawHighlightButton && m_fsPPTDrawHighlightButton->isVisible() )
        m_fsPPTDrawHighlightButton->hide();

    if (m_fsPPTDrawPenButton && m_fsPPTDrawPenButton->isVisible() )
        m_fsPPTDrawPenButton->hide();

    if (m_slideNotesButton && m_slideNotesButton->isVisible())
        m_slideNotesButton->hide();

#ifdef Q_WS_MAEMO_5
    if (m_fsAccButton && m_fsAccButton->isVisible())
        m_fsAccButton->hide();
#endif

    if (m_isViewToolBar)
        m_ui->viewToolBar->show();
    else
        m_ui->SearchToolBar->show();

    showNormal();
}

static void findTextShapesRecursive(KoShapeContainer* con, KoPAPageBase* page,
                                    QList<QPair<KoPAPageBase*, KoShape*> >& shapes,
                                    QList<QTextDocument*>& docs)
{
    foreach(KoShape* shape, con->shapes()) {
        KoTextShapeData* tsd = qobject_cast<KoTextShapeData*> (shape->userData());
        if (tsd) {
            shapes.append(qMakePair(page, shape));
            docs.append(tsd->document());
        }
        KoShapeContainer* child = dynamic_cast<KoShapeContainer*>(shape);
        if (child)
            findTextShapesRecursive(child, page, shapes, docs);
    }
}

void MainWindow::startSearch()
{
    if (!m_search || !controller())
        return;

    QString searchString = m_search->text();
    if (documentType() == SpreadsheetDocument && !searchString.isEmpty()) {
        //for spreadsheet use the find method in FoCellTool.
        cellTool()->setCaseSensitive(m_searchCaseSensitive);
        cellTool()->slotSearchTextChanged(searchString);
        m_ui->actionSearchResult->setText(i18n("%1 of %2", cellTool()->currentSearchStatistics().first,
                                               cellTool()->currentSearchStatistics().second));
        goToNextWord();
        return;
    }

    if(m_search->text()==""){
        KoCanvasBase *canvas = controller()->canvas();
        KoSelection *selection = canvas->shapeManager()->selection();
        selection->deselectAll();
        m_ui->actionSearchResult->setText(i18n("%1 of %2", 0, 0));
        return;
    }

    KoCanvasBase *canvas = controller()->canvas();
    Q_CHECK_PTR(canvas);

    KoPADocument* padoc = qobject_cast<KoPADocument*>(document());
    if (padoc) {
        // loop over all pages starting from current page to get
        // search results in the right order
        int curPage = canvas->resourceManager()->resource(\
                      KoCanvasResource::CurrentPage).toInt() - 1;
        QList<QPair<KoPAPageBase*, KoShape*> > textShapes;
        QList<QTextDocument*> textDocs;
        for (int page = 0; page < padoc->pageCount(); page++) {
            KoPAPageBase* papage = padoc->pageByIndex(page, false);
            findTextShapesRecursive(papage, papage, textShapes, textDocs);
        };

        findText(textDocs, textShapes, searchString);

        // now find the first search result in the list of positions
        // counting from the current page
        // this is not very efficient...
        bool foundIt = false;
        for (int page = curPage; page < padoc->pageCount(); page++) {
            for (int i = 0; i < m_searchTextPositions.size(); i++) {
                if (m_searchTextPositions[i].first.first == padoc->pageByIndex(page, false)) {
                    foundIt = true;
                    m_searchTextIndex = i;
                    highlightText(m_searchTextIndex);
                    break;
                }
            }
            if (foundIt) break;
        }
        if (!foundIt) {
            for (int page = 0; page < curPage; page++) {
                for (int i = 0; i < m_searchTextPositions.size(); i++) {
                    if (m_searchTextPositions[i].first.first ==
                            padoc->pageByIndex(page, false)) {
                        foundIt = true;
                        m_searchTextIndex = i;
                        highlightText(m_searchTextIndex);
                        break;
                    }
                }
                if (foundIt) break;
            }
        }
    } else {
        KoShapeManager *manager = canvas->shapeManager();
        Q_CHECK_PTR(manager);
        QList<KoShape*> shapes = manager->shapes();

        int size = shapes.size();
        if (size != 0) {
            QList<KoTextShapeData*> shapeDatas;
            QList<QTextDocument*> textDocs;
            QList<QPair<KoPAPageBase*, KoShape*> > textShapes;
            QSet<QTextDocument*> textDocSet;

            for (int i = 0; i < size; i++) {
                shapeDatas.append(qobject_cast<KoTextShapeData*> \
                                  (shapes.at(i)->userData()));
                if (shapeDatas.at(i) && !textDocSet.contains(\
                        shapeDatas.at(i)->document())) {
                    textDocSet.insert(shapeDatas.at(i)->document());
                    textDocs.append(shapeDatas.at(i)->document());
                    textShapes.append(qMakePair((KoPAPageBase*)(0),
                                                shapes.at(i)));
                }
            }
            findText(textDocs, textShapes, searchString);
        }
    }
}

void MainWindow::findText(QList<QTextDocument*> aDocs,
                          QList<QPair<KoPAPageBase*, KoShape*> > shapes,
                          const QString &aText)
{
    if (aDocs.isEmpty())
        return;

    m_searchTextPositions.clear();

    for (int i = 0; i < aDocs.size(); i++) {
        QTextDocument* doc = aDocs.at(i);
        KoShape* shape = shapes.at(i).second;

        QTextCursor result(doc);
        do {
            if (!m_searchCaseSensitive)
                result = doc->find(aText, result);
            else
                result = doc->find(aText, result, QTextDocument::FindCaseSensitively);
            if (result.hasSelection()) {
                m_searchTextPositions.append(qMakePair(qMakePair(shapes.at(i).first, shape),
                                             qMakePair(result.selectionStart(),
                                                       result.selectionEnd())));
            }
        } while (!result.isNull());
    }

    m_searchTextIndex = 0;
    if (!m_searchTextPositions.isEmpty())
        highlightText(m_searchTextIndex);
    else
        m_ui->actionSearchResult->setText(i18n("%1 of %2", 0, 0));
}

void MainWindow::highlightText(int aIndex)
{
    if (!controller() || !m_ui)
        return;

    KoCanvasBase *canvas = controller()->canvas();
    Q_CHECK_PTR(canvas);

    // first potentially go to the correct page
    KoPAPageBase* page = m_searchTextPositions.at(aIndex).first.first;
    if (page) {
        KoPAView* paview = static_cast<KoPAView*>(view());
        if (paview->activePage() != page) {
            paview->doUpdateActivePage(page);
        }
    }

    // secondly set the currently selected text shape to the one containing this search result
    KoSelection *selection = canvas->shapeManager()->selection();
    KoShape* shape = m_searchTextPositions.at(aIndex).first.second;
    if (selection->count() != 1 || selection->firstSelectedShape() != shape) {
        selection->deselectAll();
        selection->select(shape);
    }
    // ugly hack, but if we don't first disable and than re-enable the text tool
    // it will still keep the wrong textshape selected
    KoToolManager::instance()->switchToolRequested(PanTool_ID);
    KoToolManager::instance()->switchToolRequested(TextTool_ID);

    KoResourceManager *provider = canvas->resourceManager();
    Q_CHECK_PTR(provider);

    QString sizeStr = QString::number(m_searchTextPositions.size());
    QString indexStr = QString::number(aIndex + 1);

    m_ui->actionSearchResult->setText(i18n("%1 of %2",aIndex + 1,m_searchTextPositions.size() ));

    provider->setResource(KoText::CurrentTextPosition,
                          m_searchTextPositions.at(aIndex).second.first);
    provider->setResource(KoText::CurrentTextAnchor,
                          m_searchTextPositions.at(aIndex).second.second);
}

void MainWindow::goToPreviousWord()
{
    if (documentType() == SpreadsheetDocument) {
        cellTool()->findPrevious();
        m_ui->actionSearchResult->setText(i18n("%1 of %2", cellTool()->currentSearchStatistics().first,
                                               cellTool()->currentSearchStatistics().second));
        return;
    }

    if (m_searchTextPositions.isEmpty())
        return;
    if (m_searchTextIndex == 0) {
        m_searchTextIndex = m_searchTextPositions.size() - 1;
    } else {
        m_searchTextIndex--;
    }
    highlightText(m_searchTextIndex);
}

void MainWindow::goToNextWord()
{
    if (documentType() == SpreadsheetDocument) {
        cellTool()->findNext();
        m_ui->actionSearchResult->setText(i18n("%1 of %2", cellTool()->currentSearchStatistics().first,
                                               cellTool()->currentSearchStatistics().second));
        return;
    }

    if (m_searchTextPositions.isEmpty())
        return;
    if (m_searchTextIndex == m_searchTextPositions.size() - 1) {
        m_searchTextIndex = 0;
    } else {
        m_searchTextIndex++;
    }

    highlightText(m_searchTextIndex);
}

void MainWindow::searchOptionChanged(int aCheckBoxState)
{
    m_searchCaseSensitive = aCheckBoxState == Qt::Checked;
    startSearch();
}

void MainWindow::updateActions()
{
    bool docLoaded = document();
    m_ui->actionSearch->setEnabled(docLoaded);
    m_ui->actionEdit->setEnabled(docLoaded);
    m_ui->actionFullScreen->setEnabled(docLoaded);
    m_ui->actionZoomIn->setEnabled(docLoaded);
    m_ui->actionZoomOut->setEnabled(docLoaded);
    m_ui->actionZoomLevel->setEnabled(docLoaded);
    m_ui->actionNextPage->setEnabled(docLoaded);
    m_ui->actionPrevPage->setEnabled(docLoaded);
    m_ui->actionFormat->setVisible(documentType() == TextDocument || documentType() == PresentationDocument);
    m_ui->actionEdit->setVisible(documentType() == TextDocument || documentType() == PresentationDocument);
    m_ui->actionSlidingMotion->setVisible(documentType() == PresentationDocument);
}

void MainWindow::setCentralWidget(QWidget *widget)
{
    QMainWindow::setCentralWidget(widget);
    switch (documentType()) {
    case TextDocument:
        break;
    case PresentationDocument:
        break;
    case SpreadsheetDocument:
        setUpSpreadEditorToolBar();
        break;
    default:
        break;
    };
}

QWidget* MainWindow::centralWidget() const
{
    return QMainWindow::centralWidget();
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ContextMenu)
        return true;

    // TODO: refine the collaborative-editing section of eventFilter
    if (textEditor() && m_collab) {
        if(event->type() == 6 && QString::compare("KWCanvas", watched->metaObject()->className())== 0 )
        {
            //qDebug()<<"111111111111111"<<watched->metaObject()->className();
            QKeyEvent *ke=static_cast<QKeyEvent *>(event);
            if( ke->key() == Qt::Key_Backspace )
                m_collab->sendBackspace(textEditor()->selectionStart(), textEditor()->selectionEnd());
            else if (((ke->key()>=Qt::Key_Space) && (ke->key()<=Qt::Key_AsciiTilde)) || ke->key() == Qt::Key_Return )
                m_collab->sendString(textEditor()->selectionStart(), textEditor()->selectionEnd(), ke->text().toUtf8());
            else
                qDebug() << "Collaborate: Unsupported key: " << ke->key() << " (" << ke->text() << ")";
        }
    }

    // show buttons in full screen mode if user taps anywhere in the screen
    if (event && isFullScreen()) {

        if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::TabletPress) {
            m_pressed = true;
            m_panningCount = 0;
            m_slideChangePossible = true;
            m_fsTimer->start(3000);
            m_pressPos = (reinterpret_cast<QMouseEvent*>(event))->pos();
        }
        else if (   event->type() == QEvent::MouseButtonRelease
                   || event->type() == QEvent::TabletRelease)
        {
            m_pressed = false;
            //show buttons only if user just tap the screen without
            //panning document
            if (m_panningCount <= 5) {
                m_fsButton->show();
                m_fsButton->raise();
#ifdef Q_WS_MAEMO_5

                //tap on the fullscreen starts and stops the scrolling functionality

                if(!stopAcceleratorScrolling)
                {
                    qDebug()<<"this is the scroll stoppping and starting option"<<stopAcceleratorScrolling;
                    switchToScroll();
                }
#endif
                if (documentType() == PresentationDocument) {
                    showFullScreenPresentationIcons();
                }
                m_fsTimer->start(3000);
                m_slideChangePossible = false;
            }
            m_panningCount = 0;
        }
        else if ((event->type() == QEvent::TabletMove ||
                    event->type() == QEvent::MouseMove) && m_pressed) {
            int sliderMin = controller()->verticalScrollBar()->minimum();
            int sliderVal = controller()->verticalScrollBar()->value();
            int sliderMax = controller()->verticalScrollBar()->maximum();
            if (sliderVal == sliderMin || sliderVal == sliderMax)
                m_panningCount++;
        }

        if (documentType() == PresentationDocument && m_slideChangePossible
                && m_panningCount > 5 && (event->type() == QEvent::MouseMove
                                          || event->type() == QEvent::TabletMove))
        {
            int sliderMin = controller()->verticalScrollBar()->minimum();
            int sliderVal = controller()->verticalScrollBar()->value();
            int sliderMax = controller()->verticalScrollBar()->maximum();
            QPoint movePos = (reinterpret_cast<QMouseEvent*>(event))->pos();
            if (movePos.y() - m_pressPos.y() > 50 && sliderVal == sliderMin && presentationTool() && ( ! presentationTool()->toolsActivated() ) ) {
                m_slideChangePossible = false;
                triggerAction("page_previous");
                emit previousSlide();
            }
            if (   m_pressPos.y() - movePos.y() > 50
                && sliderVal == sliderMax
                && presentationTool()
                && ( ! presentationTool()->toolsActivated() ) )
            {
                m_slideChangePossible = false;
                triggerAction("page_next");
                emit nextSlide();
            }
        }
    }
    // Maemo Qt hardcodes handling of F6 to toggling full screen directly, so
    // override that shortcut to do what we want it to do instead.
    if (event && event->type() == QEvent::Shortcut) {
        QShortcutEvent *qse = reinterpret_cast<QShortcutEvent*>(event);
        if (qse->key() == QKeySequence(Qt::Key_F6)) {
            if (m_ui->actionFullScreen->isEnabled())
                fullScreen();
            return true;
        } else if (qse->key() == QKeySequence(Qt::Key_F4)) {
            showApplicationMenu();
            return true;
        }
    } else if (event->type() == QEvent::ShortcutOverride && isFullScreen()) {
        // somehow shortcuts aren't working properly in full-screen mode...
        QKeyEvent* qke = reinterpret_cast<QKeyEvent*>(event);
        if (qke->key() == Qt::Key_F7) {
            m_ui->actionZoomIn->trigger();
            return true;
        } else if (qke->key() == Qt::Key_F8) {
            m_ui->actionZoomOut->trigger();
            return true;
        }
    }

    // change presentation slide in fullscreen mode if user taps on
    // left or right side of the screen
    if (document()) {
        if (   presentationTool() && !presentationTool()->toolsActivated()
            && watched && event && documentType() == PresentationDocument && pageCount() > 0
            && isFullScreen()
            && event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = reinterpret_cast<QMouseEvent*>(event);
            // check that event wasn't from full screen push button
            if (QString::compare("QPushButton", watched->metaObject()->className())) {
                QSize size(frameSize());
                if (mouseEvent->x() <= FS_BUTTON_SIZE) {
                    triggerAction("page_previous");
                    emit previousSlide();
                } else if (mouseEvent->x() >= (size.width() - FS_BUTTON_SIZE)) {
                    triggerAction("page_next");
                    emit nextSlide();
                }
            }
        }
    }

    if(event && event->type()==QEvent::MouseButtonPress && document()) {
        QMouseEvent *mouseEvent= reinterpret_cast<QMouseEvent*>(event);
        m_xcordinate = mouseEvent->globalX ();
        m_ycordinate = mouseEvent->globalY();
        if((m_formatframe) && (m_formatframe->isVisible())) {
            if ((m_xcordinate<480) || (m_ycordinate<205))
                m_formatframe->hide();
        }
        if((m_fontstyleframe) && (m_fontstyleframe->isVisible())) {
            if(!isActiveWindow()){
            } else {
                if((m_xcordinate<325) || (m_ycordinate<205))
                    m_fontstyleframe->hide();
            }
        }
    }

    if (document() && event && event->type() == 7 && documentType() == TextDocument) {
         QKeyEvent *ke=static_cast<QKeyEvent *>(event);
         if(ke->key()!=Qt::Key_Up && ke->key()!=Qt::Key_Down &&
            ke->key()!=Qt::Key_Right && ke->key()!=Qt::Key_Left &&
            ke->key()!=Qt::Key_Shift && ke->key()!=Qt::Key_Alt &&
            ke->key()!=Qt::Key_Control && ke->key()!=Qt::Key_CapsLock &&
            ke->key()!=Qt::Key_Backspace ){
            setDocumentModified(true);
            if(m_firstChar && centralWidget()->hasFocus()){
                 centralWidget()->clearFocus();
                 controller()->setFocus();
                 m_firstChar=false;
            }
         }
    }

    if (textEditor() && event->type() == QEvent::MouseButtonDblClick && isActiveWindow()) {
        if(m_ui->actionEdit->isChecked())
            textEditor()->setPosition(textEditor()->position(),QTextCursor::MoveAnchor);
        return true;
    }
    if (event->type() == QEvent::MouseButtonDblClick && documentType() == SpreadsheetDocument) {
        setDocumentModified(true);
        return false;
    }
    if (event->type() == QEvent::MouseButtonDblClick && documentType() == PresentationDocument) {
        if(m_ui->actionEdit->isChecked()){
            KoToolManager::instance()->switchToolRequested(TextTool_ID);
        }
        else {
            KoToolManager::instance()->switchToolRequested(PanTool_ID);
        }
    }
  return false;
    //return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showApplicationMenu()
{
    HildonMenu menu(this);
    menu.exec();
}

void MainWindow::activeToolChanged(KoCanvasController* canvas, int)
{
   QObject *canvasObject = dynamic_cast<QObject*>(canvas);
   QString newTool = KoToolManager::instance()->activeToolId();

   // only Pan tool or Text tool should ever be the active tool, so if
   // another tool got activated, switch back to pan tool
    if (newTool != PanTool_ID && newTool != TextTool_ID && newTool != CellTool_ID) {
        KoToolManager::instance()->switchToolRequested(PanTool_ID);
    }
    canvasObject->setProperty("FingerScrollable", true);
}

//Function to check if application has been started by DBus
void MainWindow::checkDBusActivation()
{
    if (splash() && !isActiveWindow())
        openDocument();
}

void MainWindow::pluginOpen(bool /*newWindow*/, const QString& path)
{
    openDocument(path,false);
}

void MainWindow::menuClicked(QAction* action)
{
    if (!action->data().toBool())
    {
        return; // We return if it was not a plugin action
    }

    const QString activeText = action->text();
    OfficeInterface *nextPlugin = loadedPlugins[activeText];
    nextPlugin->setDocument(document());
    QWidget *pluginView = nextPlugin->view();
    pluginView->setParent(this);
    pluginView->setWindowFlags(Qt::Dialog);
    pluginView->show();
}

void MainWindow::loadScrollAndQuit()
{
    // if no document is loaded, simply quit
    if (!document() || !controller()) {
        QTimer::singleShot(1, qApp, SLOT(quit()));
        return;
    }
    // if still loading, wait some more
    if (document()->isLoading()) {
        QTimer::singleShot(100, this, SLOT(loadScrollAndQuit()));
        return;
    }
    // when done loading, start proceeding to the next page until the end of the document has been
    // reached
    bool done;
    if (documentType() == PresentationDocument) {
        done = currentPage() == document()->pageCount();
    } else {
        QScrollBar *v = controller()->verticalScrollBar();
        done = v->value() >= v->maximum();
    }
    if (done) {
        QTimer::singleShot(1, qApp, SLOT(quit()));
    } else {
        goToNextPage();
        QTimer::singleShot(20, this, SLOT(loadScrollAndQuit()));
    }
}

void MainWindow::mousePressEvent( QMouseEvent *event )
{
    if( presentationTool() && presentationTool()->toolsActivated() ) {
        presentationTool()->handleMainWindowMousePressEvent( event );
    }
}

void MainWindow::mouseMoveEvent( QMouseEvent *event )
{
    if( presentationTool() && presentationTool()->toolsActivated() ) {
        presentationTool()->handleMainWindowMouseMoveEvent( event );
    }
}

void MainWindow::mouseReleaseEvent( QMouseEvent *event )
{
    if( presentationTool() && presentationTool()->toolsActivated() ) {
        presentationTool()->handleMainWindowMouseReleaseEvent( event );
    }
}

void MainWindow::paintEvent( QPaintEvent */*event*/ )
{
    if( !presentationTool() ) {
        return;
    }

    if( presentationTool()->toolsActivated() ) {
        QPainter painter(this);
        QRectF target(0,0,800,480);
        QRectF source(0,0,800,480);
        painter.drawImage( target,*( presentationTool()->getImage() ), source );
    }

    if( ( !presentationTool()->getPenToolStatus() ) && ( !presentationTool()->getHighlightToolStatus() ) && controller()) {
        controller()->show();
    }
}

void MainWindow::disableFullScreenPresentationNavigation()
{
    disconnect( m_fsPPTBackButton, SIGNAL( clicked() ), this, SLOT( goToPreviousPage() ) );
    disconnect( m_fsPPTForwardButton, SIGNAL( clicked() ), this, SLOT( goToNextPage() ) );
    m_fsPPTBackButton->hide();
    m_fsPPTForwardButton->hide();
}

void MainWindow::enableFullScreenPresentationNavigation()
{
    connect( m_fsPPTBackButton, SIGNAL( clicked() ), this, SLOT( goToPreviousPage() ) );
    connect( m_fsPPTForwardButton, SIGNAL( clicked() ), this, SLOT( goToNextPage() ) );
    m_fsPPTBackButton->show();
    m_fsPPTBackButton->raise();
    m_fsPPTForwardButton->show();
    m_fsPPTForwardButton->raise();
}

void MainWindow::setUpSpreadEditorToolBar()
{
    if (documentType() != SpreadsheetDocument)
        return;
    if (m_spreadEditToolBar) {
        delete m_spreadEditToolBar;
        m_spreadEditToolBar=0;
    }
    m_spreadEditToolBar = new QToolBar();
    m_spreadEditToolBar->setIconSize(QSize(48,48));
    addToolBar(Qt::BottomToolBarArea,m_spreadEditToolBar);
    m_spreadEditToolBar->addAction(m_ui->actionEdit);
    m_spreadEditToolBar->addSeparator();
    m_spreadEditToolBar->addAction(m_ui->actionCut);
    m_spreadEditToolBar->addAction(m_ui->actionCopy);
    m_spreadEditToolBar->addAction(m_ui->actionPaste);
    m_spreadEditToolBar->addAction(m_ui->actionUndo);
    m_spreadEditToolBar->addAction(m_ui->actionRedo);
    m_spreadEditToolBar->addAction(m_ui->actionStyle);
    m_spreadEditToolBar->addAction(m_ui->actionFormat);

    cellTool()->externalEditor()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    m_spreadEditToolBar->insertWidget(m_ui->actionCut, cellTool()->externalEditor());

    if(m_addAction) {
        delete m_addAction;
        m_addAction=0;
    }
    m_addAction= new QAction(this);
    m_addAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Plus.png"));
    if(m_subtractAction) {
        delete m_subtractAction;
        m_subtractAction=0;
    }
    m_subtractAction= new QAction(this);
    m_subtractAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Subtract.png"));
    if(m_multiplyAction) {
        delete m_multiplyAction;
        m_multiplyAction=0;
    }
    m_multiplyAction= new QAction(this);
    m_multiplyAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Multiply.png"));
    if(m_divideAction) {
        delete m_divideAction;
        m_divideAction=0;
    }
    m_divideAction= new QAction(this);
    m_divideAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Divide.png"));
    if(m_percentageAction) {
        delete m_percentageAction;
        m_percentageAction=0;
    }
    m_percentageAction= new QAction(this);
    m_percentageAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Percentage.png"));
    if(m_equalsAction) {
        delete m_equalsAction;
        m_equalsAction=0;
    }
    m_equalsAction= new QAction(this);
    m_equalsAction->setIcon(QIcon(":/images/48x48/Edittoolbaricons/Equal.png"));

    m_spreadEditToolBar->addAction(m_addAction);
    m_spreadEditToolBar->addAction(m_subtractAction);
    m_spreadEditToolBar->addAction(m_multiplyAction);
    m_spreadEditToolBar->addAction(m_divideAction);
    m_spreadEditToolBar->addAction(m_percentageAction);
    m_spreadEditToolBar->addAction(m_equalsAction);

    if(m_signalMapper) {
        delete m_signalMapper;
        m_signalMapper=0;
    }
    m_signalMapper= new QSignalMapper(this);
    m_signalMapper->setMapping(m_addAction,QString("+"));
    m_signalMapper->setMapping(m_subtractAction,QString("-"));
    m_signalMapper->setMapping(m_multiplyAction,QString("*"));
    m_signalMapper->setMapping(m_divideAction,QString("/"));
    m_signalMapper->setMapping(m_percentageAction,QString("%"));
    m_signalMapper->setMapping(m_equalsAction,QString("="));

    connect(m_addAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    connect(m_subtractAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    connect(m_multiplyAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    connect(m_divideAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    connect(m_percentageAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    connect(m_equalsAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));

    connect(m_signalMapper,SIGNAL(mapped(QString)),this,SLOT(addMathematicalOperator(QString)));

    m_addAction->setVisible(false);
    m_subtractAction->setVisible(false);
    m_multiplyAction->setVisible(false);
    m_divideAction->setVisible(false);
    m_percentageAction->setVisible(false);
    m_equalsAction->setVisible(false);

    m_spreadEditToolBar->addAction(m_ui->actionMathOp);
    m_ui->actionMathOp->setVisible(true);
    m_spreadEditToolBar->hide();
}

void MainWindow::resetSpreadEditorToolBar()
{
    if (documentType() != SpreadsheetDocument)
        return;
    m_ui->actionMathOp->setChecked(false);
    m_ui->actionCut->setVisible(true);
    m_ui->actionCopy->setVisible(true);
    m_ui->actionPaste->setVisible(true);
    m_ui->actionUndo->setVisible(true);
    m_ui->actionRedo->setVisible(true);
    m_ui->actionStyle->setVisible(true);
    m_ui->actionFormat->setVisible(true);
    cellTool()->externalEditor()->hide();
    cellTool()->externalEditor()->releaseKeyboard();

    disconnect(m_signalMapper,SIGNAL(mapped(QString)),this,SLOT(addMathematicalOperator(QString)));
    disconnect(m_addAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    disconnect(m_subtractAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    disconnect(m_multiplyAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    disconnect(m_divideAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    disconnect(m_percentageAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));
    disconnect(m_equalsAction,SIGNAL(triggered()),m_signalMapper,SLOT(map()));

    removeToolBar(m_spreadEditToolBar);

    delete m_addAction;
    m_addAction=0;
    delete m_subtractAction;
    m_subtractAction=0;
    delete m_multiplyAction;
    m_multiplyAction=0;
    delete m_divideAction;
    m_divideAction=0;
    delete m_percentageAction;
    m_percentageAction=0;
    delete m_equalsAction;
    m_equalsAction=0;
    delete m_spreadEditToolBar;
    m_spreadEditToolBar=0;
}

void MainWindow::startMathMode(bool start)
{
    if (documentType() != SpreadsheetDocument)
        return;
    m_addAction->setVisible(start);
    m_subtractAction->setVisible(start);
    m_multiplyAction->setVisible(start);
    m_divideAction->setVisible(start);
    m_percentageAction->setVisible(start);
    m_equalsAction->setVisible(start);
    m_ui->actionCut->setVisible(!start);
    m_ui->actionCopy->setVisible(!start);
    m_ui->actionPaste->setVisible(!start);
    m_ui->actionUndo->setVisible(!start);
    m_ui->actionRedo->setVisible(!start);
    m_ui->actionStyle->setVisible(!start);
    m_ui->actionFormat->setVisible(!start);
}

///////////////////////////
// Collaborative editing //
///////////////////////////

void MainWindow::collaborateDialog()
{
    if(m_collab)
    {
        QMessageBox::warning(this,"Collaborative Editing",QString("An active session already exists"),QMessageBox::Ok);
        return ;
    }
    else
    {
        if (m_collabDialog) {
            delete m_collabDialog;
            m_collabDialog = 0;
        }
        m_collabDialog = new CollabDialog(this);

        connect(m_collabDialog, SIGNAL(accepted()), this, SLOT(startCollaborating()));
        //connect(m_collabDialog, SIGNAL(finished(int)), this, SLOT(startCollaborating(int)));
        connect(m_collabDialog, SIGNAL(rejected()), this, SLOT(collaborationCancelled()));

        m_collabDialog->show();
    }

}

void MainWindow::closeCollabDialog() {
    qDebug() << "Collaborate-Dialog: closeDialog() called";
    //delete m_collabDialog;
    //m_collabDialog = 0;
    m_collabDialog->hide();
    qDebug() << "Collaborate-Dialog: closeDialog() done";
}

void MainWindow::startCollaborating() {
    qDebug() << "Collaborate-Dialog: start ";
    if (m_collabDialog->isClient()) {
        if (document()) {
            qDebug() << "A document is already open. Cannot start client.";
            return closeCollabDialog();
        }
        m_collab = new CollabClient(m_collabDialog->getNick(),
                                    m_collabDialog->getAddress(),
                                    m_collabDialog->getPort(),
                                    this);
        connect(m_collab, SIGNAL(openFile(const QString&)), this, SLOT(collabOpenFile(QString)));

    } else if (m_collabDialog->isServer()) {
        if (!document()) {
            qDebug() << "No document is open. Cannot start server.";
            return closeCollabDialog();
        }
        m_collab = new CollabServer(m_collabDialog->getNick(),
                                    m_fileName,
                                    m_collabDialog->getPort(),
                                    this);
        connect(m_collab, SIGNAL(saveFile(const QString&)), this, SLOT(collabSaveFile(const QString&)));
        m_collabEditor = new KoTextEditor(textEditor()->document()); // qobject_cast<KoTextEditor*>(qobject_cast<KWView*>(document()->createView(this))->kwcanvas()->toolProxy()->selection());

    } else {
        return closeCollabDialog();
    }

    //qDebug() << "888888888888888888888888888888888888888";
    //m_collabEditor =
    //qDebug() << "999999999999999999999999999999999999999";

    connect(m_collab, SIGNAL(receivedBackspace(uint,uint)), this, SLOT(receivedBackspace(uint,uint)));
    connect(m_collab, SIGNAL(receivedFormat(uint,uint,Collaborate::FormatFlag)), this, SLOT(receivedFormat(uint,uint,Collaborate::FormatFlag)));
    connect(m_collab, SIGNAL(receivedString(uint,uint,QByteArray)), this, SLOT(receivedString(uint,uint,QByteArray)));
    connect(m_collab, SIGNAL(receivedFontSize(uint,uint,uint)), this, SLOT(receivedFontSize(uint,uint,uint)));
    connect(m_collab, SIGNAL(receivedTextColor(uint,uint,QRgb)), this, SLOT(receivedTextColor(uint,uint,QRgb)));
    connect(m_collab, SIGNAL(receivedTextBackgroundColor(uint,uint,QRgb)), this, SLOT(receivedTextBackgroundColor(uint,uint,QRgb)));
    connect(m_collab, SIGNAL(receivedFontType(uint,uint,QString)), this, SLOT(receivedFontType(uint,uint,QString)));
    connect(m_collab, SIGNAL(error(quint16)), this, SLOT(error(quint16)));

    closeCollabDialog();
    qDebug() << "Collaborate-Dialog: end ----------------------------------------- ";
}

void MainWindow::collaborationCancelled() {
    closeCollabDialog();
}

void MainWindow::receivedString(uint start, uint end, QByteArray msg) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    m_collabEditor->insertText(msg);
}

void MainWindow::receivedBackspace(uint start, uint end) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    m_collabEditor->deletePreviousChar();
}

void MainWindow::receivedFormat(uint start, uint end, Collaborate::FormatFlag format) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);

    switch(format) {
    case Collaborate::FormatAlignCenter:
        setCenterAlign(m_collabEditor);
        break;
    case Collaborate::FormatAlignJustify:
        setJustify(m_collabEditor);
        break;
    case Collaborate::FormatAlignLeft:
        setLeftAlign(m_collabEditor);
        break;
    case Collaborate::FormatAlignRight:
        setRightAlign(m_collabEditor);
        break;
    case Collaborate::FormatBold:
        setBold(m_collabEditor);
        break;
    case Collaborate::FormatItalic:
        setItalic(m_collabEditor);
        break;
    case Collaborate::FormatListBullet:
        setBulletList(m_collabEditor);
        break;
    case Collaborate::FormatListNumber:
        setNumberList(m_collabEditor);
        break;
    case Collaborate::FormatSubScript:
        setSubScript(m_collabEditor);
        break;
    case Collaborate::FormatSuperScript:
        setSuperScript(m_collabEditor);
        break;
    case Collaborate::FormatUnderline:
        setUnderline(m_collabEditor);
        break;
    default:
        qDebug() << "Collaborate: Unknown format flag";
    }
}

void MainWindow::receivedFontSize(uint start, uint end, uint size) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    setFontSize(size, m_collabEditor);
}

void MainWindow::receivedFontType(uint start, uint end, const QString &font) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    setFontType(font, m_collabEditor);
}

void MainWindow::receivedTextColor(uint start, uint end, QRgb color) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    setTextColor(QColor(color), m_collabEditor);
}

void MainWindow::receivedTextBackgroundColor(uint start, uint end, QRgb color) {
    m_collabEditor->setPosition(start);
    m_collabEditor->setPosition(end, QTextCursor::KeepAnchor);
    setTextBackgroundColor(QColor(color), m_collabEditor);
}

void MainWindow::error(quint16 err) {
    qDebug() << "Collaborate: Error: " << err;
}

void MainWindow::collabSaveFile(const QString &filename) {
    document()->saveNativeFormat(filename);
}

void MainWindow::collabOpenFile(const QString &filename) {
    m_fileName = filename;
    openDocument(filename,false);
    qDebug() << "============================================";
    m_collabEditor = new KoTextEditor(textEditor()->document()); // qobject_cast<KoTextEditor*>(qobject_cast<KWView*>(document()->createView(this))->kwcanvas()->toolProxy()->selection());
    qDebug() << "::::::::::::::::::::::::::::::::::::::::::::";
}

///////////////////////////
///////////////////////////
#ifdef HAVE_OPENGL
void MainWindow::glPresenter()
{
    if (documentType() == PresentationDocument) {
    /*
    int i=0;
    QList <QPixmap> pixm;
    while(i < document()->pageCount())
    {
        KoPADocument* slide = qobject_cast<KoPADocument*>(m_doc);
        KoPAPageBase* cur_slide = slide->pageByIndex(i, false);
        pixm << cur_slide->thumbnail();
        i++;
    }
    */
        presenter = new GLPresenter(this, gl_style, gl_showtime, storeButtonPreview()->thumbnailList());
    }
}

void MainWindow::glPresenterSet(int a , int b)
{
    gl_showtime = b * 1000;
    gl_style = a;
}
#endif
void MainWindow::showDigitalSignatureInfo()
{
    if (m_digitalSignatureDialog){
        m_digitalSignatureDialog->initializeDialog();
        m_digitalSignatureDialog->exec();
    }
}

void MainWindow::enableSelectTool()
{
    KoToolManager::instance()->switchToolRequested("InteractionTool");
}

void MainWindow::insertImage()
{
    KoShape *shape;
    KoShapeCreateCommand *cmd;
    KoSelection *selection;
    if (documentType() == TextDocument) {
        KWDocument* kwDoc = qobject_cast<KWDocument*>(document());
        shape = FoImageSelectionWidget::selectImageShape(kwDoc->resourceManager(), this);
        if (shape) {
            KWView* kwView = qobject_cast<KWView*>(document()->createView(this));
            if (currentPage()) {
                KWPage kwpage = kwordView()->currentPage();
                QRectF page = kwpage.rect();
                // make the shape be on the current page, and fit inside the current page.
                if (page.width() < shape->size().width() || page.height() < shape->size().height()) {
                    QSizeF newSize(page.width() * 0.7, page.height() * 0.7);
                    const qreal xRatio = newSize.width() / shape->size().width();
                    const qreal yRatio = newSize.height() / shape->size().height();
                    if (xRatio > yRatio) // then lets make the vertical set the size.
                        newSize.setWidth(shape->size().width() * yRatio);
                    else
                        newSize.setHeight(shape->size().height() * xRatio);
                    shape->setSize(newSize);
                }
                shape->setPosition(page.topLeft());
                int zIndex = 0;
                foreach (KoShape *s, kwView->canvasBase()->shapeManager()->shapesAt(page))
                    zIndex = qMax(s->zIndex(), zIndex);
                shape->setZIndex(zIndex+1);
            }
            cmd = new KoShapeCreateCommand(kwDoc, shape);
            selection = kwView->canvasBase()->shapeManager()->selection();
            selection->deselectAll();
            selection->select(shape);
            document()->addCommand(cmd);
        }
    }
    if (documentType() == PresentationDocument) {
        KoPADocument *prDoc = qobject_cast<KoPADocument *>(document());
        int pageIndex = controller()->canvas()->resourceManager()->resource(KoCanvasResource::CurrentPage).toInt() - 1;
        KoPAPageBase *kprpage = prDoc->pageByIndex( pageIndex, false);
        KoShapeLayer *layer = dynamic_cast<KoShapeLayer *>(kprpage->shapes().first());
        shape = FoImageSelectionWidget::selectImageShape(prDoc->resourceManager(), this);
        if(!shape)
            return;
        shape->setParent(&(*layer));
        int zIndex = 0;
        QRectF page = kprpage->contentRect();
        if (page.width() < shape->size().width() || page.height() < shape->size().height()) {
            QSizeF newSize(page.width() * 0.7, page.height() * 0.7);
            const qreal xRatio = newSize.width() / shape->size().width();
            const qreal yRatio = newSize.height() / shape->size().height();
            if (xRatio > yRatio) // then lets make the vertical set the size.
                newSize.setWidth(shape->size().width() * yRatio);
            else
                newSize.setHeight(shape->size().height() * xRatio);
            shape->setSize(newSize);
        }
        foreach (KoShape *s, controller()->canvas()->shapeManager()->shapesAt(page))
            zIndex = qMax(s->zIndex(), zIndex);
        shape->setZIndex(zIndex);
        //        shape->setPosition(page.center());
        cmd = new KoShapeCreateCommand(prDoc, shape);
        prDoc->addCommand(cmd);
    }
}

void MainWindow::insertButtonClicked()
{
    m_ui->actionInsert->setVisible(false);
    m_ui->actionInsertImage->setVisible(true);
}

void MainWindow::showCCP()
{
    if(!m_ui->actionShowCCP->isChecked()) {
        m_ui->actionShowCCP->setChecked(false);
        m_ui->actionCopy->setVisible(false);
        m_ui->actionCut->setVisible(false);
        m_ui->actionPaste->setVisible(false);
        return;
    }
    m_ui->actionShowCCP->setChecked(true);
    m_ui->actionCopy->setVisible(true);
    m_ui->actionCut->setVisible(true);
    m_ui->actionPaste->setVisible(true);
}

void MainWindow::insertNewTextShape()
{
    KoShapeFactoryBase *factory = KoShapeRegistry::instance()->value( "TextShapeID" );
    Q_ASSERT( factory );
    KoShape *textShape = factory->createDefaultShape();
    KoCanvasBase *canvasForInserting = controller()->canvas();
    Q_CHECK_PTR(canvasForInserting);
    int curPage = canvasForInserting->resourceManager()->resource(KoCanvasResource::CurrentPage).toInt() - 1;
    KoPADocument* paDocForInserting = qobject_cast<KoPADocument*>(document());
    KoPAPageBase* paPageForInserting = paDocForInserting->pageByIndex(curPage, false);
    KoShapeContainer *container =paPageForInserting;
    textShape->setParent(container);
   // textShape->setPosition (QPointF(m_xcordinate,m_ycordinate));
    paDocForInserting->addShape(textShape);
    textShape->setVisible(true);
}

void MainWindow::insertNewTable()
{
    KoCanvasBase *canvasForChecking = controller()->canvas();
    Q_CHECK_PTR(canvasForChecking);
    qDebug()<<"selection:"<<canvasForChecking->toolProxy()->selection()->hasSelection();
    if(canvasForChecking->toolProxy()->selection()->hasSelection())
    {
    KoPAView *m_kopaview = qobject_cast<KoPAView *>(view());
    m_kopaview->kopaCanvas()->toolProxy()->actions()["insert_table"]->trigger();
    }
}

// KoAbstractApplication implementation

void MainWindow::setWindowTitle(const QString& title)
{
    QMainWindow::setWindowTitle(title);
}

void MainWindow::showMessage(KoAbstractApplication::MessageType type, const QString& messageText)
{
    switch (type) {
    case UnsupportedFileTypeMessage: {
        NotifyDialog dialog(this);
        dialog.exec();
        break;
    }
    case InformationMessage:
    case InformationWithoutTimeoutMessage: {
#ifdef Q_WS_MAEMO_5
        const int timeout = (type == InformationMessage) ? QMaemo5InformationBox::DefaultTimeout
            : QMaemo5InformationBox::NoTimeout;
        QMaemo5InformationBox::information(0, messageText, timeout);
#else
        QMessageBox msgBox;
        msgBox.setText(messageText);
        msgBox.exec();
#endif
        break;
    }
    default:
        break;
    }
}

void MainWindow::startNewInstance(const QString& fileName, bool isNewDocument)
{
    QStringList args;
    args << fileName;
    args << (isNewDocument ? "true" : "false");
    QProcess::startDetached(FREOFFICE_APPLICATION_PATH, args);
}

void MainWindow::setProgressIndicatorVisible(bool visible)
{
    unsigned long val = visible ? 1 : 0;
    Atom atom = XInternAtom(QX11Info::display(),
                            "_HILDON_WM_WINDOW_PROGRESS_INDICATOR", False);
    XChangeProperty(QX11Info::display(), winId(), atom, XA_INTEGER,
                    32, PropModeReplace,
                    (unsigned char *) &val, 1);
}

void MainWindow::showUiBeforeDocumentOpening(bool isNewDocument)
{
    m_ui->viewToolBar->show();
    m_ui->actionSlidingMotion->setVisible(false);
    m_ui->actionMathOp->setVisible(false);
    m_ui->actionFormat->setVisible(false);
    m_ui->actionSlidingMotion->setVisible(false);
    m_ui->actionSearch->setChecked(false);

    switch (documentType()) {
    case PresentationDocument:
        m_ui->actionInsertTextShape->setVisible(true);
        m_ui->actionInsertTable->setVisible(true);
        break;
    case SpreadsheetDocument:
        m_ui->actionEdit->setVisible(true);
        break;
    default:;
    }

   if (documentType() == TextDocument && !isNewDocument) {
       m_digitalSignatureDialog = new DigitalSignatureDialog(m_fileName);
        if (m_digitalSignatureDialog->verifySignature()){
            QMenuBar* menu = menuBar();
            menu->insertAction(m_ui->actionClose,m_ui->actionDigitalSignature);
            connect(m_ui->actionDigitalSignature,SIGNAL(triggered()),this,SLOT(showDigitalSignatureInfo()));
        }
   }
#ifdef Q_WS_MAEMO_5
   if (documentType() == TextDocument)
   {
       /*
        * intialising the rdf
        */
       KoStore::Backend backend = KoStore::Auto;
       KoStore * store = KoStore::createStore(fileName, KoStore::Read, "", backend);

       m_foDocumentRdf = new FoDocumentRdf(document(), textEditor());
       if (!m_foDocumentRdf->loadOasis(store)) {
           delete m_foDocumentRdf;
           m_foDocumentRdf = 0;
           return false;
       }
       delete store;
       m_rdfShortcut= new QShortcut(QKeySequence(("Ctrl+R")),this);
       connect(m_rdfShortcut,SIGNAL(activated()),m_foDocumentRdf,SLOT(highlightRdf()));
   }
#endif

    raiseWindow();
}

QString MainWindow::applicationName() const
{
    return i18n("FreOffice");
}

QMessageBox::StandardButton MainWindow::askQuestion(QuestionType type, const QString& messageText)
{
    switch (type) {
    case SaveDiscardCancelQuestion: {
        ConfirmationDialog dlg(this);
        return static_cast<QMessageBox::StandardButton>(dlg.exec());
    }
    case ConfirmSheetDeleteQuestion:
        return QMessageBox::question(this, i18n("Confirm Delete"),
                                     messageText,
                                     QMessageBox::Yes | QMessageBox::No);
    default:
        kFatal() << "Unsupported question type";
    }
    return QMessageBox::Cancel;
}

void MainWindow::currentPageChanged()
{
    if (documentType() == PresentationDocument && isFullScreen()) {
        if (currentPage() == 1)
            m_fsPPTBackButton->hide();
        else if (currentPage() > 1)
            m_fsPPTBackButton->show();
        if (currentPage() == document()->pageCount())
            m_fsPPTForwardButton->hide();
        else if (currentPage() < document()->pageCount())
            m_fsPPTForwardButton->show();
    }
    const QString pageNo = i18n("pg%1 - pg%2", QString::number(currentPage()), QString::number(pageCount()));
    m_ui->actionPageNumber->setText(pageNo);
}

#include "MainWindow.moc"
