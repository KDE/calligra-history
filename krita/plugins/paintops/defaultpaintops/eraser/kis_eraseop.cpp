/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004-2008 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#include "kis_eraseop.h"

#include <QRect>

#include <kis_image.h>
#include <kis_debug.h>

#include <KoColorTransformation.h>
#include <KoColor.h>
#include <KoCompositeOp.h>

#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_brush_based_paintop_settings.h>

KisEraseOp::KisEraseOp(const KisBrushBasedPaintOpSettings *settings, KisPainter *painter, KisImageWSP image)
        : KisBrushBasedPaintOp(settings, painter)
{
    Q_UNUSED(image);
    Q_ASSERT(settings);
    Q_ASSERT(painter);
    m_sizeOption.readOptionSetting(settings);
    m_opacityOption.readOptionSetting(settings);
    m_sizeOption.sensor()->reset();
    m_opacityOption.sensor()->reset();
}

KisEraseOp::~KisEraseOp()
{
}

double KisEraseOp::paintAt(const KisPaintInformation& info)
{
// Erasing is traditionally in paint applications one of two things:
// either it is painting in the 'background' color, or it is replacing
// all pixels with transparent (black?) pixels.
//
// That's what this paint op does for now; however, anyone who has
// ever worked with paper and soft pencils knows that a sharp piece of
// eraser rubber is a pretty useful too for making sharp to fuzzy lines
// in the graphite layer, or equally useful: for smudging skin tones.
//
// A smudge tool for Krita is in the making, but when working with
// a tablet, the eraser tip should be at least as functional as a rubber eraser.
// That means that only after repeated or forceful application should all the
// 'paint' or 'graphite' be removed from the surface -- a kind of pressure
// sensitive, incremental smudge.
//
// And there should be an option to not have the eraser work on certain
// kinds of material. Layers are just a hack for this; putting your ink work
// in one layer and your pencil in another is not the same as really working
// with the combination.

    if (!painter()->device()) return 1.0;

    KisBrushSP brush = m_brush;
    if (!m_brush)
        return 1.0;
    
    if (! brush->canPaintFor(info))
        return 1.0;

    double scale = KisPaintOp::scaleForPressure(m_sizeOption.apply(info));
    if ((scale * brush->width()) <= 0.01 || (scale * brush->height()) <= 0.01) return spacing(scale);

    KisPaintDeviceSP device = painter()->device();
    QPointF hotSpot = brush->hotSpot(scale, scale);
    QPointF pt = info.pos() - hotSpot;

    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);

    KisFixedPaintDeviceSP dab = cachedDab(device->colorSpace());

    quint8 origOpacity = m_opacityOption.apply(painter(), info);

    QRect dabRect = QRect(0, 0, brush->maskWidth(scale, 0.0), brush->maskHeight(scale, 0.0));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());


    if (painter()->bounds().isValid()) {
        dstRect &= painter()->bounds();
    }

    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return 1.0;

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    if (brush->brushType() == IMAGE || brush->brushType() == PIPE_IMAGE) {
        dab = brush->paintDevice(device->colorSpace(), scale, 0.0, info, xFraction, yFraction);
    } else {
        dab = cachedDab();
        KoColor color = painter()->paintColor();
        color.convertTo(dab->colorSpace());
        brush->mask(dab, color, scale, scale, 0.0, info, xFraction, yFraction);
    }

    const KoCompositeOp* op = painter()->compositeOp();
    painter()->setCompositeOp(COMPOSITE_ERASE);
    painter()->bltFixed(dstRect.x(), dstRect.y(), dab, sx, sy, sw, sh);
    painter()->setCompositeOp(op);

    painter()->setOpacity(origOpacity);

    return spacing(scale);
}
