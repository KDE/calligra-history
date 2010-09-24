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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <KoDocument.h>
#include <KWView.h>
#include <styles/KoListStyle.h>


#include "Splash.h"
#include "PreviewDialog.h"
#include "NotesDialog.h"
#include "SlidingMotionDialog.h"

#include "CollabClient.h"
#include "CollabDialog.h"
#include "CollabServer.h"
#include "FoCellTool.h"
#include "VirtualKeyBoard.h"
#include "FoCellToolFactory.h"
#include "DigitalSignatureDialog.h"
#include "KoApplicationHelper.h"

#include <KoPAView.h>
#include <KoPACanvas.h>
#include <KoTextShapeData.h>
#include <KoTextDocument.h>
#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>

#include <KoPAView.h>
#include <KoPACanvas.h>
#include <KoTextShapeData.h>
#include <KoTextDocument.h>
#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>

#ifdef Q_WS_MAEMO_5
#include "FoDocumentRdf.h"
#include "Accelerator.h"
#endif


class QPushButton;
class QIcon;
class QTextDocument;
class QToolButton;
class QFrame;
class QLabel;
class QMessageBox;
class QLineEdit;
class QTextCursor;
class QShortcut;
class QCheckBox;
class QComboBox;
class QFontComboBox;
class QTextListFormat;
class QTextDocument;
class QGridLayout;
class QDialog;
class QListWidget;
class QListWidgetItem;
class SlidingMotionDialog;
#ifdef HAVE_OPENGL
class GLPresenter;
#endif
class FoCellTooFactory;

class KUndoStack;
class KoTextEditor;
class PresentationTool;
class MainWindowAdaptor;
class KoCanvasController;
class KoCanvasControllerWidget;
class KoShape;
class KoPAPageBase;

class OfficeInterface;

namespace Ui
{
class MainWindow;
}


/*!
 * \brief Main window of the application. KoCanvasController is set as
 * the central widget. It displays the loaded documents.
 */
class MainWindow : public QMainWindow, private KoApplicationHelper
{
    Q_OBJECT

public:
    MainWindow(Splash *aSplash, QWidget *parent = 0);
    ~MainWindow();
    //void tabletEvent ( QTabletEvent * event );
    void mousePressEvent ( QMouseEvent * event );
    void mouseMoveEvent ( QMouseEvent * event );
    void mouseReleaseEvent ( QMouseEvent * event );
    void paintEvent( QPaintEvent * event );

    void disableFullScreenPresentationNavigation();
    void enableFullScreenPresentationNavigation();
    void openNewDocumentType(QString type);

protected:
        void closeEvent(QCloseEvent *);

private:
    Ui::MainWindow *m_ui;
    FoCellToolFactory *m_cellToolFactory;


    
///////////////////////////
// Collaborative editing //
///////////////////////////

// TODO: Better error handling

private:
    CollabDialog* m_collabDialog;
    Collaborate* m_collab;
    KoTextEditor* m_collabEditor;

    void closeCollabDialog();

private slots:
    void startCollaborating();
    void collaborationCancelled();
    void collaborateDialog();

    void receivedFontSize(uint start, uint end, uint size);
    void receivedTextColor(uint start, uint end, QRgb color);
    void receivedTextBackgroundColor(uint start, uint end, QRgb color);
    void receivedFontType(uint start, uint end, const QString &font);
    void receivedString(uint start, uint end, QByteArray msg);
    void receivedBackspace(uint start, uint end);
    void receivedFormat(uint start, uint end, Collaborate::FormatFlag format);
    void error(quint16 err);

    void collabSaveFile(const QString &filename);
    void collabOpenFile(const QString &filename);

/////////////////////////////
/////////////////////////////

    //presentation editing
    private:
        QTextDocument *document;
        QPointer<KoTextEditor> m_pEditor;
        KoShape *m_textShape;
        KoShape *currentShapeSelected;
        KoTextShapeData *currentSelectedTextShapeData;
        QTextDocument *documentForCurrentShape;

        KoPAView *m_kopaview;
    private slots:
        void insertNewTextShape();
        void insertNewTable();
//        void deleteSelection();


    ////

/////virtualkeyboard

private:

    QShortcut *shortcutForVirtualKeyBoard;
    QShortcut *spaceHandlerShortcutForVirtualKeyBoard;
    static bool virtualKeyBoardIsOnScreen;


private slots:

