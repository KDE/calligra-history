/* This file is part of the KDE project

   Copyright 2010 Johannes Simon <johannes.simon@gmail.com>

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

#ifndef KCHART_SINGLE_MODEL_HELPER_H
#define KCHART_SINGLE_MODEL_HELPER_H

// Qt
#include <QObject>

namespace KChart {

class ChartProxyModel;
class Table;

class SingleModelHelper : public QObject
{
    Q_OBJECT

public:
    SingleModelHelper( Table *table, ChartProxyModel *proxyModel );

private slots:
    void slotModelStructureChanged();

private:
    Table *const m_table;
    ChartProxyModel *const m_proxyModel;
};

} // namespace KChart

#endif // KCHART_SINGLE_MODEL_HELPER_H
