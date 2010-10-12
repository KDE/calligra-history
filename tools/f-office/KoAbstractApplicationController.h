/* This file is part of the KDE project
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Boudewijn Rempt <boud@kogmbh.com>
 * Copyright (C) 2010 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KOABSTRACTAPPLICATIONCONTROLLER_H
#define KOABSTRACTAPPLICATIONCONTROLLER_H

#include <QMessageBox>
#include <KoPAPageBase.h>
#include <KoShape.h>
#include <KoDocument.h>

//TODO #include "CollaborateInterface.h"

class QTextDocument;
class QSplashScreen;
class QCloseEvent;

class KUndoStack;
class KoView;
class KWView;
class KoTextEditor;
class KoCanvasControllerWidget;
class FoCellToolFactory;
class FoCellTool;
class StoreButtonPreview;
class PresentationTool;

//class KoAbstractApplicationSignalReceiver;

class KoAbstractApplicationController //TODO: public CollaborateInterface
{
public:
    KoAbstractApplicationController();
    virtual ~KoAbstractApplicationController();

#if 0
    //! Receiver provides slots that can be connected to signals. The slots are:
    /*! - void documentPageSetupChanged()
        - void goToPage(int page)
        - void goToPreviousPage()
        - void goToNextPage()
    */
    QObject *signalReceiver() const;
#endif

    enum MessageType {
        UnsupportedFileTypeMessage,
        InformationMessage,              //!< Information with default timeout
        InformationWithoutTimeoutMessage //!< Information without timeout
    };

    enum QuestionType {
        SaveDiscardCancelQuestion, //!< "Document is modified, do you want to save it before closing?" question
                                   //!< with possible resultsL QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel
        ConfirmSheetDeleteQuestion //!< "Do you want to delete the sheet?" question
                                   //!< with possible results: QMessageBox::Yes, QMessageBox::No
                                   //! Provides localized message.
    };

    /*!
     * @return true if @a extension is supported by the application.
     * Comparison is case insensitive.
     */
    bool isSupportedExtension(const QString& extension) const;

    /*!
     * @return true if @a extension is natively supported by the application (i.e. is ODT, ODP or ODS).
     * Comparison is case insensitive.
     */
    bool isNativeDocumentExtension(const QString& extension) const;

    /*!
     * @return true if @a extension is supported by the application as text document.
     * Comparison is case insensitive.
     */
    bool isTextDocumentExtension(const QString& extension) const;

    /*!
     * @return true if @a extension is supported by the application as presentation document.
     * Comparison is case insensitive.
     */
    bool isPresentationDocumentExtension(const QString& extension) const;

    /*!
     * @return true if @a extension is supported by the application as spreadsheet document.
     * Comparison is case insensitive.
     */
    bool isSpreadsheetDocumentExtension(const QString& extension) const;

    /*!
     * Opens document from @a fileName. If @a isNewDocument is true, new document is created.
     */
    virtual bool openDocument(const QString &fileName, bool isNewDocument = false);

    /*!
     * Closes document without asking for confirmation. Can be reimplemented.
     */
    virtual void closeDocument();

    /*!
     * Opens document.
     * @return true if document has been saved.
     */
    virtual bool openDocument();

    /*!
     * Shows document save dialog.
     */
    QString getSaveFileName();

    /*!
     * Saves document. Calls saveDocumentAs() if needed.
     * @return true if document has been saved.
     */
    virtual bool saveDocument();

    /*!
     * Saves document under name selected by user.
     * @return true if document has been saved.
     */
    virtual bool saveDocumentAs();

    /*!
     * Document type.
     */
    enum DocumentType { NoDocument, TextDocument, PresentationDocument, SpreadsheetDocument };

    /*!
     * @return type of loaded document or NoDocument if no document is loaded.
     */
    inline DocumentType documentType() const { return m_type; }

    /*!
     * @return number of pages in loaded document or 0 if document is not loaded.
     */
    int pageCount() const;

    /*!
     * @return current page number.
     */
    int currentPage() const { return m_currentPage; }

    /*!
     * Goes to particular page @a page.
     */
    void goToPage(int page);

    /*!
     * Goes to previous page.
     */
    void goToPreviousPage();

    /*!
     * Goes to next page.
     */
    void goToNextPage();

    /*!
     * Shows or hides virtual keyboard.
     */
    void toggleVirtualKeyboardVisibility();