    void showVirtualKeyBoardOnScreen();
    void spaceHandlerForVirtualKeyBoard();

    ////////////////////
    ////////////////////////////////////
    // Accelerator scrolling ,sliding //
    ////////////////////////////////////
#ifdef Q_WS_MAEMO_5

private:

/*!
 * true if scrolling start
 */

static bool stopAcceleratorScrolling;
/*!
 * true if slide transition starts
 */
static bool enableAccelerator;
/*!
 * true if scrolling starts
 */
static bool enableScrolling;


private:
/*!
 *  For accessing the accelerator
 */
AcceleratorScrollSlide acceleratorForScrolAndSlide;
/*!
 * Pointer to fullscreen Accelerometer Button
 */
QPushButton *m_fsAccButton;

/*!
 * shortcut for enabling the gestures
 */
QShortcut *shortcutForAccelerator;



private slots:
/*!
 * Slot to toggle between the slide transition
 */
void switchToSlid();
/*!
 * Slot to toggle between the scrolling
 */
void switchToScroll();
/*!
 * Slot for the scroll action
 */
void scrollAction();
/*!
 * Slot for closing all the accelerator settings
 */
void closeAcceleratorSettings();
/*!
 * Slot for Enabling and disabling the scroll
 */
void  enableDisableScrollingOption();

friend void AcceleratorScrollSlide::ifYesScroll();

friend void AcceleratorScrollSlide::ifNoScroll();

#endif
////////////////////////////////
////////////////////////////////

private:
   /*!
      * Holds the properties of presentation style and time
      */
    int gl_showtime;
    int gl_style;
    /*!
     * X-cordinate and Y-cordinate values at mouse click position
     */
    int m_xcordinate ;
    int m_ycordinate ;
    /*!
     * Font related information for each Character in NewDocuments 
     */
    int m_fontsize;
    /*!
     * Dialog for fontsize
     */
    QDialog *m_fontSizeDialog;
    QVBoxLayout *m_fontSizeDialogLayout;
    QLineEdit *m_fontSizeLineEdit;
    QListWidget *m_fontSizeList;
    /*!
     * Format frame declaration
     */
    QFrame * m_formatframe;
    QGridLayout * m_formatframelayout;
    QPushButton * m_alignleft;
    QPushButton * m_alignright;
    QPushButton * m_aligncenter;
    QPushButton * m_numberedlist;
    QPushButton * m_bulletlist;
    QPushButton * m_alignjustify;
    /*!
     * Font style frame declaration
     */
    QFrame * m_fontstyleframe;
    QGridLayout *m_fontstyleframelayout;
    QFontComboBox *m_fontcombobox;
    QPushButton *m_fontsizebutton;
    QPushButton *m_textcolor;
    QPushButton *m_textbackgroundcolor;
    QPushButton *m_superscript;
    QPushButton *m_subscript;
    QPushButton * m_bold;
    QPushButton * m_italic;
    QPushButton * m_underline;
    /*!
     * New document chooser dialog
     */
    QDialog * m_docdialog;
    QGridLayout * m_docdialoglayout;
    QToolButton * m_document;
    QToolButton * m_presenter;
    QToolButton * m_spreadsheet;
    /*!
     * Presentation Template chooser dialog
     */
    QDialog * m_tempselectiondialog;
    QGridLayout * m_tempdialoglayout;
    QListWidget * m_templateWidget;
    QPushButton * m_go;
    QPushButton * m_closetemp;
    QLabel * m_templatepreview;
    QStringList m_temptitle;
    QStringList m_templatepath;
    QString newpresenter;
    /*!
     * Confirmation dialog for closing new document
     */
    QDialog *m_confirmationdialog;
    QGridLayout *m_confirmationdialoglayout;
    QPushButton *m_yes;
    QPushButton *m_no;
    QPushButton *m_cancel;
    QLabel *m_message;
    /*!
     * Actions for spreadEditToolBar.
     */
    QAction *m_addAction;
    QAction *m_subtractAction;
    QAction *m_multiplyAction;
    QAction *m_divideAction;
    QAction *m_percentageAction;
    QAction *m_equalsAction;
    QSignalMapper *m_signalMapper;
    /*!
     * Frame for Spreadsheet sheet information.
     */
    QFrame *m_sheetInfoFrame;
    QGridLayout *m_sheetInfoFrameLayout;
    QPushButton *m_addSheet;
    QPushButton *m_removeSheet;
    QPushButton *m_sheetName;

