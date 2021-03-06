/*
 *  Copyright (c) 2010 Adam Celarek <kdedev at xibo dot at>
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
 */

#include "kis_color_selector_component.h"

#include "kis_color_selector_base.h"

#include "KoColorSpace.h"
#include <QPainter>
#include <QMouseEvent>


KisColorSelectorComponent::KisColorSelectorComponent(KisColorSelector* parent) :
    QObject(parent),
    m_hue(0),
    m_hsvSaturation(1),
    m_value(1),
    m_hslSaturation(1),
    m_lightness(0.5),
    m_parent(parent),
    m_width(0),
    m_height(0),
    m_x(0),
    m_y(0),
    m_dirty(true),
    m_lastColorSpace(0),
    m_lastX(0),
    m_lastY(0)
{
    Q_ASSERT(parent);
}

void KisColorSelectorComponent::setGeometry(int x, int y, int width, int height)
{
    m_x=x;
    m_y=y;
    m_width=width;
    m_height=height;
    m_dirty=true;
}

void KisColorSelectorComponent::paintEvent(QPainter* painter)
{
    painter->save();
    painter->translate(m_x, m_y);
    paint(painter);
    painter->restore();

    m_dirty=false;
    m_lastColorSpace=colorSpace();
}

void KisColorSelectorComponent::mouseEvent(int x, int y)
{
    int newX=qBound(0, (x-m_x), width());
    int newY=qBound(0, (y-m_y), height());

    selectColor(newX, newY);
    m_lastX=newX;
    m_lastY=newY;
}

const KoColorSpace* KisColorSelectorComponent::colorSpace() const
{
    const KoColorSpace* cs = m_parent->colorSpace();
    Q_ASSERT(cs);
    return cs;
}

bool KisColorSelectorComponent::isDirty() const
{
    return m_dirty || m_lastColorSpace!=colorSpace();
}

bool KisColorSelectorComponent::isComponent(int x, int y) const
{
    if(x>=0 && y>=0 && x<=width() && y<=height())
        return true;
    else
        return false;
}

QColor KisColorSelectorComponent::currentColor()
{
    return selectColor(m_lastX, m_lastY);
}

void KisColorSelectorComponent::setParam(qreal hue, qreal hsvSaturation, qreal value, qreal hslSaturation, qreal lightness)
{
    if(qFuzzyCompare(m_hue, hue) &&
       qFuzzyCompare(m_hsvSaturation, hsvSaturation) &&
       qFuzzyCompare(m_value, value) &&
       qFuzzyCompare(m_hslSaturation, hslSaturation) &&
       qFuzzyCompare(m_lightness, lightness))
        return;

    if(hue>=0. && hue<=1.)
        m_hue=hue;

    if(hsvSaturation>=0. && hsvSaturation<=1.) {
        m_hsvSaturation=hsvSaturation;
        m_hslSaturation=-1;
    }

    if(value>=0. && value<=1.) {
        m_value=value;
        m_lightness=-1;
    }

    if(hslSaturation>=0. && hslSaturation<=1.) {
        m_hslSaturation=hslSaturation;
        m_hsvSaturation=-1;
    }

    if(lightness>=0. && lightness<=1.) {
        m_lightness=lightness;
        m_value=-1;
    }

    m_dirty=true;
    emit update();
}

int KisColorSelectorComponent::width() const
{
    return m_width;
}

int KisColorSelectorComponent::height() const
{
    return m_height;
}

void KisColorSelectorComponent::setConfiguration(Parameter param, Type type)
{
    m_parameter = param;
    m_type = type;
}
