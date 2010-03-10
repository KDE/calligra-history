/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
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

#include "mypaint_paintop.h"
#include "mypaint_paintop_settings.h"
#include <kis_debug.h>

#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_paint_information.h>
#include <kis_paintop_registry.h>

#include "mypaint_paintop_factory.h"
#include "mypaint_brush_resource.h"
#include "mypaint_paintop_settings.h"
#include "mypaint_surface.h"

MyPaint::MyPaint(const MyPaintSettings *settings, KisPainter * painter, KisImageWSP image)
    : KisPaintOp(painter)
    , m_settings(settings)
{
    Q_ASSERT(settings);
    Q_UNUSED(image);

    m_surface = new MyPaintSurface(settings->node()->projection(), painter->device());
    MyPaintFactory *factory = static_cast<MyPaintFactory*>(KisPaintOpRegistry::instance()->get("mypaintbrush"));
    m_brush = factory->brush(settings->getString("filename"));
    m_brush->new_stroke();
    QColor c = painter->paintColor().toQColor();
    qreal h, s, v, a;
    c.getHsvF(&h, &s, &v, &a);
    m_brush->set_color_hsv((float)h, (float)s, (float)v);
    m_mypaintThinksStrokeHasEnded = false;
    m_eventTime.start(); // GTK puts timestamps in its events, Qt doesn't, so fake it.
}

MyPaint::~MyPaint()
{
    delete m_surface;
}

double MyPaint::paintAt(const KisPaintInformation& info)
{
    if (m_mypaintThinksStrokeHasEnded) {
        m_settings->brush()->new_stroke();
    }
    m_mypaintThinksStrokeHasEnded =
                        m_brush->stroke_to(m_surface,
                                           info.pos().x(),
                                           info.pos().y(),
                                           info.pressure(),
                                           double(m_eventTime.elapsed()) / 1000);
    return 1.0;
}

KisDistanceInformation MyPaint::paintLine(const KisPaintInformation &pi1, const KisPaintInformation &pi2, const KisDistanceInformation& savedDist)
{
    Q_UNUSED(savedDist);

    if (!painter()) return KisDistanceInformation();

    if (m_mypaintThinksStrokeHasEnded) {
        m_brush->new_stroke();
    }
    m_mypaintThinksStrokeHasEnded = m_brush->stroke_to(m_surface,
                                                       pi1.pos().x(), pi1.pos().y(),
                                                       pi1.pressure(),
                                                       double(m_eventTime.elapsed()) / 1000);
    if (m_mypaintThinksStrokeHasEnded) {
        m_brush->new_stroke();
    }
    m_mypaintThinksStrokeHasEnded = m_brush->stroke_to(m_surface,
                                                       pi2.pos().x(), pi2.pos().y(),
                                                       pi2.pressure(),
                                                       double(m_eventTime.elapsed()) / 1000);

    // not sure what to do with these...
    KisVector2D end = toKisVector2D(pi2.pos());
    KisVector2D start = toKisVector2D(pi1.pos());
    KisVector2D dragVec = end - start;
    return KisDistanceInformation(0, dragVec.norm());
}