    /*!
     * line edit for search
     */
    QLineEdit *m_search;
    /*!
     * Checkbox to change between normal and exact match searches
     */
    QCheckBox *m_exactMatchCheckBox;
    /*!
     * Pointer to QTextDocument
     */
    QTextDocument *m_textDocument;
    /*!
     * Pointer to KoTextEditor
     */
    KoTextEditor *m_editor;
    /*!
     * Pointer to KWView
     */
    KWView * m_kwview;
    /*!
     * Pointer to KoView
     */
    KoView *m_view;
    /*!
     * Pointer to KoCanvasController
     */
    KoCanvasControllerWidget *m_controller;
    /*!
     * Pointer to KUndoStack
     */
    KUndoStack *m_undostack;
    /*!
     * Pointer to FreOffice CellTool
     */
    FoCellTool *m_focelltool;
    /*!
     * Integers about current page
     */
    int m_vPage, m_hPage;
    /*!
     * Flag for pressed state
     */
    bool m_pressed;
    /*!
     * Flag for seeing which toolbar was active
     */
    bool m_isViewToolBar;
    /*!
     * Timer for hiding full screen button
     */
    QTimer *m_fsTimer;
    /*!
     * Full screen push button
     */
    QPushButton *m_fsButton;
    /*!
     * Icon for full screen button
     */
    const QIcon m_fsIcon;

    /*!
     * Full screen back button for presentations
     */
    QPushButton *m_fsPPTBackButton;

    /*!
     * Full screen forward button for presentations
     */
    QPushButton *m_fsPPTForwardButton;

    /*!
     * Current page number. Saved in MainWindow::resourceChanged() slot.
     */
    int m_currentPage;

    /*!
     * Index for moving between searched strings
     */
    int m_index;
    /*!
     * Positions for found text strings
     */
    QList<QPair<QPair<KoPAPageBase*, KoShape*>, QPair<int, int> > > m_positions;
    /*!
     * Flag for seeing if search is for whole words. false by default
     */
    bool m_wholeWord;
    /*!
     * flag for checking open document type
     */
    enum DocumentType { Text, Presentation, Spreadsheet };
    DocumentType m_type;
    /*!
     * Pointer to splash class
     */
    Splash *m_splash;
    /*!
     * Pointer to pen draw button
     */
    QPushButton *m_fsPPTDrawPenButton;
    /*!
     * Pointer to highlight draw button
     */
    QPushButton *m_fsPPTDrawHighlightButton;
    /*!
     * Pointer to presentation drawing tools
     */
    PresentationTool *m_pptTool;
    /*!
     *Pointer to the MainWindowAdaptor object
     */
    MainWindowAdaptor *m_dbus;

    void init();
    /*!
     * close the document if it is open and reinit to 0
     */
    void closeDocument();
    /*!
     * style formatting function
     */
    void doStyle(KoListStyle::Style, KoTextEditor* editor);
    /*!
     *opening a new document
     */
    void openNewDocument(DocumentType);
    /*!
     *Retrieves path for saving file
     */
    QString getFileSavePath(DocumentType);
    /*!
     *Function to add formatframe components
     */
    QPushButton *addFormatFrameComponent(const QString &imagepath);
    /*!
     *Function to add fontstyleframe components
     */
    QPushButton *addFontStyleFrameComponent(const QString &imagepath);
    /*!
     *Function to create new document
     */
    QToolButton *addNewDocument(const QString &docname);
    /*!
     * Find string from document
     * /param pointer to QTextDocument
     * /param reference to text to be searched
     */
#ifdef HAVE_OPENGL
    GLPresenter *presenter;
#endif
    /*!
     * Open GL Class , to handle the slide show.
     */
    void findText(QList<QTextDocument*> docs, QList<QPair<KoPAPageBase*, KoShape*> > shapes, const QString &aText);
    /*!
     * Find string from document
     * /param current index
     */
    void highlightText(int aIndex);

    /*!
     * Trigger an action from the action collection of the current KoView.
     * /param name The name of the action to trigger
     * /return bool Returns false if there was no action with the given name found
     */
    bool triggerAction(const char* name);

