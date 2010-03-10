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

#include "kis_grid_paintop.h"
#include "kis_grid_paintop_settings.h"

#include <cmath>

#include <QRect>

#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_types.h>
#include <kis_paintop.h>
#include <kis_paint_information.h>
#include <kis_random_sub_accessor.h>

#include <KoColor.h>
#include <KoColorSpace.h>

#include <kis_gridop_option.h>
#include <kis_grid_shape_option.h>
#include <kis_color_option.h>

#ifdef BENCHMARK
    #include <QTime>
#endif

KisGridPaintOp::KisGridPaintOp(const KisGridPaintOpSettings *settings, KisPainter * painter, KisImageWSP image)
    : KisPaintOp( painter )
    , m_settings( settings )
    , m_image ( image )
{

    m_properties.fillProperties(settings);
    m_colorProperties.fillProperties(settings);
    
    m_xSpacing = m_properties.gridWidth * m_properties.scale;
    m_ySpacing = m_properties.gridHeight * m_properties.scale;
    m_spacing = m_xSpacing;

    m_dab = new KisPaintDevice( painter->device()->colorSpace() );
    m_painter = new KisPainter(m_dab);
    m_painter->setPaintColor( painter->paintColor() );
    m_painter->setFillStyle(KisPainter::FillStyleForegroundColor);
    m_pixelSize = settings->node()->paintDevice()->colorSpace()->pixelSize();
#ifdef BENCHMARK
    m_count = m_total = 0;
#endif
    
}

KisGridPaintOp::~KisGridPaintOp()
{
    delete m_painter;
}

double KisGridPaintOp::paintAt(const KisPaintInformation& info)
{
#ifdef BENCHMARK
    QTime time;
    time.start();
#endif

    if (!painter()) return m_spacing;
    m_dab->clear();

    int gridWidth = m_properties.gridWidth * m_properties.scale;
    int gridHeight = m_properties.gridHeight * m_properties.scale;

    int divide;
    if (m_properties.pressureDivision){
        divide = m_properties.divisionLevel * info.pressure();
    }else{
        divide = m_properties.divisionLevel;
    }
    divide = qRound(m_properties.scale * divide);

    int posX = qRound( info.pos().x() );
    int posY = qRound( info.pos().y() );

    QPoint dabPosition( posX - posX % gridWidth, posY - posY % gridHeight );
    QPoint dabRightBottom(dabPosition.x() + gridWidth, dabPosition.y() + gridHeight);

    divide = qMax(1, divide);
    qreal yStep = gridHeight / (qreal)divide;
    qreal xStep = gridWidth / (qreal)divide;

    KisRandomSubAccessorPixel acc = m_settings->node()->paintDevice()->createRandomSubAccessor();
    
    QRectF tile;
    KoColor color( painter()->paintColor() );
   
    qreal vertBorder = m_properties.vertBorder; 
    qreal horzBorder = m_properties.horizBorder;
    if (m_properties.randomBorder){
        if (vertBorder == horzBorder){
            vertBorder = horzBorder = vertBorder * drand48();
        }else{
            vertBorder *= drand48();
            horzBorder *= drand48();
        }
    }
    
    bool shouldColor = true;
    // fill the tile
    if (m_colorProperties.fillBackground){
        m_dab->fill(dabPosition.x(), dabPosition.y(), gridWidth, gridHeight, painter()->backgroundColor().data());
    }
    
    for (int y = 0; y < divide; y++){
        for (int x = 0; x < divide; x++){
            // determine the tile size
            tile = QRectF(dabPosition.x() + x*xStep,dabPosition.y() + y*yStep, xStep, yStep);
            tile.adjust(vertBorder,horzBorder,-vertBorder,-horzBorder);
            tile = tile.normalized();

            // do color transformation
            if (shouldColor){
                if (m_colorProperties.sampleInputColor){
                    acc.moveTo(tile.center().x(), tile.center().y());
                    acc.sampledRawData( color.data() );
                }else{
                    memcpy(color.data(),painter()->paintColor().data(), m_pixelSize);
                }

                // mix the color with background color
                if (m_colorProperties.mixBgColor)
                {       
                    KoMixColorsOp * mixOp = source()->colorSpace()->mixColorsOp();

                    const quint8 *colors[2];
                    colors[0] = color.data();
                    colors[1] = painter()->backgroundColor().data();

                    qint16 colorWeights[2];
                    int MAX_16BIT = 255;
                    qreal blend = info.pressure();

                    colorWeights[0] = static_cast<quint16>( blend * MAX_16BIT); 
                    colorWeights[1] = static_cast<quint16>( (1.0 - blend) * MAX_16BIT); 
                    mixOp->mixColors(colors, colorWeights, 2, color.data() );
                }

                if (m_colorProperties.useRandomHSV){
                    QHash<QString, QVariant> params;
                    params["h"] = (m_colorProperties.hue / 180.0) * drand48();
                    params["s"] = (m_colorProperties.saturation / 100.0) * drand48();
                    params["v"] = (m_colorProperties.value / 100.0) * drand48();

                    KoColorTransformation* transfo;
                    transfo = m_dab->colorSpace()->createColorTransformation("hsv_adjustment", params);
                    transfo->transform(color.data(), color.data() , 1);
                }
                
                if (m_colorProperties.useRandomOpacity){
                    qreal alpha = drand48();
                    color.setOpacity( alpha );
                    m_painter->setOpacity( qRound(alpha * OPACITY_OPAQUE_U8) );
                }

                if ( !m_colorProperties.colorPerParticle ){
                    shouldColor = false;
                }
                
                m_painter->setPaintColor(color);
            }

            // paint some element
            switch (m_properties.shape){
                case 0:
                {
                            m_painter->paintEllipse( tile ); 
                            break;
                }
                case 1: 
                {
                            // anti-aliased version
                            //m_painter->paintRect(tile);
                            m_dab->fill(tile.topLeft().x(), tile.topLeft().y(), tile.width(), tile.height(), color.data());
                            break;
                }
                case 2:
                {
                            m_painter->drawDDALine(tile.topRight(), tile.bottomLeft());
                            break;
                }
                case 3:
                {
                            m_painter->drawLine( tile.topRight(), tile.bottomLeft() );
                            break;
                }
                case 4:
                {
                            m_painter->drawThickLine(tile.topRight(), tile.bottomLeft() , 1,10);
                            break;
                }
                default:
                {
                            kDebug() << " implement or exclude from GUI ";
                            break;
                }
            }
        }
    }
    
    QRect rc = m_dab->extent();
    painter()->bitBlt(rc.topLeft(), m_dab, rc);

    
#ifdef BENCHMARK
    int msec = time.elapsed();
    kDebug() << msec << " ms/dab " << "[average: " << m_total / (qreal)m_count << "]";
    m_total += msec;
    m_count++;
#endif
    return m_spacing;
}

void KisGridProperties::fillProperties(const KisPropertiesConfiguration* setting)
{
    gridWidth = setting->getInt(GRID_WIDTH);
    gridHeight = setting->getInt(GRID_HEIGHT);
    divisionLevel = setting->getInt(GRID_DIVISION_LEVEL);
    pressureDivision =  setting->getBool(GRID_PRESSURE_DIVISION);
    randomBorder = setting->getBool(GRID_RANDOM_BORDER);
    scale = setting->getDouble(GRID_SCALE);
    vertBorder  = setting->getDouble(GRID_VERTICAL_BORDER);
    horizBorder = setting->getDouble(GRID_HORIZONTAL_BORDER);
    
    shape = setting->getInt(GRIDSHAPE_SHAPE);
}
