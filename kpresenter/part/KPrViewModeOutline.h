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

#ifndef KPRVIEWMODEOUTLINE_H
#define KPRVIEWMODEOUTLINE_H

#include <KoPAViewMode.h>

class QTextEdit;

/**
 * @brief View for outline mode.
 */
class KPrViewModeOutline : public KoPAViewMode {

    Q_OBJECT;

public:

    KPrViewModeOutline( KoPAView * view, KoPACanvas * canvas );
    virtual ~KPrViewModeOutline();

    virtual void paintEvent( KoPACanvas * canvas, QPaintEvent* event );
    virtual void tabletEvent( QTabletEvent *event, const QPointF &point );
    virtual void mousePressEvent( QMouseEvent *event, const QPointF &point );
    virtual void mouseDoubleClickEvent( QMouseEvent *event, const QPointF &point );
    virtual void mouseMoveEvent( QMouseEvent *event, const QPointF &point );
    virtual void mouseReleaseEvent( QMouseEvent *event, const QPointF &point );
    virtual void keyPressEvent( QKeyEvent *event );
    virtual void keyReleaseEvent( QKeyEvent *event );
    virtual void wheelEvent( QWheelEvent * event, const QPointF &point );

    void activate(KoPAViewMode *previousViewMode);
    void deactivate();

private:

    /**
     * @brief The outline editor.
     */
    QTextEdit * m_editor;

};

#endif // KPRVIEWMODEOUTLINE_H
