/* This file is part of the KDE project

   Copyright (c) 2010 Cyril Oblikov <munknex@gmail.com>

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

#include "TreeShapeFactory.h"

#include "TreeShape.h"
//#include "TreeShapeConfigWidget.h"

#include <KoXmlNS.h>

#include <klocale.h>
#include <kdebug.h>

TreeShapeFactory::TreeShapeFactory()
    : KoShapeFactoryBase(TREESHAPEID, i18n("TreeShape"))
{
    setToolTip(i18n("Tree for mind maps"));
    setIcon("x-shape-image");
    setLoadingPriority(2);
    setOdfElementNames(KoXmlNS::draw, QStringList("tree"));
}

TreeShapeFactory::~TreeShapeFactory() {}

KoShape *TreeShapeFactory::createDefaultShape(KoResourceManager *documentResources) const
{
    TreeShape *defaultShape = new TreeShape(documentResources);
    defaultShape->setShapeId(TREESHAPEID);

    return defaultShape;
}

bool TreeShapeFactory::supports(const KoXmlElement &e) const
{
    kDebug() << ((e.localName() == "tree") && (e.namespaceURI() == KoXmlNS::draw));
    return ((e.localName() == "tree") && (e.namespaceURI() == KoXmlNS::draw));
}

// QList<KoShapeConfigWidgetBase*> TreeShapeFactory::createShapeOptionPanels()
// {
//     QList<KoShapeConfigWidgetBase*> panels;
//     panels.append(new TreeShapeConfigWidget());
//     return panels;
// }