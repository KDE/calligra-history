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
#include <KoPADocument.h>
#include <KoPAView.h>
#include <KoTextShapeData.h>
#include <KPrPage.h>
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

    m_defaultFrameFormat.setLeftMargin(15.0);
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

#include <QTextTable>
#include <QTextTableCell>

void KPrViewModeOutline::activate(KoPAViewMode * previousViewMode)
{
    Q_UNUSED(previousViewMode);

    populate();

    m_view->hide();
    m_editor->show();
    m_editor->setFocus(Qt::ActiveWindowFocusReason);

    connect(m_editor->document(), SIGNAL(contentsChange(int, int, int)), this, SLOT(synchronize(int,int,int)));
}

void KPrViewModeOutline::populate() {
    QTextCursor cursor = m_editor->document()->rootFrame()->lastCursorPosition();

    foreach (KoPAPageBase * pageBase, m_view->kopaDocument()->pages()) {
        KPrPage * page = static_cast<KPrPage *>(pageBase);
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
            m_link.insert(cursor.insertFrame(*frameFormat), pair.second->document()); // Create frame and save the link
            cursor.setCharFormat(*charFormat);
            // insert text (create lists where needed)
            cursor.insertText(pair.second->document()->toPlainText());
            cursor.movePosition(QTextCursor::End);
        }
    }
}

void KPrViewModeOutline::deactivate()
{
    disconnect(m_editor->document(), SIGNAL(contentsChange(int, int, int)), this, SLOT(synchronize(int,int,int)));
    m_editor->hide();
    m_editor->clear(); // Content will be regenerated when outline mode is activated
    m_link.clear();    // ditto
    m_view->show();
}

#include <QTextList>
#include <QTextListFormat>
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
    if(selectionStart >= startOfLine && selectionEnd <= endOfLine && selectionEnd != startOfLine) {
        return false;
    }

    // if selection is through several frames, no indent should be done, but normal tab key
    // should not be handled either

    cursor.beginEditBlock();
    QTextBlock block = cursor.block().document()->findBlock(selectionStart);

    cursor.movePosition(QTextCursor::EndOfLine);
    QTextList *list = cursor.currentList();
    QTextListFormat format = list->format();

    format.setIndent(indent?1:-1);
    QTextList *newList = cursor.insertList(format);

    while (block.isValid() && ((block.position() < selectionEnd) || oneOf)) {
        block.textList()->remove(block);
        newList->add(block);
        oneOf = false;
        block = block.next();
    }

    cursor.deletePreviousChar();
    cursor.endEditBlock();
    return true;
}

void KPrViewModeOutline::placeholderSwitch()
{
    qDebug() << "Placeholder switched !";
}

void KPrViewModeOutline::synchronize(int position, int charsRemoved, int charsAdded)
{
    QMap<QTextFrame*, QTextDocument *>::const_iterator i = m_link.constBegin();
     qDebug() << "Event on position " << position;
     // define in which frame text has changed
     for (; i != m_link.constEnd() && (i.key()->firstPosition() > position || i.key()->lastPosition() < position); ++i) {
         qDebug() << "Not in frame " << i.key() << " from pos " << i.key()->firstPosition() << " to " << i.key()->lastPosition();
     }

     if(i == m_link.constEnd()) { return; }

     QTextCursor cur(m_editor->document());
     QTextCursor target(i.value());
     int frameBegin = i.key()->firstPosition();

     qDebug() << "Event on frame " << i.key() << " from pos " << frameBegin << " to " << i.key()->lastPosition();
     qDebug() << charsRemoved << " chars removed";

     // synchronyze real target
     if (charsAdded > 0) {
         cur.setPosition(position);
         cur.setPosition(position+charsAdded, QTextCursor::KeepAnchor);
         qDebug() << charsAdded << " chars added (" << cur.selectedText() << ")";
         target.setPosition(position - frameBegin);
         target.insertText(cur.selectedText());
     }

     if (charsRemoved > 0) {
         target.setPosition(position - frameBegin);
         target.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, charsRemoved);
         target.deleteChar();
     }
}

void KPrViewModeOutline::KPrOutlineEditor::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
        if(int(event->modifiers()) == Qt::ControlModifier) {
            outline->placeholderSwitch();
        }
        else if (int(event->modifiers()) == 0) {
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
    default:
        QTextEdit::keyPressEvent(event);
    }
}


KPrViewModeOutline::KPrOutlineEditor::~KPrOutlineEditor()
{
}
