/* This file is part of the KDE project
 * Copyright ( C ) 2007 Thorsten Zachmann <zachmann@kde.org>
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

#include "KPrPageData.h"
#include <KoShape.h>
#include <KoTextShapeData.h>

KPrPageData::KPrPageData()
{
}

KPrPageData::~KPrPageData()
{
}

KPrShapeAnimations & KPrPageData::animations()
{
    return m_animations;
}

KPrPlaceholders & KPrPageData::placeholders()
{
    return m_placeholders;
}

const KPrPlaceholders & KPrPageData::placeholders() const
{
    return m_placeholders;
}

KoTextShapeData* KPrPageData::pageTitle()
{
    return getFirstTextShapeByClass("title");
}

KoTextShapeData* KPrPageData::pageOutline()
{
    return getFirstTextShapeByClass("outline");
}

KoTextShapeData* KPrPageData::pageSubtitle()
{
    return getFirstTextShapeByClass("subtitle");
}

KoTextShapeData* KPrPageData::getFirstTextShapeByClass(QString shapeClass)
{
    KoShape *shape = m_placeholders.getFirstPlaceholderByClass(shapeClass);
    return (shape == NULL) ? NULL : (qobject_cast<KoTextShapeData*>( shape->userData() ));
}
