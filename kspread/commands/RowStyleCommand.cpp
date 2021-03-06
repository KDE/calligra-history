/* This file is part of the KDE project
   Copyright 2009 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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

#include "RowStyleCommand.h"

#include "Damages.h"
#include "kspread_limits.h"
#include "Map.h"
#include "RowColumnFormat.h"
#include "RowFormatStorage.h"
#include "Sheet.h"
#include "SheetPrint.h"

using namespace KSpread;

RowStyleCommand::RowStyleCommand(QUndoCommand *parent)
        : AbstractRegionCommand(parent)
        , m_height(0.0)
        , m_hidden(false)
        , m_pageBreak(false)
{
}

RowStyleCommand::~RowStyleCommand()
{
    qDeleteAll(m_rowFormats);
}

void RowStyleCommand::setHeight(double height)
{
    m_height = height;
}

void RowStyleCommand::setHidden(bool hidden)
{
    m_hidden = hidden;
}

void RowStyleCommand::setPageBreak(bool pageBreak)
{
    m_pageBreak = pageBreak;
}

void RowStyleCommand::setTemplate(const RowFormat &rowFormat)
{
    m_height = rowFormat.height();
    m_hidden = rowFormat.isHidden();
    m_pageBreak = rowFormat.hasPageBreak();
}

bool RowStyleCommand::mainProcessing()
{
    const Region::ConstIterator end(constEnd());
    for (Region::ConstIterator it(constBegin()); it != end; ++it) {
        const QRect range = (*it)->rect();
        // Save the old style.
        if (m_firstrun) {
            for (int row = range.top(); row <= range.bottom(); ++row) {
                if (!m_sheet->rowFormats()->isDefaultRow(row) && !m_rowFormats.contains(row)) {
                    m_rowFormats.insert(row, new RowFormat(m_sheet->rowFormats(), row));
                }
            }
        }
        if (m_reverse) {
            for (int row = range.top(); row <= range.bottom(); ++row) {
                // Set the new style.
                if (m_rowFormats.contains(row)) {
                    m_sheet->insertRowFormat(m_rowFormats.value(row));
                } else {
                    m_sheet->deleteRowFormat(row);
                }
            }
        } else {
            m_sheet->rowFormats()->setRowHeight(range.top(), range.bottom(), m_height);
            m_sheet->rowFormats()->setHidden(range.top(), range.bottom(), m_hidden);
            m_sheet->rowFormats()->setPageBreak(range.top(), range.bottom(), m_pageBreak);
        }
        // Possible visual cache invalidation due to dimension change; rebuild it.
        const Region region(1, range.top(), KS_colMax, KS_rowMax - range.bottom() + 1, m_sheet);
        m_sheet->map()->addDamage(new CellDamage(m_sheet, region, CellDamage::Appearance));
    }
    m_sheet->print()->updateVerticalPageParameters(0);
    return true;
}
