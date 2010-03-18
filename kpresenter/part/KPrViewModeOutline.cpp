/* This file is part of the KDE project
 * Copyright (C) 2010 Ludovic DELFAU <ludovicdelfau@gmail.com>
 * Copyright (C) 2010 Julien DESGATS <julien.desgats@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (  at your option ) any later version.
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

#include "KPrViewModeOutline.h"
#include "KPresenter.h"
#include <KoPADocument.h>
#include <KoPAView.h>
#include <KoTextShapeData.h>
#include <KPrPage.h>
#include <pagelayout/KPrPageLayout.h>
#include <pagelayout/KPrPageLayouts.h>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextList>
#include <QTextListFormat>
#include <QKeyEvent>
#include <QShortcut>
#include <QKeySequence>

KPrViewModeOutline::KPrViewModeOutline( KoPAView * view, KoPACanvas * canvas )
    : KoPAViewMode(view, canvas)
    , m_editor(new KPrOutlineEditor(this, m_view->parentWidget()))
    , m_link()
{
    m_editor->hide();

    // sets text format
    m_titleCharFormat.setFontWeight(QFont::Bold);
    m_titleCharFormat.setFontPointSize(14.0);
    m_titleFrameFormat.setTopMargin(12.0);
    m_titleFrameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    m_titleFrameFormat.setBorder(1);

    m_defaultFrameFormat.setLeftMargin(15.0);
    m_defaultFrameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    m_defaultFrameFormat.setBorder(1);
}

KPrViewModeOutline::~KPrViewModeOutline()
{

}

void KPrViewModeOutline::paintEvent( KoPACanvas * canvas, QPaintEvent* event )
{
    Q_ASSERT( m_canvas == canvas );
    Q_UNUSED(event);
    Q_UNUSED(canvas);
}

void KPrViewModeOutline::tabletEvent( QTabletEvent *event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::mousePressEvent( QMouseEvent *event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::mouseDoubleClickEvent( QMouseEvent *event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::mouseMoveEvent( QMouseEvent *event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::mouseReleaseEvent( QMouseEvent *event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::keyPressEvent( QKeyEvent *event )
{
    Q_UNUSED(event);
}

void KPrViewModeOutline::keyReleaseEvent( QKeyEvent *event )
{
    Q_UNUSED(event);
}

void KPrViewModeOutline::wheelEvent( QWheelEvent * event, const QPointF &point )
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeOutline::activate(KoPAViewMode * previousViewMode)
{
    Q_UNUSED(previousViewMode);

    populate();

    m_view->hide();
    m_editor->show();
    m_editor->setFocus(Qt::ActiveWindowFocusReason);
}

void KPrViewModeOutline::enableSync() {
    connect(m_editor->document(), SIGNAL(contentsChange(int, int, int)), this, SLOT(synchronize(int,int,int)));
}

void KPrViewModeOutline::disableSync() {
    disconnect(m_editor->document(), SIGNAL(contentsChange(int, int, int)), this, SLOT(synchronize(int,int,int)));
}

void KPrViewModeOutline::populate() {
    disableSync();
    m_editor->clear();
    m_link.clear();
    QTextCursor cursor = m_editor->document()->rootFrame()->lastCursorPosition();
    int i=0;
    QTextFrameFormat slideFrameFormat;
    int numSlide = 0;
    foreach (KoPAPageBase * pageBase, m_view->kopaDocument()->pages()) {
        KPrPage * page = static_cast<KPrPage *>(pageBase);

        slideFrameFormat.setBackground(QBrush((i++%2)?QColor(240,240,240):QColor(255,255,255)));
        QTextFrame* slideFrame = cursor.insertFrame(slideFrameFormat);

        // Copy relevant content of the page in the frame
        foreach (OutlinePair pair, page->placeholders().outlineData()) {
            // gets text format
            QTextFrameFormat *frameFormat;
            QTextCharFormat *charFormat;
            if (pair.first == "title") {
                frameFormat = &m_titleFrameFormat;
                charFormat = &m_titleCharFormat;
            }
            else if (pair.first == "subtitle" || pair.first == "outline") {
                frameFormat = &m_defaultFrameFormat;
                charFormat = &m_defaultCharFormat;
            }
            else {
                continue;
            }

            QTextFrame *frame = cursor.insertFrame(*frameFormat);
            FrameData frameData = {pair.second->document(), numSlide, pair.first};
            m_link.insert(frame, frameData); // Create frame and save the link
            cursor.setCharFormat(*charFormat);
            // insert text (create lists where needed)
            insertText(pair.second->document(), frame, charFormat);
            cursor.setPosition(slideFrame->lastPosition());
        }
        cursor.movePosition(QTextCursor::End);
        ++numSlide;
    }

    enableSync();
}

void KPrViewModeOutline::insertText(QTextDocument* sourceShape, QTextFrame* destFrame, QTextCharFormat* charFormat) {
    // we  start by insert raw blocks
    QTextCursor destFrameCursor = destFrame->firstCursorPosition();
    for (QTextBlock currentBlock = sourceShape->begin(); currentBlock.isValid(); currentBlock = currentBlock.next()) {
        destFrameCursor.setCharFormat(*charFormat);
        destFrameCursor.insertText(currentBlock.text());
        destFrameCursor.insertBlock();
    }
    destFrameCursor.deletePreviousChar();

    // then we format lists
    QTextList *currentList = 0;
    int currentIndent = -1;
    for (QTextBlock srcBlock = sourceShape->begin(), destBlock = destFrame->firstCursorPosition().block();
        srcBlock.isValid() && destBlock.isValid();
        srcBlock = srcBlock.next(), destBlock = destBlock.next()) {
        if (srcBlock.textList()) {
            QTextCursor destCursor(destBlock);
            if (currentList) {
                currentList->add(destBlock);
            }
            else {
                currentList = destCursor.createList(QTextListFormat::ListDisc);
            }
            QTextBlockFormat format = destBlock.blockFormat();
            currentIndent = srcBlock.blockFormat().leftMargin();
            format.setLeftMargin(currentIndent);
            destCursor.setBlockFormat(format);
        }
    }
}

void KPrViewModeOutline::deactivate()
{
    m_editor->hide();
    m_view->show();
}

bool KPrViewModeOutline::indent(bool indent)
{
    QTextCursor cursor = m_editor->textCursor();
    int selectionStart = qMin(cursor.anchor(), cursor.position());
    int selectionEnd = qMax(cursor.anchor(), cursor.position());
    bool oneOf = (selectionStart == selectionEnd); //ensures the block containing the cursor is selected in that case

    // if cursor is not at line start and there is no selection, no ident has to be done
    cursor.movePosition(QTextCursor::StartOfLine);
    int startOfLine = cursor.position();
    cursor.movePosition(QTextCursor::EndOfLine);
    int endOfLine = cursor.position();
    if (selectionStart >= startOfLine && selectionEnd <= endOfLine && selectionEnd != startOfLine) {
        qDebug() << "Not indent";
        return false;
    }

    // if selection is through several frames, no indent should be done, but normal tab key
    // should not be handled either
    cursor.setPosition(selectionStart);
    QTextDocument *targetShape = m_link[cursor.currentFrame()].textDocument;
    cursor.setPosition(selectionEnd);
    QTextDocument *selectionEndShape = m_link[cursor.currentFrame()].textDocument;

    if (targetShape == 0 || targetShape != selectionEndShape) {
        qDebug("Incorrect indent");
        return true;
    }

    int frameOffset = cursor.currentFrame()->firstPosition();

    cursor.setPosition(selectionStart);
    cursor.beginEditBlock();

    QTextCursor targetCursor(targetShape);
    targetCursor.beginEditBlock();

    qDebug() << (indent?"Indent":"Unindent") << " from "<<selectionStart << " to " << selectionEnd;

    QTextBlock block = cursor.block();
    for (; block.isValid() && block.textList() && ((block.position() < selectionEnd) || oneOf); block = block.next()) {
        QTextBlockFormat format = block.blockFormat();
        if (indent || format.leftMargin() > 0) {
            int newMargin = format.leftMargin() + (indent?10:-10);
            qDebug() << "Indent "<<block.text() << " old indent: " << format.leftMargin() << "; new indent: " << newMargin;
            format.setLeftMargin(newMargin);
            cursor.setPosition(block.position());
            cursor.setBlockFormat(format);

            targetCursor.setPosition(block.position() - frameOffset);
            QTextBlockFormat targetFormat = targetCursor.blockFormat();
            targetFormat.setLeftMargin(newMargin);
            targetCursor.setBlockFormat(targetFormat);
        } else {
            qDebug() << "New slide!";
        }
        oneOf = false;
    }
    cursor.endEditBlock();
    targetCursor.endEditBlock();

    // if the selection is not fully a list, we undo the changes
    if (!block.previous().contains(selectionEnd)) {
        qDebug() << "undo last indent";
        cursor.document()->undo();
        targetCursor.document()->undo();
    }

    return true;
}

void KPrViewModeOutline::placeholderSwitch()
{
    QTextCursor cur = m_editor->textCursor();
    QTextFrame* currentFrame = cur.currentFrame();
    FrameData &currentFrameData = m_link[currentFrame];

    if (!currentFrameData.textDocument || currentFrameData.type == "title") {
        return;
    }

    // we search the next known frame
    while((cur.currentFrame() == currentFrame ||
           !(m_link[cur.currentFrame()].textDocument)) &&
          !cur.atEnd()) {
        cur.movePosition(QTextCursor::NextCharacter);
    }

    if(!cur.atEnd() && m_link[cur.currentFrame()].numSlide == currentFrameData.numSlide) {
        m_editor->setTextCursor(cur);
    }
}

void KPrViewModeOutline::addSlide() {
    int numSlide = m_link[currentFrame()].numSlide;
    // Active the current page and insert page
    m_view->setActivePage(m_view->kopaDocument()->pageByIndex(numSlide, false));
    m_view->insertPage();
    // Search layouts
    KPrPageLayouts * layouts = m_view->kopaDocument()->resourceManager()->resource(KPresenter::PageLayouts).value<KPrPageLayouts*>();
    Q_ASSERT( layouts );
    const QList<KPrPageLayout *> layoutMap = layouts->layouts();
    // Add the layout 1
    //TODO Find constant for 1
    KPrPage * page = static_cast<KPrPage *>(m_view->kopaDocument()->pages()[numSlide + 1]);
    page->setLayout(layoutMap[1], m_view->kopaDocument());
    // Reload the editor
    populate();
}

void KPrViewModeOutline::synchronize(int position, int charsRemoved, int charsAdded)
{
     QTextCursor cur(m_editor->document());
     cur.setPosition(position);
     QTextFrame* frame = cur.currentFrame();
     int frameBegin = frame->firstPosition();

     qDebug() << "Event on pos " << position << ", frame " << frame << "(" << frameBegin << " => " << frame->lastPosition() << ") "<< (m_link.contains(frame)?"(known)":"(unknown)");
     qDebug() << charsAdded << " chars added; " << charsRemoved << " chars removed";
     while(cur.currentFrame() == frame) cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
     qDebug() << "Frame contents: " << cur.selectedText();

     QTextDocument* targetDocument = m_link[frame].textDocument;
     if(!targetDocument) {  // event on an unknown frame (parasite blank line ?)
         qDebug() << "Incorrect action";
         return;
     }

     QTextCursor target(targetDocument);
     // synchronyze real target
     if (charsRemoved > 0) {
         target.setPosition(position - frameBegin);
         target.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, charsRemoved);
         target.deleteChar();
     }
     if (charsAdded > 0) {
         cur.setPosition(position);
         qDebug() << cur.currentFrame();
         cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, charsAdded);
         qDebug() << cur.currentFrame();
         qDebug() << charsAdded << " chars added (" << cur.selectedText() << ")";
         target.setPosition(position - frameBegin);
         target.insertText(cur.selectedText());
     }
}

void KPrViewModeOutline::KPrOutlineEditor::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
        if (int(event->modifiers()) == 0) {
            if(!outline->indent(true)) {
                // if no indent has been done, a regular tab is inserted
                QTextEdit::keyPressEvent(event);
            }
        }
        else {
            QTextEdit::keyPressEvent(event);
        }
        break;
    case Qt::Key_Backtab:
        if (int(event->modifiers()) == Qt::ShiftModifier) {
            // if no indent has been done, event is not passed to ancestor because this will
            // result in a focus lost
            outline->indent(false);
        } else {
            QTextEdit::keyPressEvent(event);
        }
        break;
    case Qt::Key_Return:
        qDebug() << "Qt::Key_Return";
        if (int(event->modifiers()) == 0) {
            if (outline->m_link[outline->m_editor->textCursor().currentFrame()].type == "title") {
                outline->addSlide();
            }
            else {
                // the native behaviour whan adding an item is buggy as hell !
                QTextCursor cur = textCursor();
                QTextDocument* target = outline->m_link[cur.currentFrame()].textDocument;
                if (!(cur.currentList() && target)) {
                    QTextEdit::keyPressEvent(event);
                    break;
                }

                outline->disableSync();
                QTextEdit::keyPressEvent(event);
                outline->enableSync();
                QTextCursor targetCur(target);
                targetCur.setPosition(cur.position() - cur.currentFrame()->firstPosition());
                targetCur.insertText("\n");
            }
        } else if(int(event->modifiers()) == Qt::ControlModifier) {
            qDebug() << "placeholderSwitch";
            outline->placeholderSwitch();
        }
        else {
            QTextEdit::keyPressEvent(event);
        }
        break;
    default:
        qDebug()<< event->key();
        QTextEdit::keyPressEvent(event);
    }
}


KPrViewModeOutline::KPrOutlineEditor::~KPrOutlineEditor()
{
}
