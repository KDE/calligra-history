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
#include <QKeyEvent>
#include <QShortcut>
#include <QKeySequence>

KPrViewModeOutline::KPrViewModeOutline( KoPAView * view, KoPACanvas * canvas )
    : KoPAViewMode(view, canvas)
    , m_editor(new KPrOutlineEditor(this, m_view->parentWidget()))
{
    //m_editor->setFocusPolicy(Qt::ClickFocus);
    m_editor->hide();
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
    // Create cursor for editing documents
    QTextCursor cursor = m_editor->document()->rootFrame()->lastCursorPosition();
    // Custom frame format
    QTextFrameFormat frameFormat;
    frameFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    frameFormat.setBorder(1.0);
    // Create frame for each page
    foreach (KoPAPageBase * pageBase, m_view->kopaDocument()->pages()) {
        KPrPage * page = dynamic_cast<KPrPage *>(pageBase);
        if (page == NULL) continue;
        QTextFrame * slideFrame = cursor.insertFrame(frameFormat);
        // Copy relevant content of the page in the frame
        foreach (OutlinePair pair, page->placeholders().outlineData()) {
            if (pair.second == NULL) continue;
            if (pair.first == "title" || pair.first == "subtitle" || pair.first == "outline") {
                cursor.insertFrame(frameFormat);
                cursor.insertFragment(QTextDocumentFragment(pair.second->document()));
                cursor = slideFrame->lastCursorPosition();
            }
        }
        cursor = m_editor->document()->rootFrame()->lastCursorPosition();
    }
    m_view->hide();
    m_editor->show();
    m_editor->setFocus(Qt::ActiveWindowFocusReason);
}

void KPrViewModeOutline::deactivate()
{
    m_editor->hide();
    m_editor->clear(); // Content will be regenerated when outline mode is activate
    m_view->show();
}

void KPrViewModeOutline::unindent()
{
    qDebug() << "Unindented !";
}

#include <QTextList>
#include <QTextListFormat>
void KPrViewModeOutline::indent()
{
    QTextCursor cursor = m_editor->textCursor();
    qDebug() << "Indented !";    

    int selectionStart = qMin(cursor.anchor(), cursor.position());
    int selectionEnd = qMax(cursor.anchor(), cursor.position());

    QTextBlock block = cursor.block().document()->findBlock(selectionStart);

    bool oneOf = (selectionStart == selectionEnd); //ensures the block containing the cursor is selected in that case
    QTextList *list = cursor.currentList();
    QTextListFormat format = list->format();
    format.setIndent(format.indent() + 1);
    QTextList *newList = cursor.insertList(format);

    while (block.isValid() && ((block.position() < selectionEnd) || oneOf)) {
        /*m_blocks.append(block);
        if (block.textList()) {
            m_lists.insert(m_blocks.size() - 1, KoTextDocument(block.document()).list(block.textList()));
            Q_ASSERT(m_lists.value(m_blocks.size() - 1));
            m_levels.insert(m_blocks.size() - 1, effectiveLevel(m_lists.value(m_blocks.size() - 1)->level(block)));
        }*/

        list->remove(block);
        newList->add(block);

        oneOf = false;
        block = block.next();
        qDebug() << "Indented";
    }
}

void KPrViewModeOutline::placeholderSwitch()
{
    qDebug() << "Placeholder switched !";
}

void KPrViewModeOutline::KPrOutlineEditor::keyPressEvent(QKeyEvent *event)
{
    qDebug("KeyPress: %x, modifier: %x", event->key(), int(event->modifiers()));
    switch (event->key()) {
    case Qt::Key_Tab:
        if(int(event->modifiers()) == Qt::ControlModifier) {
            outline->placeholderSwitch();
        }
        else if (int(event->modifiers()) == 0) {
            outline->indent();
        }
        else {
            QTextEdit::keyPressEvent(event);
        }
        break;
    case Qt::Key_Backtab:
        if (int(event->modifiers()) == Qt::ShiftModifier) {
            outline->unindent();
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
