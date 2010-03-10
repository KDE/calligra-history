/*
 *  Copyright (c) 2008 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "kis_deform_paintop.h"
#include "kis_deform_paintop_settings.h"

#include <cmath>

#include <QRect>

#include <kis_image.h>
#include <kis_debug.h>

#include "kis_global.h"
#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_types.h"
#include "kis_paintop.h"
#include "kis_selection.h"
#include "kis_random_accessor.h"

#include <kis_fixed_paint_device.h>

#include "kis_deform_option.h"
#include "kis_brush_size_option.h"

KisDeformPaintOp::KisDeformPaintOp(const KisDeformPaintOpSettings *settings, KisPainter * painter, KisImageWSP image)
        : KisPaintOp(painter)
{
    Q_UNUSED(image);
    Q_ASSERT(settings);
    
    m_sizeProperties.readOptionSetting(settings);
    
    m_properties.action = settings->getInt(DEFORM_ACTION);
    m_properties.deformAmount = settings->getDouble(DEFORM_AMOUNT);
    m_properties.useBilinear = settings->getBool(DEFORM_USE_BILINEAR);
    m_properties.useCounter = settings->getBool(DEFORM_USE_COUNTER);
    m_properties.useOldData = settings->getBool(DEFORM_USE_OLD_DATA);
    
    m_deformBrush.setProperties( &m_properties );
    m_deformBrush.setSizeProperties( &m_sizeProperties );
    m_deformBrush.initDeformAction();
    
    m_useMovementPaint = settings->getBool(DEFORM_USE_MOVEMENT_PAINT);
    m_dev = source();
    
    if ((m_sizeProperties.diameter * 0.5) > 1) {
        m_ySpacing = m_xSpacing = m_sizeProperties.diameter * 0.5 * m_sizeProperties.spacing;
    } else {
        m_ySpacing = m_xSpacing = 1.0;
    }
    m_spacing = m_xSpacing;
}

KisDeformPaintOp::~KisDeformPaintOp()
{
}

double KisDeformPaintOp::paintAt(const KisPaintInformation& info)
{
    if (!painter()) return m_spacing;
    if (!m_dev) return m_spacing;
 
    if (true){
        KisFixedPaintDeviceSP dab = cachedDab(painter()->device()->colorSpace());

        qint32 x;
        double subPixelX;
        qint32 y;
        double subPixelY;
        
        QPointF pt = info.pos();
        if (m_sizeProperties.jitterEnabled){
                pt.setX(pt.x() + (  ( m_sizeProperties.diameter * drand48() ) - m_sizeProperties.diameter * 0.5) * m_sizeProperties.jitterMovementAmount);
                pt.setY(pt.y() + (  ( m_sizeProperties.diameter * drand48() ) - m_sizeProperties.diameter * 0.5) * m_sizeProperties.jitterMovementAmount);
        }
        qreal rotation = m_sizeProperties.rotation;
        qreal scale = m_sizeProperties.scale;

        QPointF pos = pt - m_deformBrush.hotSpot(scale,rotation);
            
        splitCoordinate(pos.x(), &x, &subPixelX);
        splitCoordinate(pos.y(), &y, &subPixelY);
        
        m_deformBrush.paintMask(dab, m_dev, scale,rotation,info.pos(), subPixelX,subPixelY);
        
        painter()->bltFixed(QPoint(x, y), dab, dab->bounds());
        return m_spacing;
    }else{
        if (!m_dab) {
            m_dab = new KisPaintDevice(painter()->device()->colorSpace());
        } else {
            m_dab->clear();
        }

        m_deformBrush.oldDeform(m_dab,m_dev,info.pos());
        QRect rc = m_dab->extent();
        painter()->bitBlt(rc.x(), rc.y(), m_dab, rc.x(), rc.y(), rc.width(), rc.height());
        return m_spacing;
    }
}


double KisDeformPaintOp::spacing(double pressure) const
{
    return m_spacing;
}



