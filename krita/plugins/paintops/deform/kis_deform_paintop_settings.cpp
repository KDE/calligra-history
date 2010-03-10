/*
 *  Copyright (c) 2008,2009,2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include <kis_deform_paintop_settings.h>
#include <kis_deform_paintop_settings_widget.h>

#include <kis_brush_size_option.h>

bool KisDeformPaintOpSettings::paintIncremental()
{
    return true;
}


QRectF KisDeformPaintOpSettings::paintOutlineRect(const QPointF& pos, KisImageWSP image, OutlineMode _mode) const
{
    if (_mode != CURSOR_IS_OUTLINE) return QRectF();
    qreal width = getDouble(BRUSH_DIAMETER)  * getDouble(BRUSH_SCALE);
    qreal height = getDouble(BRUSH_DIAMETER) * getDouble(BRUSH_ASPECT)  * getDouble(BRUSH_SCALE);
    QRectF brush(0,0,width,height);
    brush.translate(-brush.center());
    QTransform m;
    m.reset();
    m.rotate(getDouble(BRUSH_ROTATION));
    brush = m.mapRect(brush);
    brush.adjust(-1,-1,1,1);
    return image->pixelToDocument(brush).translated(pos);
}

void KisDeformPaintOpSettings::paintOutline(const QPointF& pos, KisImageWSP image, QPainter &painter, OutlineMode _mode) const
{
    if (_mode != CURSOR_IS_OUTLINE) return;
    qreal width = getDouble(BRUSH_DIAMETER)  * getDouble(BRUSH_SCALE);
    qreal height = getDouble(BRUSH_DIAMETER) * getDouble(BRUSH_ASPECT)  * getDouble(BRUSH_SCALE);

    QRectF brush(0,0,width,height);
    brush.translate(-brush.center());
    painter.save();
    painter.translate( pos);
    painter.rotate(getDouble(BRUSH_ROTATION));
    painter.setPen(Qt::black);
    painter.drawEllipse(image->pixelToDocument(brush));
    painter.restore();
}
