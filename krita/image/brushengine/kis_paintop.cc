/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004,2007,2010 Cyrille Berger <cberger@cberger.net>
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

#include "kis_paintop.h"

#include <math.h>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoPointerEvent.h>

#include "kis_painter.h"
#include "kis_layer.h"
#include "kis_types.h"

#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_global.h"
#include "kis_iterators_pixel.h"
#include "kis_datamanager.h"
#include "kis_paintop_settings.h"
#include "kis_paintop_preset.h"
#include "kis_paint_information.h"
#include "kis_vec.h"
#include "kis_perspective_math.h"
#include "kis_fixed_paint_device.h"

#define BEZIER_FLATNESS_THRESHOLD 0.5
#define MAXIMUM_SCALE 2
#include <kis_distance_information.h>

struct KisPaintOp::Private {
    Private()
            : dab(0) {
    }

    KisFixedPaintDeviceSP dab;
    KoColor color;
    KoColor previousPaintColor;
    KisPainter* painter;
};


KisPaintOp::KisPaintOp(KisPainter * painter) : d(new Private)
{
    d->painter = painter;
}

KisPaintOp::~KisPaintOp()
{
    d->dab.clear();
    delete d;
}

KisFixedPaintDeviceSP KisPaintOp::cachedDab()
{
    return cachedDab(d->painter->device()->colorSpace());
}

KisFixedPaintDeviceSP KisPaintOp::cachedDab(const KoColorSpace *cs)
{
    if (!d->dab || !(*d->dab->colorSpace() == *cs)) {
        d->dab = new KisFixedPaintDevice(cs);
    }
    return d->dab;
}

void KisPaintOp::splitCoordinate(double coordinate, qint32 *whole, double *fraction) const
{
    qint32 i = static_cast<qint32>(coordinate);

    if (coordinate < 0) {
        // We always want the fractional part to be positive.
        // E.g. -1.25 becomes -2 and +0.75
        i--;
    }

    double f = coordinate - i;

    *whole = i;
    *fraction = f;
}

static KisDistanceInformation paintBezierCurve(KisPaintOp *paintOp,
                               const KisPaintInformation &pi1,
                               const KisVector2D &control1,
                               const KisVector2D &control2,
                               const KisPaintInformation &pi2,
                               const KisDistanceInformation& savedDist)
{
    KisDistanceInformation newDistance;
    LineEquation line = LineEquation::Through(toKisVector2D(pi1.pos()), toKisVector2D(pi2.pos()));
    qreal d1 = line.absDistance(control1);
    qreal d2 = line.absDistance(control2);

    if ((d1 < BEZIER_FLATNESS_THRESHOLD && d2 < BEZIER_FLATNESS_THRESHOLD)
#ifdef Q_CC_MSVC
            || isnan(d1) || isnan(d2)) {
#else
            || std::isnan(d1) || std::isnan(d2)) {
#endif
        newDistance = paintOp->paintLine(pi1, pi2, savedDist);
    } else {
        // Midpoint subdivision. See Foley & Van Dam Computer Graphics P.508
        KisVector2D l2 = (toKisVector2D(pi1.pos()) + control1) / 2;
        KisVector2D h = (control1 + control2) / 2;
        KisVector2D l3 = (l2 + h) / 2;
        KisVector2D r3 = (control2 + toKisVector2D(pi2.pos())) / 2;
        KisVector2D r2 = (h + r3) / 2;
        KisVector2D l4 = (l3 + r2) / 2;

        KisVector2D midMovement = (pi1.movement() + pi2.movement()) * 0.5;
        
        KisPaintInformation middlePI = KisPaintInformation::mix(toQPointF(l4), 0.5, pi1, pi2, midMovement);
        
        newDistance = paintBezierCurve(paintOp, pi1, l2, l3, middlePI, savedDist);
        newDistance = paintBezierCurve(paintOp, middlePI, r2, r3, pi2, newDistance);
    }

    return newDistance;
}

KisDistanceInformation KisPaintOp::paintBezierCurve(const KisPaintInformation &pi1,
                                    const QPointF &control1,
                                    const QPointF &control2,
                                    const KisPaintInformation &pi2,
                                    const KisDistanceInformation& savedDist)
{
    return ::paintBezierCurve(this, pi1, toKisVector2D(control1), toKisVector2D(control2), pi2, savedDist);
}


KisDistanceInformation KisPaintOp::paintLine(const KisPaintInformation &pi1,
                             const KisPaintInformation &pi2,
                             const KisDistanceInformation& savedDist)
{
    KisVector2D end = toKisVector2D(pi2.pos());
    KisVector2D start = toKisVector2D(pi1.pos());

    KisVector2D dragVec = end - start;

    Q_ASSERT(savedDist.distance >= 0);

    double endDist = dragVec.norm();
    double currentDist = savedDist.distance;

    dragVec.normalize();
    KisVector2D step(0, 0);

    double sp = savedDist.spacing;
    while (currentDist < endDist) {

        QPointF p = toQPointF(start +  currentDist * dragVec);

        double t = currentDist / endDist;

        sp = paintAt(KisPaintInformation::mix(p, t, pi1, pi2, sp * dragVec));
        currentDist += sp;
    }

    QRect r(pi1.pos().toPoint(), pi2.pos().toPoint());
    d->painter->addDirtyRect(r.normalized());

    return KisDistanceInformation(currentDist - endDist, sp);
}

KisPainter* KisPaintOp::painter() const
{
    return d->painter;
}

KisPaintDeviceSP KisPaintOp::source() const
{
    return d->painter->device();
}

double KisPaintOp::scaleForPressure(double pressure)
{
    double scale = pressure / PRESSURE_DEFAULT;

    if (scale < 0) {
        scale = 0;
    }

    if (scale > MAXIMUM_SCALE) {
        scale = MAXIMUM_SCALE;
    }

    return scale;
}
