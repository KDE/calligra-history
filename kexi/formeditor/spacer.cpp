/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#include <qsizepolicy.h>
#include <qpainter.h>
#include <qdom.h>
#include <qvariant.h>

#include <kdebug.h>

#include "objecttree.h"
#include "container.h"
#include "form.h"
#include "formIO.h"
#include "widgetlibrary.h"

#include "spacer.h"


namespace KFormDesigner {

Spacer::Spacer(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void
Spacer::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.eraseRect(0,0,width(), height());
	p.drawLine(0, 0, width()-1, height()-1);
	p.drawLine(0, height()-1, width()-1, 0);
}

bool
Spacer::showProperty(const QString &name)
{
	kdDebug() << "Spacer::showProperty() " << endl;
	if((name == "name") || (name == "geometry") || (name == "sizePolicy"))
		return true;
	else
		return false;
}

void
Spacer::saveSpacer(ObjectTreeItem *item, QDomElement &parent, QDomDocument &domDoc)
{
	QDomElement tclass = domDoc.createElement("spacer");
	parent.appendChild(tclass);
	FormIO::prop(tclass, domDoc, "name", item->widget()->property("name"), item->widget());
	FormIO::prop(tclass, domDoc, "geometry", item->widget()->property("geometry"), item->widget());

	// Saving spacer orientation
	QDomElement propertyE = domDoc.createElement("property");
	propertyE.setAttribute("name", "orientation");

	QString orient;
	if(item->parent()->container()->layoutType() == Container::HBox)
		orient = "Horizontal";
	else if(item->parent()->container()->layoutType() == Container::VBox)
		orient = "Vertical";
	else
		orient = "Horizontal";

	QDomElement type = domDoc.createElement("enum");
	QDomText valueE = domDoc.createTextNode(orient);
	type.appendChild(valueE);
	propertyE.appendChild(type);
	tclass.appendChild(propertyE);
}

void
Spacer::loadSpacer(const QString &wname, Container *container, WidgetLibrary *lib, const QDomElement &el, QWidget *parent)
{
	QWidget *w = lib->createWidget("Spacer", parent, wname.latin1(), container);
	ObjectTreeItem *tree =  new ObjectTreeItem("Spacer", wname, w);
	container->form()->objectTree()->addChild(container->tree(), tree);

	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.toElement().tagName() == "property")
		{
			QString name = n.toElement().attribute("name");
			if((name != "geometry") && (name != "name"))
				continue;

			QVariant val = FormIO::readProp(n.toElement().firstChild(), w, name);
			w->setProperty(name.latin1(), val);
			tree->addModProperty(name, val);
		}
	}
	w->show();
}

}

#include "spacer.moc"

