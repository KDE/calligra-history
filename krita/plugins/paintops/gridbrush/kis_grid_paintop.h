/*
 * Copyright (c) 2009,2010 Lukáš Tvrdý (lukast.dev@gmail.com)
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

#ifndef KIS_GRID_PAINTOP_H_
#define KIS_GRID_PAINTOP_H_

//#define BENCHMARK

#include <klocale.h>
#include <kis_paintop.h>
#include <kis_types.h>
#include <kis_color_option.h>

#include <KoColor.h>

#include "kis_grid_paintop_settings.h"


class QPointF;
class KisPainter;

class KisGridProperties{
public:
    quint16 gridWidth;
    quint16 gridHeight;
    quint16 divisionLevel;
    bool pressureDivision;
    bool randomBorder;
    qreal scale;
    qreal vertBorder;
    qreal horizBorder;
    
    quint8 shape;
public: 
    void fillProperties(const KisPropertiesConfiguration* setting);
};

class KisGridPaintOp : public KisPaintOp
{

public:

    KisGridPaintOp(const KisGridPaintOpSettings *settings, KisPainter * painter, KisImageWSP image);
    virtual ~KisGridPaintOp();

    double paintAt(const KisPaintInformation& info);

    virtual bool incremental() const {
        return false;
    }

private:
    const KisGridPaintOpSettings* m_settings;
    KisImageWSP         m_image;
    KisPaintDeviceSP    m_dab;
    KisPainter*         m_painter;
    double              m_xSpacing;
    double              m_ySpacing;
    double              m_spacing;
    int                 m_pixelSize;
    KisGridProperties   m_properties;
    KisColorProperties  m_colorProperties;

    
#ifdef BENCHMARK
    int m_total;
    int m_count;
#endif
    
};

#endif // KIS_GRID_PAINTOP_H_
