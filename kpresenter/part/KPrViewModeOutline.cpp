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

#include <KoPAView.h>
#include <QTextEdit>

KPrViewModeOutline::KPrViewModeOutline( KoPAView * view, KoPACanvas * canvas )
    : KoPAViewMode(view, canvas)
    , m_editor(new QTextEdit(m_view->parentWidget()))
{
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
    m_view->hide();
    m_editor->show();
    m_editor->setFocus(Qt::ActiveWindowFocusReason);
}

void KPrViewModeOutline::deactivate()
{
    m_editor->hide();
    m_view->show();
}
