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

#include "kis_smudgeop.h"

#include <QRect>

#include <kis_debug.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorTransformation.h>
#include <KoColor.h>
#include <KoCompositeOp.h>

#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_selection.h>
#include <kis_brush_based_paintop_settings.h>

KisSmudgeOp::KisSmudgeOp(const KisBrushBasedPaintOpSettings *settings, KisPainter *painter, KisImageWSP image)
        : KisBrushBasedPaintOp(settings, painter)
        , m_firstRun(true)
        , m_target(0)
        , m_srcdev(0)
{
    Q_UNUSED(image);
    Q_ASSERT(settings);
    Q_ASSERT(painter);
    m_sizeOption.readOptionSetting(settings);
    m_opacityOption.readOptionSetting(settings);
    m_rateOption.readOptionSetting(settings);
    m_sizeOption.sensor()->reset();
    m_opacityOption.sensor()->reset();
    m_rateOption.sensor()->reset();

    m_srcdev = new KisPaintDevice(painter->device()->colorSpace());
    m_target = new KisPaintDevice(painter->device()->colorSpace());

}

KisSmudgeOp::~KisSmudgeOp()
{
}

double KisSmudgeOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()->device()) return 1.0;

    KisBrushSP brush = m_brush;
    if (!brush)
        return 1.0;

    if (! brush->canPaintFor(info))
        return 1.0;

    double scale = KisPaintOp::scaleForPressure(m_sizeOption.apply(info));
    if ((scale * brush->width()) <= 0.01 || (scale * brush->height()) <= 0.01) return 1.0;

    KisPaintDeviceSP device = painter()->device();
    QPointF hotSpot = brush->hotSpot(scale, scale);
    QPointF pt = info.pos() - hotSpot;

    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);

    KisFixedPaintDeviceSP dab = 0;

    QRect dabRect = QRect(0, 0, brush->maskWidth(scale, 0.0), brush->maskHeight(scale, 0.0));
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());
    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return 1.0;

    if (brush->brushType() == IMAGE || brush->brushType() == PIPE_IMAGE) {
        dab = brush->paintDevice(device->colorSpace(), scale, 0.0, info, xFraction, yFraction);
        dab->convertTo(KoColorSpaceRegistry::instance()->alpha8());
    } else {
        dab = cachedDab();
        KoColor color = painter()->paintColor();
        color.convertTo(dab->colorSpace());
        brush->mask(dab, color, scale, scale, 0.0, info, xFraction, yFraction);
        dab->convertTo(KoColorSpaceRegistry::instance()->alpha8());
    }

    qint32 sw = dab->bounds().width();
    qint32 sh = dab->bounds().height();

    /* To smudge, one does the following:
         * at first, initialize a temporary paint device with a copy of the original (dab-sized piece, really).
         * all other times:
             reduce the transparency of the temporary paint device so as to let it mix gradually
         * combine the temp device with the piece the brush currently is 'painting', according to a mix (opacity)
             note that in the first step, this does the actual copying of the data
         * this combination is then composited upon the actual image
       TODO: what happened exactly in 1.6 (and should happen now) when the dab resizes halfway due to pressure?
    */
    int opacity = OPACITY_OPAQUE_U8;
    if (!m_firstRun) {
        opacity = m_rateOption.apply(opacity, info);

        KisRectIterator it = m_srcdev->createRectIterator(0, 0, sw, sh);
        KoColorSpace* cs = m_srcdev->colorSpace();
        while (!it.isDone()) {
            cs->setOpacity(it.rawData(), quint8(cs->opacityF(it.rawData()) * opacity), 1);
            ++it;
        }

        opacity = OPACITY_OPAQUE_U8 - opacity;
    } else {
        m_firstRun = false;
    }

    KisPainter copyPainter(m_srcdev);
    copyPainter.setOpacity(opacity);
    copyPainter.bitBlt(0, 0, device, pt.x(), pt.y(), sw, sh);
    copyPainter.end();

    m_target->clear();

    // Looks hacky, but we lost bltMask, or the ability to easily convert alpha8 paintdev to selection?
    KisSelectionSP dabAsSelection = new KisSelection();
    copyPainter.begin(dabAsSelection);
    copyPainter.setOpacity(OPACITY_OPAQUE_U8);
    copyPainter.setCompositeOp(COMPOSITE_COPY);
    copyPainter.bltFixed(0, 0, dab, 0, 0, sw, sh);
    copyPainter.end();

    copyPainter.begin(m_target);
    copyPainter.setCompositeOp(COMPOSITE_OVER);
    copyPainter.setSelection(dabAsSelection);
    copyPainter.bitBlt(0, 0, m_srcdev, 0, 0, sw, sh);
    copyPainter.end();

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    sw = dstRect.width();
    sh = dstRect.height();

    painter()->bitBlt(dstRect.x(), dstRect.y(), m_target, sx, sy, sw, sh);
    return spacing(scale);
}