    /*!
     * Update the enabled/disabled state of actions depending on if a document is currently
     * loaded.
     */
    void updateActions();

    /*!
     * Event filter to catch all mouse events to be able to properly show and hide the fullscreen
     * button when in fullscreen mode.
     */
    bool eventFilter(QObject *watched, QEvent *event);

    /*!
     * Check filetype
     * /param filename
     * /return true if supported
     */
    bool checkFiletype(const QString &fileName);

    /*!
     * shows back and forward buttons in fullscreen presentation mode
     */
    void showFullScreenPresentationIcons();

    /*!
     * Raise window. For some reason Qt programs are not not raised
     * automatically after D-Bus messages. This low level code sends
     * _NET_ACTIVE_WINDOW message to Matchbox and window is raised.
     */
    void raiseWindow(void);
    /*!
     * Function to check activeFormatFrame Options
     */
    void activeFormatOptionCheck();
    /*!
     * Function to check activeFontStyleFrame Options
     */
    void activeFontOptionCheck();
    /*!
     * Function for navigating to NextSheet
     */
    void nextSheet();
    /*!
     * Function for navigating to PreviousSheet
     */
    void prevSheet();
    /*!
     * Confirmation Dialog Destructor
     */
    void confirmationDialogDestructor();
    /*!
     * Format Frame Destructor
     */
    void formatFrameDestructor();
    /*!
     * FontStyle Frame Destructor
     */
    void fontStyleFrameDestructor();
    /*!
     * Template Chooser
     */
    void templateSelectionDialog();
    /*!
     *
     */
    bool m_firstChar;
    /*!
     * Set up tool bar.
     */
    void setUpSpreadEditorToolBar();
    /*!
     * Reset the SpreadEditor toolbar.
     */
    void resetSpreadEditorToolBar();

private:
    // Apply the selected formatting
    bool setFontSize(int size, KoTextEditor* editor);
    bool setFontType(const QString &font, KoTextEditor* editor);
    bool setTextColor(const QColor &color, KoTextEditor* editor);
    bool setTextBackgroundColor(const QColor &color, KoTextEditor* editor);
    bool setBold(KoTextEditor* editor);
    bool setItalic(KoTextEditor* editor);
    bool setUnderline(KoTextEditor* editor);
    bool setLeftAlign(KoTextEditor* editor);
    bool setRightAlign(KoTextEditor* editor);
    bool setCenterAlign(KoTextEditor* editor);
    bool setJustify(KoTextEditor* editor);
    bool setNumberList(KoTextEditor* editor);
    bool setBulletList(KoTextEditor* editor);
    bool setSubScript(KoTextEditor* editor);
    bool setSuperScript(KoTextEditor* editor);
    /*!
     * Handle the Spreadsheet sheet information
     */
    void spreadSheetInfo();

    //! Implemented for KoApplicationHelper
    virtual void showMessage(KoApplicationHelper::MessageType type, const QString& messageText = QString());

    //! Implemented for KoApplicationHelper
    virtual void startNewInstance(const QString& fileName, bool isNewDocument);

    //! Implemented for KoApplicationHelper
    virtual void setProgressIndicatorVisible(bool visible);

    //! Implemented for KoApplicationHelper
    virtual void showUiOnDocumentOpening();

private slots:

