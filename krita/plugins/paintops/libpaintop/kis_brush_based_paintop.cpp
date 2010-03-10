/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "kis_brush_based_paintop.h"
#include "kis_brush.h"
#include "kis_properties_configuration.h"
#include "kis_brush_option.h"

#include <QImage>
#include <QPainter>

KisBrushBasedPaintOp::KisBrushBasedPaintOp(const KisPropertiesConfiguration* settings, KisPainter* painter)
        : KisPaintOp(painter)
{
    Q_ASSERT(settings);
    KisBrushOption brushOption;
    brushOption.readOptionSetting(settings);
    m_brush = brushOption.brush();
}

double KisBrushBasedPaintOp::spacing(double scale) const
{
    // XXX: The spacing should vary as the pressure changes along the line.
    // This is a quick simplification.
    double xSpacing = m_brush->xSpacing(scale);
    double ySpacing = m_brush->ySpacing(scale);

    if (xSpacing < 0.5) {
        xSpacing = 0.5;
    }
    if (ySpacing < 0.5) {
        ySpacing = 0.5;
    }

    if (xSpacing > ySpacing) {
        return xSpacing;
    } else {
        return ySpacing;
    }
}

bool KisBrushBasedPaintOp::canPaint() const
{
    return m_brush != 0;
}
