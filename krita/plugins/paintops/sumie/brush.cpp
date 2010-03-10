/*
 *  Copyright (c) 2008-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "brush.h"
#include "brush_shape.h"
#include "trajectory.h"

#include <KoColor.h>
#include <KoColorSpace.h>
#include <KoColorTransformation.h>

#include <QVariant>
#include <QHash>
#include <QList>

#include "kis_random_accessor.h"

#include <cmath>
#include <ctime>

const float radToDeg = 57.29578f;

const QString HUE = "h";
const QString SATURATION = "s";
const QString VALUE = "v";

#if defined(_WIN32) || defined(_WIN64)
#define srand48 srand
inline double drand48()
{
    return double(rand()) / RAND_MAX;
}
#endif


Brush::Brush()
{
    srand48(time(0));
    m_counter = 0;
    m_lastAngle = 0.0;
    m_oldPressure = 0.0f;

    m_params[HUE] = 0.0;
    m_params[SATURATION] = 0.0;
    m_params[VALUE] = 0.0;
}


void Brush::setBrushShape(BrushShape brushShape)
{
    m_initialShape = brushShape;
    m_bristles = brushShape.getBristles();
}


void Brush::setInkColor(const KoColor &color)
{
    for (int i = 0; i < m_bristles.size(); i++) {
        m_bristles[i]->setColor(color);
    }
}


void Brush::paintLine(KisPaintDeviceSP dev, KisPaintDeviceSP layer, const KisPaintInformation &pi1, const KisPaintInformation &pi2)
{
    m_counter++;

    qreal x1 = pi1.pos().x();
    qreal y1 = pi1.pos().y();

    qreal x2 = pi2.pos().x();
    qreal y2 = pi2.pos().y();

    qreal dx = x2 - x1;
    qreal dy = y2 - y1;
    
    qreal angle = atan2(dy, dx);

    qreal distance = sqrt(dx * dx + dy * dy);

    qreal pressure = pi2.pressure();
    if (m_properties->useMousePressure && pi2.pressure() == 0.5) { // it is mouse and want pressure from mouse movement
        pressure = 1.0 - computeMousePressure(distance);
    } else if (pi2.pressure() == 0.5) { // if it is mouse
        pressure = 1.0;
    }

    Bristle *bristle = 0;
    KoColor bristleColor;

    KisRandomAccessor accessor = dev->createRandomAccessor((int)x1, (int)y1);
    m_pixelSize = dev->colorSpace()->pixelSize();
    m_dabAccessor = &accessor;
    m_dev = dev;

    

    qreal inkDeplation;
    QVariant saturationVariant;

    m_params[SATURATION] = 0.0;
    KoColorTransformation* transfo;
    transfo = m_dev->colorSpace()->createColorTransformation("hsv_adjustment", m_params);
    int saturationId = transfo->parameterId(SATURATION);

    rotateBristles(angle);
    // if this is first time the brush touches the canvas and we use soak the ink from canvas
    if ((m_counter == 1) && m_properties->useSoakInk){
        KisRandomConstAccessor laccessor = layer->createRandomConstAccessor((int)x1, (int)y1);
        colorifyBristles(laccessor,layer->colorSpace() ,pi1.pos());
    }

    qreal fx1, fy1, fx2, fy2;
    int size = m_bristles.size();
     
    QVector<QPointF> bristlePath; // path for single bristle
    int inkDepletionSize = m_properties->inkDepletionCurve.size();
    for (int i = 0; i < size; i++) {

        if (!m_bristles.at(i)->enabled()) continue;
        bristle = m_bristles[i];

        qreal randomX = drand48() * 2;
        qreal randomY = drand48() * 2;
        randomX -= 1.0;
        randomY -= 1.0;
        randomX *= m_properties->randomFactor;
        randomY *= m_properties->randomFactor;

        qreal scale = pressure * m_properties->scaleFactor;
        qreal shear = pressure * m_properties->shearFactor;

        m_transform.reset();
        m_transform.scale(scale, scale);
        m_transform.translate(randomX, randomY);

        m_transform.shear(shear, shear);

        // transform start dab
        m_transform.map(bristle->x(), bristle->y(), &fx1, &fy1);
        // transform end dab
        m_transform.map(bristle->x(), bristle->y(), &fx2, &fy2);

        // all coords relative to device position
        fx1 += x1;
        fy1 += y1;

        fx2 += x2;
        fy2 += y2;

        // paint between first and last dab
        bristlePath = m_trajectory.getLinearTrajectory(QPointF(fx1, fy1), QPointF(fx2, fy2), 1.0);
        int brpathSize = m_trajectory.size();

        bristleColor = bristle->color();
        int bristleCounter = 0;
        
        for (int i = 0; i < brpathSize ; i++) {
            bristleCounter = bristle->increment();
            if (bristleCounter >= inkDepletionSize - 1) {
                inkDeplation = m_properties->inkDepletionCurve[inkDepletionSize - 1];
            } else {
                inkDeplation = m_properties->inkDepletionCurve[bristleCounter];
            }

            // saturation transformation of the bristle ink color
            // add check for hsv transformation
            if (m_properties->useSaturation && transfo != 0) {
                if (m_properties->useWeights) {

                    // new weighted way (experiment)
                    saturationVariant = (
                                            (pressure * m_properties->pressureWeight) +
                                            (bristle->length() * m_properties->bristleLengthWeight) +
                                            (bristle->inkAmount() * m_properties->bristleInkAmountWeight) +
                                            ((1.0 - inkDeplation) * m_properties->inkDepletionWeight)) - 1.0;
                } else {
                    // old way of computing saturation
                    saturationVariant = (
                                            pressure *
                                            bristle->length() *
                                            bristle->inkAmount() *
                                            (1.0 - inkDeplation)) - 1.0;

                }
                transfo->setParameter(saturationId, saturationVariant);
                transfo->transform(bristleColor.data(), bristleColor.data() , 1);
            }

            // opacity transformation of the bristle color
            if (m_properties->useOpacity) {
                qreal opacity = 255.0;
                if (m_properties->useWeights) {
                    opacity = (
                                  (pressure * m_properties->pressureWeight) +
                                  (bristle->length() * m_properties->bristleLengthWeight) +
                                  (bristle->inkAmount() * m_properties->bristleInkAmountWeight) +
                                  ((1.0 - inkDeplation) * m_properties->inkDepletionWeight));

                } else {
                    opacity =
                        /* pressure * */
                        bristle->length() *
                        bristle->inkAmount() *
                        (1.0 - inkDeplation);
                }
                bristleColor.setOpacity(opacity);
            }

            addBristleInk(bristle, bristlePath.at(i).x(), bristlePath.at(i).y(), bristleColor);
            bristle->setInkAmount(1.0 - inkDeplation);
        }

    }
    rotateBristles(-angle);
    //repositionBristles(angle,slope);
    m_dev = 0;
    m_dabAccessor = 0;
}


