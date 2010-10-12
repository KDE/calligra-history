/*
 *  kis_tool_dyna.cpp - part of Krita
 *
 *  Copyright (c) 2009 Lukáš Tvrdý <LukasT.dev@gmail.com>
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

#include "kis_tool_dyna.h"
#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QPainter>
#include <QRect>
#include <QCheckBox>
#include <QGridLayout>
#include <QSlider>
#include <QComboBox>

#include <kis_debug.h>
#include <klocale.h>

#include "KoPointerEvent.h"
#include "KoCanvasBase.h"

#include "kis_config.h"
#include "kis_paintop_preset.h"
#include "kis_paintop_registry.h"
#include "kis_paint_information.h"

#include "kis_cursor.h"
#include "kis_painter.h"

#include "kis_paint_device.h"
#include "kis_layer.h"
#include <QDoubleSpinBox>

#define MAXIMUM_SMOOTHNESS 1000
#define MAXIMUM_MAGNETISM 1000

KisToolDyna::KisToolDyna(KoCanvasBase * canvas)
        : KisToolFreehand(canvas, KisCursor::load("tool_freehand_cursor.png", 5, 5), i18n("Dyna"))
{
    setObjectName("tool_dyna");

    m_rate = 200; // Conveniently hardcoded for now
    m_timer = new QTimer(this);
    Q_CHECK_PTR(m_timer);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeoutPaint()));
    
    initDyna();
}


void KisToolDyna::initDyna()
{
    /* dynadraw init */
    m_curmass = 0.5;
    m_curdrag = 0.15;
    m_mouse.fixedangle = true;
    m_width = 1.5;
    m_xangle = 0.60;
    m_yangle = 0.20;
    m_widthRange = 0.05;

    m_previousPressure = 0.5;
}


KisToolDyna::~KisToolDyna()
{
    delete m_timer;
    m_timer = 0;
}

void KisToolDyna::timeoutPaint()
{
    Q_ASSERT(currentPaintOpPreset()->settings()->isAirbrushing());

    if (currentImage() && m_painter) {
        paintAt(m_previousPaintInformation);
        QRegion r = m_painter->takeDirtyRegion();
        dbgPlugins << "Timeout paint dirty region:" << r;
        currentNode()->setDirty(r);
    }

}


void KisToolDyna::initPaint(KoPointerEvent *e)
{
    m_rate = currentPaintOpPreset()->settings()->rate();
    
    QRectF imageSize = QRectF(QPointF(0.0,0.0),currentImage()->size());
    QRectF documentSize = currentImage()->pixelToDocument(imageSize);
    m_surfaceWidth = documentSize.width();
    m_surfaceHeight = documentSize.height();
    setMousePosition(e->point);
    m_mouse.init(m_mousePos.x(), m_mousePos.y());
    
    KisToolFreehand::initPaint(e);

    if (!m_painter) {
        warnKrita << "Didn't create a painter! Something is wrong!";
        return;
    }

    m_painter->setPaintOpPreset(currentPaintOpPreset(), currentImage());
    if (currentPaintOpPreset()->settings()->isAirbrushing()) {
        m_timer->start(m_rate);
    }
}


void KisToolDyna::endPaint()
{
    m_timer->stop();
    KisToolFreehand::endPaint();
}

void KisToolDyna::mousePressEvent(KoPointerEvent *e)
{
    setMousePosition(e->point);
    m_mouse.init(m_mousePos.x(), m_mousePos.y());
    m_odelx = m_mousePos.x();
    m_odely = m_mousePos.y();
    
    KisToolFreehand::mousePressEvent(e);
}


void KisToolDyna::mouseMoveEvent(KoPointerEvent *e)
{
    if(!MOVE_CONDITION(event, KisTool::PAINT_MODE)) {
        KisToolFreehand::mouseMoveEvent(e);
        return;
    }

    setMousePosition(e->point);

    if (applyFilter(m_mousePos.x(), m_mousePos.y())) {
        KoPointerEvent newEvent = filterEvent(e);
        KisToolFreehand::mouseMoveEvent(&newEvent);
    }

    if (m_painter && m_painter->paintOp() && currentPaintOpPreset()->settings()->isAirbrushing()) {
        m_timer->start(m_rate);
    }
}

