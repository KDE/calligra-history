/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

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

#ifndef KODOM_H
#define KODOM_H

#include <qdom.h>

/**
 * This namespace contains a few convenience functions to simplify code using QDom
 * (when loading OASIS documents, in particular).
 */
namespace KoDom {

    /**
     * A namespace-aware version of QDomNode::namedItem(),
     * which also takes care of casting to a QDomElement.
     * Use this when a domelement is known to have only *one* child element
     * with a given tagname.
     *
     * Note: do *NOT* use getElementsByTagNameNS, it's recursive!
     */
    QDomElement namedItemNS( const QDomNode& node, const char* nsURI, const char* localName );

};

#endif /* KODOM_H */