protected:
    // -- for implementation --

    /*!
     * Shows message with text @a messageText of type @a type.
     */
    virtual void showMessage(MessageType type, const QString& messageText = QString()) = 0;

    /*!
     * Shows question message of type @a type with optional text @a messageText.
     */
    virtual QMessageBox::StandardButton askQuestion(QuestionType type, const QString& messageText = QString()) = 0;

    /*!
     * Starts new instance of the application and opens document from @a fileName.
     * If @a isNewDocument is true, new document is created.
     */
    virtual void startNewInstance(const QString& fileName, bool isNewDocument) = 0;

    /*!
     * Shows or hides progress bar indicator.
     */
    virtual void setProgressIndicatorVisible(bool visible) = 0;

    /*!
     * Performs UI initialization needed before document opening.
     */
    virtual void showUiBeforeDocumentOpening(bool isNewDocument) = 0;

    /*!
     * Asks user for a file path name for opening and returns it.
     * @return selected filename or empty string if selection was cancelled.
     * See QFileDialog::getOpenFileName() for explanation of arguments.
     */
    virtual QString showGetOpenFileNameDialog(const QString& caption, const QString& dir, const QString& filter) = 0;

    /*!
     * Asks user for a file path name for saving and returns it.
     * @return selected filename or empty string if selection was cancelled.
     * See QFileDialog::getSaveFileName() for explanation of arguments.
     */
    virtual QString showGetSaveFileNameDialog(const QString& caption, const QString& dir, const QString& filter) = 0;

    /*!
     * Sets central widget for application view.
     */
    virtual void setCentralWidget(QWidget *widget) = 0;

    /*!
     * Sets main window's title to @a title.
     */
    virtual void setWindowTitle(const QString& title) = 0;

    /*!
     * @return central widget for application view.
     */
    virtual QWidget* centralWidget () const = 0;

    /*!
     * Update the enabled/disabled state of actions depending on if a document is currently
     * loaded.
     */
    virtual void updateActions() = 0;

    /*!
     * @return translated application name.
     */
    virtual QString applicationName() const = 0;

    /*!
     * Shows or hides virtual keyboard.
     */
    virtual void setVirtualKeyboardVisible(bool set) = 0;

    /*!
     * @return true if virtual keyboard is visible.
     */
    virtual bool isVirtualKeyboardVisible() const = 0;

    // -- for possible reimplementation --

    /*!
     * Called whenever page setup for document changes.
     * @see document(), KWDocument::pageSetupChanged()
     */
    virtual void documentPageSetupChanged();

    /*!
     * Sets editing mode on or off. @return true on success.
     * It is possible to reimplement this method to add more functionality;
     * in this case implementation from the superclass should be called.
     */
    virtual bool setEditingMode(bool set);

    /*!
     * Moves to next slide.
     */
    virtual void goToNextSlide() = 0;

    /*!
     * Moves to previous slide.
     */
    virtual void goToPreviousSlide() = 0;

    /*!
     * Current page number has changed. Reaction on this should be implemented here.
     */
    virtual void currentPageChanged() = 0;

    // -- utilities --

    /*!
     * Opens document (slot).
     */
    virtual bool doOpenDocument();

    /*!
     * Sets document modification flag.
     */
    void setDocumentModified(bool set) { if (m_doc) m_doc->setModified(set); }

    /*!
     * @return true if document is modified.
     */
    bool isDocumentModified() const { return m_doc ? m_doc->isModified() : false; }

    /*!
     * @return loaded document object.
     */
    inline KoDocument* document() const { return m_doc; }

    /*!
     * @return view.
     */
    inline KoView* view() const { return m_view; }

    /*!
     * @return controller.
     */
    inline KoCanvasControllerWidget* controller() const { return m_controller; }

    /*!
     * @return text editor object.
     */
    inline KoTextEditor* textEditor() const { return m_editor; }

    /*!
     * Pointer to KWView
     */
    KWView* kwordView() const;

    /*!
     * Sets splash screen to @a splash.
     */
    void setSplashScreen(QSplashScreen* splash);

    /*!
     * @return pointer to document's undo stack or 0 if there is no document.
     */
    inline KUndoStack* undoStack() const { return m_doc ? m_doc->undoStack() : 0; }

    /*!
     * Moves to next sheet.
     */
    void goToNextSheet();

    /*!
     * Moves to previous sheet.
     */
    void goToPreviousSheet();

    /*!
     * Trigger an action from the action collection of the current KoView.
     * @param name The name of the action to trigger
     * @return bool Returns false if there was no action with the given name found
     */
    bool triggerAction(const char* name);

    /*!
     * Handles application's close event. @return true if close event can be accepted.
     */
    bool handleCloseEvent(QCloseEvent *event);

    /*!
     * Slot
     */
    void resourceChanged(int key, const QVariant &value);

    /*!
     * Slot
     */
    void addSheet();

    /*!
     * Slot
     */
    void removeSheet();

    /*!
     * @return name of the current sheet.
     */
    QString currentSheetName() const;

    StoreButtonPreview *storeButtonPreview() const;

    FoCellTool* cellTool() const;

