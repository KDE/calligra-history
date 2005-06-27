/***************************************************************************
*   Copyright (C) 2004 by Alexander Dymo                                  *
*   cloudtemple@mskat.net                                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Library General Public License as       *
*   published by the Free Software Foundation; either version 2 of the    *
*   License, or (at your option) any later version.                       *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with this program; if not, write to the                 *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#ifndef KUDESIGNERPROPERTYSERIALIZER_H
#define KUDESIGNERPROPERTYSERIALIZER_H

#include <property.h>

using namespace KOProperty;

namespace Kudesigner
{

/**
@short Contains functions to convert Property values to strings
*/
class PropertySerializer
{
public:
    PropertySerializer();
    ~PropertySerializer();

    static QString toString( Property *prop );
    static QVariant fromString( Property *prop, const QString &str );
};

}

#endif