// dyna algorithm
int KisToolDyna::applyFilter(qreal mx, qreal my)
{
    /* calculate mass and drag */
    qreal mass = flerp(1.0, 160.0, m_curmass);
    qreal drag = flerp(0.00, 0.5, m_curdrag * m_curdrag);

    /* calculate force and acceleration */
    qreal fx = mx - m_mouse.curx;
    qreal fy = my - m_mouse.cury;

    m_mouse.acc = sqrt(fx * fx + fy * fy);

    if (m_mouse.acc < 0.000001) {
        return 0;
    }

    m_mouse.accx = fx / mass;
    m_mouse.accy = fy / mass;

    /* calculate new velocity */
    m_mouse.velx += m_mouse.accx;
    m_mouse.vely += m_mouse.accy;
    m_mouse.vel = sqrt(m_mouse.velx * m_mouse.velx + m_mouse.vely * m_mouse.vely);
    m_mouse.angx = -m_mouse.vely;
    m_mouse.angy = m_mouse.velx;
    if (m_mouse.vel < 0.000001) {
        return 0;
    }

    /* calculate angle of drawing tool */
    m_mouse.angx /= m_mouse.vel;
    m_mouse.angy /= m_mouse.vel;
    if (m_mouse.fixedangle) {
        m_mouse.angx = m_xangle;
        m_mouse.angy = m_yangle;
    }

    m_mouse.velx = m_mouse.velx * (1.0 - drag);
    m_mouse.vely = m_mouse.vely * (1.0 - drag);

    m_mouse.lastx = m_mouse.curx;
    m_mouse.lasty = m_mouse.cury;
    m_mouse.curx = m_mouse.curx + m_mouse.velx;
    m_mouse.cury = m_mouse.cury + m_mouse.vely;

    return 1;
}


KoPointerEvent KisToolDyna::filterEvent(KoPointerEvent* event)
{
    qreal wid = m_widthRange - m_mouse.vel;

    wid = wid * m_width;

    if (wid < 0.00001) {
        wid = 0.00001;
    }

    qreal delx = m_mouse.angx * wid;
    qreal dely = m_mouse.angy * wid;

    qreal px = m_mouse.lastx;
    qreal py = m_mouse.lasty;
    qreal nx = m_mouse.curx;
    qreal ny = m_mouse.cury;

    QPointF prev(px , py);         // previous position
    QPointF now(nx , ny);           // new position

    QPointF prevr(px + m_odelx , py + m_odely);
    QPointF prevl(px - m_odelx , py - m_odely);

    QPointF nowl(nx - delx , ny - dely);
    QPointF nowr(nx + delx , ny + dely);

    // transform coords from float points into image points
    prev.rx() *= m_surfaceWidth;
    prevr.rx() *= m_surfaceWidth;
    prevl.rx() *= m_surfaceWidth;
    now.rx()  *= m_surfaceWidth;
    nowl.rx() *= m_surfaceWidth;
    nowr.rx() *= m_surfaceWidth;

    prev.ry() *= m_surfaceHeight;
    prevr.ry() *= m_surfaceHeight;
    prevl.ry() *= m_surfaceHeight;
    now.ry()  *= m_surfaceHeight;
    nowl.ry() *= m_surfaceHeight;
    nowr.ry() *= m_surfaceHeight;

    qreal m_pressure;
#if 0

    qreal xTilt, yTilt;
    qreal m_rotation;
    qreal m_tangentialPressure;

    // some funny debugging
    dbgPlugins << "m_mouse.vel: " << m_mouse.vel;
    dbgPlugins << "m_mouse.velx: " << m_mouse.velx;
    dbgPlugins << "m_mouse.vely: " << m_mouse.vely;
    dbgPlugins << "m_mouse.accx: " << m_mouse.accx;
    dbgPlugins << "m_mouse.accy: " << m_mouse.accy;


    dbgPlugins << "fixed: " << m_mouse.fixedangle;
    dbgPlugins << "drag: " << m_curdrag;
    dbgPlugins << "mass: " << m_curmass;
    dbgPlugins << "xAngle: " << m_xangle;
    dbgPlugins << "yAngle: " << m_yangle;

#endif

    m_pressure =  m_mouse.vel * 100;
    m_pressure = qBound(0.0,m_pressure, 1.0);
    
    m_odelx = delx;
    m_odely = dely;
    
    // how to change pressure in the KoPointerEvent???
    return KoPointerEvent(event,now);
}


