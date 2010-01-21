/* This file is part of the KDE project
    Copyright (C) 2009 Jeremy Lugagne <lugagne.jeremy@gmail.com>

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

#include "KoDockContainer.h"
#include <QVBoxLayout>
#include <kdebug.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDockWidget>
#include <QVBoxLayout>
#include <KoDockRegistry.h>
#include <KoMainWindow.h>
#include <QList>

#include <QEvent>
#include <kgenericfactory.h>

KoDockContainer::KoDockContainer()
{
    show();
    setLayout(new QVBoxLayout());

    QList<KMainWindow*> windowList = KMainWindow::memberList();
    qDebug() << "number of windows : " << windowList.size();
    foreach(KMainWindow* window, windowList){
        KoMainWindow* mainWindow = dynamic_cast<KoMainWindow*>(window);

        if(mainWindow){
            qDebug() << "MainWindow catched";
            QList<QDockWidget*> list = mainWindow->dockWidgets();
            qDebug() << "number of dockers : " << list.size();
            foreach(QDockWidget* dockWidget, list){
                if(dockWidget) {
                    qDebug() << "installEventFilter";
                    dockWidget->installEventFilter(this);
                }
            }
        }
    }
}

void KoDockContainer::addDockWidget(QDockWidget *widget)
{
    installEventFilter(widget);
}

void KoDockContainer::catchDockWidget(QDockWidget *widget)
{
    if(m_widgets.contains(widget)){
        kDebug() << "already added";
        return;
    }
    m_widgets.push_back(widget);
    m_widgetsFlags.push_back(widget->windowFlags());
    widget->setVisible(true);
    widget->setWindowFlags(Qt::Popup);
    layout()->addWidget(widget);
}

void KoDockContainer::releaseDockWidget(QDockWidget* widget)
{
    int idx = m_widgets.indexOf(widget);
    if(idx != -1 )
    {
        QDockWidget* widget = m_widgets[idx];
        widget->setWindowFlags(m_widgetsFlags[idx]);
        m_widgets.removeAt(idx);
        m_widgetsFlags.removeAt(idx);
    }
}

bool KoDockContainer::eventFilter(QObject *object, QEvent *event)
{
    QDockWidget* dockWidget = dynamic_cast<QDockWidget*>(object);
    qDebug() << dockWidget->windowTitle() << " : eventFilter : " << event->type();
    if(dockWidget && event->type() == QEvent::MouseButtonRelease){
        kDebug() << "event MouseButtonRealease catched";
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if(mouseEvent){
            kDebug() << "docker catched";
            if(isOver(mouseEvent->pos())){
                catchDockWidget(dockWidget);
            }
            return false;
        }
    }
    return true;
}

bool KoDockContainer::isOver(const QPoint& globalPos )
{
    QPoint localPos = mapFromGlobal(globalPos);
    return (localPos.x() < geometry().width() && localPos.y() < geometry().height());
}