void Brush::rotateBristles(double angle)
{
    qreal tx, ty, x, y;

    m_transform.reset();
    m_transform.rotateRadians(angle);

    for (int i = 0; i < m_bristles.size(); i++) {
        if (m_bristles.at(i)->enabled()){
            x = m_bristles.at(i)->x();
            y = m_bristles.at(i)->y();
            m_transform.map(x, y, &tx, &ty);
            m_bristles[i]->setX(tx);
            m_bristles[i]->setY(ty);
        }
    }
    m_lastAngle = angle;
}

void Brush::repositionBristles(double angle, double slope)
{
    // setX
    srand48((int)slope);
    for (int i = 0; i < m_bristles.size(); i++) {
        float x = m_bristles[i]->x();
        m_bristles[i]->setX(x + drand48());
    }

    // setY
    srand48((int)angle);
    for (int i = 0; i < m_bristles.size(); i++) {
        float y = m_bristles[i]->y();
        m_bristles[i]->setY(y + drand48());
    }
}

Brush::~Brush(){}

inline void Brush::addBristleInk(Bristle *bristle, float wx, float wy, const KoColor &color)
{
    int ix = qRound(wx);
    int iy = qRound(wy);
    m_dabAccessor->moveTo(ix, iy);
    if (m_dev->colorSpace()->opacityU8(m_dabAccessor->rawData()) < color.opacityU8()) {
        memcpy(m_dabAccessor->rawData(), color.data(), m_pixelSize);
    }
    bristle->upIncrement();
}

void Brush::oldAddBristleInk(Bristle *bristle, float wx, float wy, const KoColor &color)
{
    KoMixColorsOp * mixOp = m_dev->colorSpace()->mixColorsOp();
    m_dabAccessor->moveTo((int)wx, (int)wy);
    const quint8 *colors[2];
    colors[0] = color.data();
    colors[1] = m_dabAccessor->rawData(); // this is always (0,0,0) in RGB

    qint16 colorWeights[2];

    colorWeights[0] = static_cast<quint8>(color.opacityU8());
    colorWeights[1] = static_cast<quint8>(255 - color.opacityU8());
    mixOp->mixColors(colors, colorWeights, 2, m_dabAccessor->rawData());

    //memcpy ( m_dabAccessor->rawData(), color.data(), m_pixelSize );
    // bristle delivered some ink
    bristle->upIncrement();
}


void Brush::putBristle(Bristle *bristle, float wx, float wy, const KoColor &color)
{
    m_dabAccessor->moveTo((int)wx, (int)wy);
    memcpy(m_dabAccessor->rawData(), color.data(), m_pixelSize);
    // bristle delivered some ink
    bristle->upIncrement();
}

double Brush::computeMousePressure(double distance)
{
    double scale = 20.0;
    double minPressure = 0.02;
    double oldPressure = m_oldPressure;

    double factor = 1.0 - distance / scale;
    if (factor < 0.0) factor = 0.0;

    double result = ((4.0 * oldPressure) + minPressure + factor) / 5.0;
    m_oldPressure = result;
    return result;
}


void Brush::colorifyBristles(KisRandomConstAccessor& acc, KoColorSpace * cs, QPointF point)
{
    QPoint p = point.toPoint();
    KoColor color(cs);
    int pixelSize = cs->pixelSize();

    Bristle *b = 0;
    int size = m_bristles.size();
    for (int i = 0; i < size; i++){
        b = m_bristles[i];
        int x = qRound(b->x() + point.x());
        int y = qRound(b->y() + point.y());
        acc.moveTo(x,y);
        memcpy(color.data(),acc.rawData(),pixelSize);
        b->setColor(color);
    }
    
}