    void menuClicked(QAction* action);
    void pluginOpen(bool newWindow, const QString& path);
    void updateUI();
    void resourceChanged(int key, const QVariant &value);
    void showFontSizeDialog();
    void fontSizeEntered();
    void fontSizeRowSelected(QListWidgetItem *item);
    void startMathMode(bool start);
    /**
     * Adds symbol for spreadEditToolBar
     */
    void addMathematicalOperator(QString mathSymbol);
    /*!
     * Remove a sheet.
     */
    void removeSheet();
    /*!
     * Add add a sheet.
     */
    void addSheet();
    /*!
     * Current sheet info.
     */
    QString currentSheetName();
    /*!
     * Slot to perform UndoAction
     */
    void doUndo();
    /*!
     * Slot to perform RedoAction
     */
    void doRedo();
    /*!
     * Slot to actionSearch toggled signal
     */
    void toggleToolBar(bool);
    /*!
     * Slot to actionEdit toggled signal
     */
    void editToolBar(bool status=true);
    /*!
     * Slot to actionClose signal
     * @parm isWindowClosed set to true if you are calling it from the window
     *       close event.
     */
    void closeDoc(bool isWindowClosed=false);
    /*!
     *  Slot to convert character into bold
     */
    void doBold();
    /*!
     *  Slot to convert character into italic
     */
    void doItalic();
    /*!
     *  Slot to convert character into underline
     */
    void doUnderLine();
    /*!
     *  Slot for Left Alignment
     */
    void doLeftAlignment();
    /*!
     *  Slot for right Alignment
     */
    void doRightAlignment();
    /*!
     *  Slot for center Alignment
     */
    void doCenterAlignment();
    /*!
     *  Slot for justify Alignment
     */
    void doJustify();
    /*!
     *  Slot for adding Numbers
     */
    void doNumberList();
    /*!
     *  Slot for adding Bullets
     */
    void doBulletList();
    /*!
     *  Slot to perform sub Script action
     */
    void doSubScript();
    /*!
     *  Slot to perform super Script action
     */
    void doSuperScript();
    /*!
     *  Slot for font size Selection
     */
    void selectFontSize(int size);
    /*!
     *  Slot for font type Selection
     */
    void selectFontType();
    /*!
     *  Slot for text color Selection
     */
    void selectTextColor();
    /*!
     *  Slot for text backgroundcolor Selection
     */
    void selectTextBackGroundColor();
    /*!
     * Slot to discard newDocument without performing save operation
     */
    void discardNewDocument();
    /*!
     *    Slot to return back from closing new document
     */
    void returnToDoc();
    /*!
     * Slot to display formatframe with all options
     */
    void openFormatFrame();
    /*!
     * Slot to display fontstyleframe with all options
     */
     void openFontStyleFrame();
     /*!
      * Slot to display Math operation options
      */
    void openMathOpFrame();
    /*!
     * Slot to open new documnet
     */
    void openNewDoc();
    /*!
     * Slot to open new presenter
     */
    void openNewPresenter();
    /*!
     * Slot to open new spreadsheet
     */
    void openNewSpreadSheet();
     /*!
     * Slot to actionZoomIn triggered signal
     */
    void zoomIn();
    /*!
     * Slot to actionZoomOut triggered signal
     */
    void zoomOut();
    /*!
     * Slot to actionZoomLevel triggered signal
     */
    void zoom();
    void zoomToPage();
    void zoomToPageWidth();
    /*!
     * Slot to actionNextPage triggered signal
     */
    void nextPage();
    /*!
     * Slot to actionPrevPage triggered signal
     */
    void prevPage();
    /*!
     * Slot to fullscreen toolbutton triggered signal
     * Logic for switching from  normal mode to full screen mode
     */
    void fullScreen();
    /*!
     * Slot to mFSTimer timeout signal.
     * Hides full screen button
     */
    void fsTimer();
    /*!
     * Slot to mFSButton clicked signal
     * Deactivates fullscreen mode
     */
    void fsButtonClicked();
    /*!
     * Slot for actionSearch triggered signal
     */
    void startSearch();
    /*!
     * Slot for moving to previous found text string
     */
    void previousWord();
    /*!
     * Slot for moving to next found text string
     */
    void nextWord();
    /*!
     * Slot for toggleing between whole word search and part of word search
     */
    void searchOptionChanged(int aCheckBoxState);
    /*!
     * Slot to perform copy operation on selected text
     */
    void copy();
      /*!
     * Slot to perform cut operation on selected text
     */
    void cut();
    /*!
     * Slot to perform paste operation on selected text
     */
    void paste();
    /*!
     * Slot that shows a hildonized application menu
     */
    void showApplicationMenu();
    /*!
     * Slot that is invoked when the currently active tool changes.
     */
    void activeToolChanged(KoCanvasControllerWidget *canvas, int uniqueToolId);
    /*!
     * function for opening existing document.
     */
    void doOpenDocument();
    /*!
     * Slot to actionAbout triggered signal
     */
    void openAboutDialog();
    /*!
     * Slot to show  slide transition options
     */
    void slideTransitionDialog();
    /*!
     * Slot to preview selected templates
     */
    void selectedTemplatePreview(int number);
    /*!
     * Open selected template
     */
    void openSelectedTemplate();
    /*!
     * Close Template Selection Dialog
     */
    void closeTempSelectionDialog();
    /*!
     * Slot for progress indicator
     */
    //void slotProgress(int value);
    /*!
     * Slot to show digital signature information
     */
    void showDigitalSignatureInfo();

public slots:
    /*!
     * Slot to perform save operation
     */
    void saveFile();
    /*!
     * Slot to perform save as operation
     */
    void saveFileAs();
    /*!
     * Slot to  dialog fileSelected signal
     * /param filename
     */
    void openDocument(const QString &fileName, bool isNewDocument=false);
    /*!
     * Slot to choose new document
     */
    void chooseDocumentType();
    /*!
     * Slot to actionOpen triggered signal
     */
    void openFileDialog();
    /*!
     * Slot to check DBus activation, If document is not opened
     * then open filedialog
     */
    void checkDBusActivation();
    /*!
     * Slot to test scrolling of a loaded document and quitting afterwards.
     * This slot checks if a document has been loaded. If so, it calls pagedown and waits
     * until the end of the document has been reached.
     * Then it quits.
     */
    void loadScrollAndQuit();
    /*!
     * Slot to go to perticular page
     */
    void gotoPage(int page);
    /*!
     * Slot to show the preview dialog
     */
    void showPreviewDialog();
    /*!
     * Slot to show the notes dialog when show notes dialog is clicked
     */
    void slideNotesButtonClicked();
    /*!
     * Slot to update current slide in the presentation, when the slide changed in the notes dialog
     */
    void moveSLideFromNotesSLide(bool flag);
#ifdef HAVE_OPENGL
    void glPresenter();
    void glPresenterSet(int,int);
#endif
private:

