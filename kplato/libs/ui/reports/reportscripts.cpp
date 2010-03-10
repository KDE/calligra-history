/* This file is part of the KDE project
  Copyright (C) 2010 Dag Andersen <danders@get2net.dk>

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
  Boston, MA 02110-1301, USA.
*/

#include "reportscripts.h"

#include "reportdata.h"

namespace KPlato
{


ProjectAccess::ProjectAccess( ReportData *rd )
    : m_reportdata( rd )
{
}

QString ProjectAccess::Name() const
{
    if ( m_reportdata && m_reportdata->project() ) {
        return m_reportdata->project()->name();
    }
    return QString();
}

QString ProjectAccess::Manager() const
{
    if ( m_reportdata && m_reportdata->project() ) {
        return m_reportdata->project()->leader();
    }
    return QString();
}


} // namespace KPlato

#include "reportscripts.moc"
