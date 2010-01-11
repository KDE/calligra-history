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

#include "KoSuperDockerFactory.h"
#include "KoSuperDockerDocker.h"

KoSuperDockerFactory::KoSuperDockerFactory()
{
}

QString KoSuperDockerFactory::id() const
{
    return QString("Super Docker");
}

QDockWidget* KoSuperDockerFactory::createDockWidget()
{
    KoSuperDockerDocker* widget = new KoSuperDockerDocker();
    widget->setObjectName(id());

    return widget;
}

KoDockFactory::DockPosition KoSuperDockerFactory::defaultDockPosition() const
{
    return DockMinimized;
}