    QMap<QString, OfficeInterface*> loadedPlugins;

    /*!
     * true if document is modified
     */
    bool m_isDocModified;
    /*!
     * flag for new file to existing file conversion
     */
    bool m_existingFile;
    /*!
     * QShortcut to cut text with Ctrl-X
     */
    QShortcut *m_cutShortcut;
    /*!
     * QShortcut for copying text with Ctrl-C
     */
    QShortcut *m_copyShortcut;
    /*!
     * QShortcut for copying text with Ctrl-V
     */
    QShortcut *m_pasteShortcut;
    /*!
     * QShortcut for copying text with Ctrl-Z
     */
    QShortcut *m_undoShortcut;
    /*!
     * QShortcut for copying text with Ctrl-P
     */
    QShortcut *m_redoShortcut;
    /*!
     * Count of mouseMove or tabletMove events after MousePress or tabletPress event
     */
    int m_panningCount;
    /*!
     * Position of last mousePress or tabletPress event
     */
    QPoint m_pressPos;
    /*!
     * True if slide can be changed by panning document
     */
    bool m_slideChangePossible;
    /*!
     * pointer to preview button store
     */
    StoreButtonPreview *storeButtonPreview;
    /*!
     * view number used while dbus session creation
     */
    int viewNumber;
    /*!
     * Pointer to show notes button
     */
    QPushButton *m_slideNotesButton;
    /*!
     * Icon for show notes button
     */
    const QIcon m_slideNotesIcon;
    /*!
     * Pointer to notes dialog
     */
    NotesDialog *notesDialog;
    /*!
     * Pointer to sliding motion dialog
     */
    SlidingMotionDialog *m_slidingmotiondialog;

    /*!
     * Tool bar for spread sheet.
     */
    QToolBar *m_spreadEditToolBar;

    DigitalSignatureDialog *digitalSignatureDialog;

#ifdef Q_WS_MAEMO_5
    FoDocumentRdf *foDocumentRdf;

    QShortcut *rdfShortcut;
#endif
signals:
    /*!
     * Presentation has entered full screen mode.
     */
    void presentationStarted();
    /*!
     * Presentation has exited from full screen mode.
     */
    void presentationStopped();
    /*!
     * Presentation has moved to the next slide.
     */
    void nextSlide();
    /*!
     * Presentation has moved to the previous slide.
     */
    void previousSlide();

 public slots:
    /*!
     * Enable the select tool. This will allow shapes to be selected and moved around
     */
    void enableSelectTool();
    /*!
     * Insert picture into the document
     */
    void insertImage();
    /*!
     * Shows available items which can be inserted into the document
     */
    void insertButtonClicked();
    /*!
     * Show cut,copy, paste actions
     */
    void showCCP();

};

#endif // MAINWINDOW_H
