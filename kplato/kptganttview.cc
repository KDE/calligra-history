/* This file is part of the KDE project
   Copyright (C) 2002 The Koffice Team <koffice@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
 
#include "kptganttview.h"

#include "kpttimescale.h"
#include "kptprojectlist.h"
#include "kptview.h"
#include "kptganttcanvas.h"
#include "kptcanvasitem.h"
#include "kptnode.h"
#include "kptnodeitem.h"
#include "kptpart.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptmilestone.h"

#include "KDGanttViewItem.h"
#include "KDGanttViewTaskItem.h"
#include "KDGanttViewSummaryItem.h"
#include "KDGanttViewEventItem.h"

#include <kdebug.h>

#include <qsplitter.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qheader.h>
#include <qpopupmenu.h>

KPTGanttView::KPTGanttView( KPTView *view, QWidget *parent )
    : KDGanttView( parent, "Gantt view" ),
    m_mainview( view ),
	m_currentItem(0)
{
    setScale(KDGanttView::Day);
    
	connect(this, SIGNAL(lvContextMenuRequested ( KDGanttViewItem *, const QPoint &, int )),
	             this, SLOT (popupMenuRequested(KDGanttViewItem *, const QPoint &, int)));
	
	connect(this, SIGNAL(lvCurrentChanged(KDGanttViewItem*)), this, SLOT (currentItemChanged(KDGanttViewItem*)));
	
}

void KPTGanttView::zoom(double zoom)
{
}

void KPTGanttView::draw(KPTNode &node) 
{
    kdDebug()<<k_funcinfo<<endl;
	clear();
	KPTDuration *time;
	KPTDuration *dur;

	if (node.type() == KPTProject::TYPE)
    {	
	    KDGanttViewSummaryItem *item = new KPTGanttViewSummaryItem(this, node);
		time = node.getStartTime();
		dur = node.getExpectedDuration();
	    item->setStartTime(time->dateTime());
		time->add(dur);
	    item->setEndTime(time->dateTime());
		item->setOpen(true);
		delete time;
		delete dur;
	
	    drawChildren(item, node);
    }		
	else
		kdDebug()<<k_funcinfo<<"Not implemented yet"<<endl;
	
}

void KPTGanttView::drawChildren(KDGanttViewSummaryItem *parentItem, KPTNode &parentNode)
{
	QPtrListIterator<KPTNode> nit(parentNode.childNodeIterator()); 
	for ( nit.toLast(); nit.current(); --nit )
	{
		KPTNode *n = nit.current();
		if (n->type() == KPTProject::TYPE)
	        drawProject(parentItem, *n);
		else if (n->type() == KPTTask::TYPE)
		    drawTask(parentItem, *n);
		else if (n->type() == KPTMilestone::TYPE)
			drawMilestone(parentItem, *n);
		else
		    kdDebug()<<k_funcinfo<<"Not implemented yet"<<endl;
			
	}
}

void KPTGanttView::drawProject(KDGanttViewSummaryItem *parentItem, KPTNode &node)
{
	KPTDuration *time = node.getStartTime();
	KPTDuration *dur = node.getExpectedDuration();
	KPTGanttViewSummaryItem *item = new KPTGanttViewSummaryItem(parentItem, node);
	item->setStartTime(time->dateTime());
	time->add(dur);
	item->setEndTime(time->dateTime());
	item->setOpen(true);
	delete time;
	delete dur;
	
	drawChildren(item, node);
}

void KPTGanttView::drawTask(KDGanttViewSummaryItem *parentItem, KPTNode &node)
{
	KPTDuration *time = node.getStartTime();
	KPTDuration *dur = node.getExpectedDuration();
	if (node.numChildren() > 0)
    {	
		KPTGanttViewSummaryItem *item = new KPTGanttViewSummaryItem(parentItem, node);
		item->setStartTime(time->dateTime());
		time->add(dur);
		item->setEndTime(time->dateTime());
		item->setOpen(true);
    	
		drawChildren(item, node);
	}
	else
	{
		KPTGanttViewTaskItem *item = new KPTGanttViewTaskItem(parentItem, node);
		item->setStartTime(time->dateTime());
		time->add(dur);
		item->setEndTime(time->dateTime());
		item->setOpen(true);
	}
	delete time;
	delete dur;
}

void KPTGanttView::drawMilestone(KDGanttViewSummaryItem *parentItem, KPTNode &node)
{
	KPTDuration *time = node.getStartTime();
	KPTGanttViewEventItem *item = new KPTGanttViewEventItem(parentItem, node);
	item->setStartTime(time->dateTime());
	item->setLeadTime(time->dateTime().addDays(1));
	item->setOpen(true);
	delete time;
}

void KPTGanttView::currentItemChanged(KDGanttViewItem* item)
{
    m_currentItem = item;
}

KPTNode *KPTGanttView::currentNode()
{
    KDGanttViewItem *curr = m_currentItem;
	if (!curr)
		curr = firstChild();
	if (curr->type() == KDGanttViewItem::Summary)
	{
	    KPTGanttViewSummaryItem *item = (KPTGanttViewSummaryItem *)curr;
		kdDebug()<<k_funcinfo<<"Summary item="<<item<<endl;
		return &(item->getNode());
	}
	else if (curr->type() == KDGanttViewItem::Task)
	{
		KPTGanttViewTaskItem *item = (KPTGanttViewTaskItem *)curr;
		kdDebug()<<k_funcinfo<<"Task item="<<item<<endl;
		return &(item->getNode());
	}
	else if (curr->type() == KDGanttViewItem::Event)
	{
		KPTGanttViewEventItem *item = (KPTGanttViewEventItem *)curr;
		kdDebug()<<k_funcinfo<<"Event item="<<item<<endl;
		return &(item->getNode());
	}
	kdDebug()<<k_funcinfo<<"No item="<<endl;
	return 0;
}

void KPTGanttView::popupMenuRequested(KDGanttViewItem * item, const QPoint & pos, int)
{
	QPopupMenu *menu = m_mainview->popupMenu("node_popup");
	if (menu)
	{
		int id = menu->exec(pos);
		kdDebug()<<k_funcinfo<<"id="<<id<<endl;
	}
	else
		kdDebug()<<k_funcinfo<<"No menu!"<<endl;
}

#include "kptganttview.moc"
