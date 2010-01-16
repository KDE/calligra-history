/* This file is part of the KDE project
    Copyright (C) 2010 Jeremy Lugagne <lugagne.jeremy@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KODOCKWONTAINER_H
#define KODOCKWONTAINER_H
#include <QWidget>

class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDockWidget;
class QEvent;
class QObjet;

class KoDockContainer : public QWidget
{
Q_OBJECT
public :
    KoDockContainer();
    void catchDockWidget(QDockWidget *widget);
    void releaseDockWidget(QDockWidget *widget);
    void addDockWidget(QDockWidget *widget);

protected :
    bool isOver(const QPoint& pos );
    bool eventFilter(QObject *object, QEvent *event);

private :
    QList<QDockWidget*> m_widgets;
    QList<Qt::WindowFlags> m_widgetsFlags;
};
#endif /*KODOCKWONTAINER_H*/