void KisToolDyna::slotSetDrag(double drag)
{
    m_dragDist = drag;
}


void KisToolDyna::slotSetMass(double mass)
{
    m_curmass = mass;
}


void KisToolDyna::slotSetDynaWidth(double width)
{
    m_width = width;
}


void KisToolDyna::slotSetWidthRange(double widthRange)
{
    m_widthRange = widthRange;
}


void KisToolDyna::slotSetXangle(double angle)
{
    m_xangle = angle;
}


void KisToolDyna::slotSetYangle(double angle)
{
    m_yangle = angle;
}


void KisToolDyna::slotSetFixedAngle(bool fixedAngle)
{
    m_mouse.fixedangle = fixedAngle;
}

QWidget * KisToolDyna::createOptionWidget()
{

    QWidget * optionWidget = KisToolFreehand::createOptionWidget();
    optionWidget->setObjectName(toolId() + "option widget");

    QLabel* initWidthLbl = new QLabel(i18n("Initial width:"), optionWidget);
    QLabel* massLbl = new QLabel(i18n("Mass:"), optionWidget);
    QLabel* dragLbl = new QLabel(i18n("Drag:"), optionWidget);
    QLabel* xAngleLbl = new QLabel(i18n("X angle:"), optionWidget);
    QLabel* yAngleLbl = new QLabel(i18n("Y angle:"), optionWidget);
    QLabel* widthRangeLbl = new QLabel(i18n("Width range:"), optionWidget);

    m_chkFixedAngle = new QCheckBox(i18n("Fixed angle:"), optionWidget);
    m_chkFixedAngle->setChecked(false);
    connect(m_chkFixedAngle, SIGNAL(toggled(bool)), this, SLOT(slotSetFixedAngle(bool)));

    m_initWidthSPBox = new QDoubleSpinBox(optionWidget);
    m_initWidthSPBox->setValue(1.5);
    connect(m_initWidthSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetDynaWidth(double)));
    m_massSPBox = new QDoubleSpinBox(optionWidget);
    m_massSPBox->setValue(0.5);
    connect(m_massSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetMass(double)));
    m_dragSPBox = new QDoubleSpinBox(optionWidget);
    m_dragSPBox->setValue(0.15);
    connect(m_dragSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetDrag(double)));
    m_xAngleSPBox = new QDoubleSpinBox(optionWidget);
    m_xAngleSPBox->setValue(0.6);
    connect(m_xAngleSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetXangle(double)));
    m_yAngleSPBox = new QDoubleSpinBox(optionWidget);
    m_yAngleSPBox->setValue(0.2);
    connect(m_yAngleSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetYangle(double)));
    m_widthRangeSPBox = new QDoubleSpinBox(optionWidget);
    m_widthRangeSPBox->setValue(0.05);
    connect(m_widthRangeSPBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetWidthRange(double)));

    m_optionLayout = new QGridLayout(optionWidget);
    Q_CHECK_PTR(m_optionLayout);

    m_optionLayout->setMargin(0);
    m_optionLayout->setSpacing(2);

    KisToolFreehand::addOptionWidgetLayout(m_optionLayout);
    m_optionLayout->addWidget(initWidthLbl, 5, 0);
    m_optionLayout->addWidget(m_initWidthSPBox, 5, 1, 1, 2);
    m_optionLayout->addWidget(massLbl, 6, 0);
    m_optionLayout->addWidget(m_massSPBox, 6, 1, 1, 2);
    m_optionLayout->addWidget(dragLbl, 7, 0);
    m_optionLayout->addWidget(m_dragSPBox, 7, 1, 1, 2);
    m_optionLayout->addWidget(m_chkFixedAngle, 8, 0);
    m_optionLayout->addWidget(xAngleLbl, 9, 0);
    m_optionLayout->addWidget(m_xAngleSPBox, 9, 1, 1, 2);
    m_optionLayout->addWidget(yAngleLbl, 10, 0);
    m_optionLayout->addWidget(m_yAngleSPBox, 10, 1, 1, 2);
    m_optionLayout->addWidget(widthRangeLbl, 11, 0);
    m_optionLayout->addWidget(m_widthRangeSPBox, 11, 1, 1, 2);

    return optionWidget;
}

#include "kis_tool_dyna.moc"