//! @todo remove
    PresentationTool *presentationTool() const;

//! @todo remove
    void setPresentationTool(PresentationTool *tool);

    QSplashScreen* splash() const { return m_splash; }

//! @todo make private
    bool m_firstChar;
//! @todo make private
    /*!
     * Index for moving between searched strings
     */
    int m_searchTextIndex;
//! @todo make private
    /*!
     * Positions for found text strings
     */
    QList<QPair<QPair<KoPAPageBase*, KoShape*>, QPair<int, int> > > m_searchTextPositions;
//! @todo make private
    /*!
     * Flag for seeing if search is case sensitive. False by default.
     * @todo implement this
     */
    bool m_searchWholeWords;
//! @todo make private
    /*!
     * Flag for seeing if search is case sensitive. False by default.
     */
    bool m_searchCaseSensitive;

private:
    inline QObject *thisObject() { return dynamic_cast<QObject*>(this); }

    bool saveDocumentInternal(const QString& fileName);

#if 0
    /*!
     * Internal object for receiving signals.
     */
    KoAbstractApplicationSignalReceiver *m_signalReceiver;
#endif

    /*!
     * Name of the opened file.
     */
    QString m_fileName;

    /*!
     * File name that will be opened by doOpenDocument().
     */
    QString m_fileNameToOpen;

    /*!
     * Pointer to KoDocument
     */
    KoDocument *m_doc;

    /*!
     * true if document is currently being loaded
     */
    bool m_isLoading;

    /*!
     * flag for checking open document type
     */
    DocumentType m_type;

    /*!
     * Current page number. Saved in MainWindow::resourceChanged().
     */
    int m_currentPage;

    /*!
     * Pointer to KoView
     */
    KoView *m_view;

    /*!
     * Pointer to QTextDocument
     */
    QTextDocument *m_textDocument;

    /*!
     * Pointer to KoTextEditor
     */
    KoTextEditor *m_editor;

    /*!
     * Pointer to KoCanvasController
     */
    KoCanvasControllerWidget *m_controller;

    /*!
     * Pointer to FreOffice CellTool
     */
    FoCellTool *m_cellTool;

    FoCellToolFactory *m_cellToolFactory;

    /*!
     * pointer to preview button store
     */
    StoreButtonPreview *m_storeButtonPreview;

    /*!
     * Pointer to splash class
     */
    QSplashScreen *m_splash;
    /*!
     * Pointer to presentation drawing tools
     */
    PresentationTool *m_presentationTool;

//    friend class KoAbstractApplicationSignalReceiver;
};

#endif
