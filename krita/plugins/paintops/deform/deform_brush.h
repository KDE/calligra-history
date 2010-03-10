/*
 *  Copyright (c) 2008, 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef _DEFORM_BRUSH_H_
#define _DEFORM_BRUSH_H_

#include <kis_paint_device.h>
#include <kis_paint_information.h>

#include <kis_brush_size_option.h>

#if defined(_WIN32) || defined(_WIN64)
#define srand48 srand
inline double drand48()
{
    return double(rand()) / RAND_MAX;
}
#endif

class DeformProperties
{
public:
    int action;
    qreal deformAmount;
    bool useBilinear;
    bool useCounter;
    bool useOldData;
};


class DeformBase{
    public:
        DeformBase(){}
        virtual ~DeformBase(){}
        virtual void transform(qreal * x, qreal * y, qreal distance){
        };
};

/// Inverse weighted inverse scaling - grow&shrink
class DeformScale : public DeformBase {

public:
    void setFactor(qreal factor){ m_factor = factor; }
    qreal factor(){ return m_factor; }
    virtual void transform(qreal* x, qreal* y, qreal distance){
        qreal scaleFactor = (1.0 - distance) * m_factor + distance;
        *x = *x / scaleFactor;
        *y = *y / scaleFactor;
    }

private:
    qreal m_factor;
};

/// Inverse weighted rotation - swirlCW&&swirlCWW
class DeformRotation : public DeformBase {

public:
    void setAlpha(qreal alpha){ m_alpha = alpha; }
    virtual void transform(qreal* maskX, qreal* maskY, qreal distance){
            distance = 1.0 - distance;
            qreal rotX = cos(-m_alpha * distance) * (*maskX) - sin(-m_alpha * distance) * (*maskY);
            qreal rotY = sin(-m_alpha * distance) * (*maskX) + cos(-m_alpha * distance) * (*maskY);
            
            *maskX = rotX;
            *maskY = rotY;
    }

private:
    qreal m_alpha;
};

/// Inverse move
class DeformMove : public DeformBase {
public:
    void setFactor(qreal factor){ m_factor = factor; }
    void setDistance(qreal dx, qreal dy){ m_dx = dx; m_dy = dy; }
    virtual void transform(qreal* maskX, qreal* maskY, qreal distance){
            *maskX -= m_dx * m_factor * (1.0 - distance);
            *maskY -= m_dy * m_factor * (1.0 - distance);
    }

private:
    qreal m_dx;
    qreal m_dy;
    qreal m_factor;
};

/// Inverse lens distortion
class DeformLens : public DeformBase {
public:
    void setLensFactor(qreal k1, qreal k2){ m_k1 = k1; m_k2 = k2; }
    void setMaxDistance(qreal maxX, qreal maxY){ m_maxX = maxX; m_maxY = maxY;}
    void setMode(bool out){ m_out = out; }
    
    virtual void transform(qreal* maskX, qreal* maskY, qreal distance){
            //normalize
            qreal normX = *maskX / m_maxX;
            qreal normY = *maskY / m_maxY;

            qreal radius_2 = normX * normX  + normY * normY;
            qreal radius_4 = radius_2 * radius_2;

            if (m_out) {
                *maskX = normX * (1.0 + m_k1 * radius_2 + m_k2 * radius_4);
                *maskY = normY * (1.0 + m_k1 * radius_2 + m_k2 * radius_4);
            } else {
                *maskX = normX / (1.0 + m_k1 * radius_2 + m_k2 * radius_4);
                *maskY = normY / (1.0 + m_k1 * radius_2 + m_k2 * radius_4);
            }

            *maskX = m_maxX * (*maskX);
            *maskY = m_maxY * (*maskY);
    }

private:
    qreal m_k1,m_k2;
    qreal m_maxX, m_maxY;
    bool m_out;
};

/// Randomly disturb the pixels
class DeformColor : public DeformBase {
public:
    DeformColor(){ srand48(time(0)); }
    
    void setFactor(qreal factor){ m_factor = factor; }
    virtual void transform(qreal* x, qreal* y, qreal distance){
        qreal randomX = m_factor * ((drand48() * 2.0) - 1.0);
        qreal randomY = m_factor * ((drand48() * 2.0) - 1.0);
        *x += randomX;
        *y += randomY;
    }

private:
    qreal m_factor;
};





class DeformBrush
{

public:
    DeformBrush();
    ~DeformBrush();
    
    void paintMask(KisFixedPaintDeviceSP dab, KisPaintDeviceSP layer,
                   qreal scale,qreal rotation,QPointF pos,
                   qreal subPixelX,qreal subPixelY);
                   
    void oldDeform(KisPaintDeviceSP dab,KisPaintDeviceSP layer,QPointF pos);
    void setSizeProperties(KisBrushSizeProperties * properties){ m_sizeProperties = properties; }
    void setProperties(DeformProperties * properties){ m_properties = properties; }
    void initDeformAction();
    QPointF hotSpot(qreal scale, qreal rotation);

private:
    // return true if can paint
    bool setupAction(QPointF pos);
    /// move pixel from new computed coords newX, newY to x,y (inverse mapping)
    void movePixel(qreal newX, qreal newY, quint8 *dst);
    void debugColor(const quint8* data, KoColorSpace * cs);

    qreal maskWidth(qreal scale){
        return m_sizeProperties->diameter * scale;
    }
    
    qreal maskHeight(qreal scale){
        return m_sizeProperties->diameter * m_sizeProperties->aspect  * scale;
    }

    inline qreal norme(qreal x,qreal y){
        return x*x + y*y; 
    }


private:
    KisRandomSubAccessorPixel * m_srcAcc;
    bool m_firstPaint;
    qreal m_prevX, m_prevY;
    int m_counter;
    quint32 m_pixelSize;

    qreal m_centerX;
    qreal m_centerY;
    qreal m_majorAxis;
    qreal m_minorAxis;
    qreal m_inverseScale;
    qreal m_maskRadius;
    QRectF m_maskRect;

    DeformBase * m_deformAction;
    
    DeformProperties * m_properties;
    KisBrushSizeProperties * m_sizeProperties;
};


#endif
