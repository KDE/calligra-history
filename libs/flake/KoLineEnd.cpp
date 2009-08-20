/* This file is part of the KDE project
   Made by Tomislav Lukman (tomislav.lukman@ck.tel.hr)
   Copyright (C) 2002 Tomislav Lukman <tomislav.lukman@ck.t-com.hr>
   Copyright (C) 2002-2003 Rob Buis <buis@kde.org>
   Copyright (C) 2005-2006 Tim Beaulen <tbscope@gmail.com>
   Copyright (C) 2005-2007 Thomas Zander <zander@kde.org>
   Copyright (C) 2005-2006 Inge Wallin <inge@lysator.liu.se>
   Copyright (C) 2005-2008 Jan Hambrecht <jaham@gmx.net>
   Copyright (C) 2006 Casper Boemann <cbr@boemann.dk>
   Copyright (C) 2006 Peter Simonsson <psn@linux.se>
   Copyright (C) 2006 Laurent Montel <montel@kde.org>
   Copyright (C) 2007 Thorsten Zachmann <t.zachmann@zagge.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KoLineEnd.h" 
#include "KoPathPoint.h"
#include "KoPathPointData.h"

#include <QPixmap>
#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>
#include <QRectF>
#include <QBrush>
#include <QMatrix>

//qRegisterMetaType<KoLineEnd>("KoLineEnd");

KoLineEnd::KoLineEnd(const QString &name, const QString &path, const QString &view)
{
  m_name = name;
  m_path = path;
  m_view = view;
}

KoLineEnd::KoLineEnd()
{
}

KoLineEnd::KoLineEnd(const KoLineEnd &lineEnd)
{
    m_name = lineEnd.m_name;
    m_path = lineEnd.m_path;
    m_view = lineEnd.m_view;
    m_svg = lineEnd.m_svg;
}

KoLineEnd::~KoLineEnd()
{
}

QByteArray KoLineEnd::generateSVG(const QSize &size, const QString &comment)
{
    QByteArray width(QString::number(size.width()).toUtf8());
    QByteArray height(QString::number(size.height()).toUtf8());
    QByteArray str("<?xml version=\"1.0\" standalone=\"no\"?>");
    str.append("<!--"+comment.toUtf8()+"-->");
    str.append("<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">");
    str.append("<svg width=\""+width+"px\" height=\""+height+"px\" viewBox=\""+m_view.toUtf8()+"\" preserveAspectRatio=\"xMidYMid meet\" xmlns=\"http://www.w3.org/2000/svg\">");
    str.append("<path width=\""+width+"px\" height=\""+height+"px\" fill=\"black\"  d=\""+m_path.toUtf8()+"\" transform=\""+m_transform.toUtf8()+"\" />");
    str.append("</svg>");
    m_svg = str;
    return str;
}

QByteArray KoLineEnd::getSVG() const
{
    return m_svg;
}

QIcon KoLineEnd::drawIcon(const QSize &size, const int proportion)
{
    QSvgRenderer endLineRenderer;
    QPixmap endLinePixmap(size);
    endLinePixmap.fill(QColor(Qt::transparent));
    QPainter endLinePainter(&endLinePixmap);
    // Convert path to SVG
    //endLineRenderer.load(generateSVG(QSize(size.width()-6, size.height()-6)));
    endLineRenderer.load(generateSVG(QSize(30, 30)));
    endLineRenderer.render(&endLinePainter, QRectF(proportion, proportion, size.width()-(proportion*2), size.height()-(proportion*2)));

    // return QIcon
    return QIcon (endLinePixmap);
}

QString KoLineEnd::name() const
{
    return m_name;
}

QString KoLineEnd::path() const
{
    return m_path;
}

QString KoLineEnd::viewBox() const
{
    return m_view;
}

QString KoLineEnd::transform() const
{
    return m_transform;
}

void KoLineEnd::setTransform(const QString &transform)
{
    m_transform = transform;
}

void KoLineEnd::paint(QPainter &painter, const QRectF &rect, const float angle)
{
    m_transform.clear();
    QStringList viewBoxValues = m_view.split(" ");
    if(viewBoxValues.count() == 4){
        kDebug() << m_view;
        kDebug() << viewBoxValues;
        kDebug() << "nb : " << viewBoxValues.count();
        int centerX = viewBoxValues.at(2).toInt()/2;
        int centerY = viewBoxValues.at(3).toInt()/2;
        m_transform.append("translate("+QString::number(centerX)+", "+QString::number(centerY)+"), rotate("+QString::number((int)angle-90)+", "+QString::number(centerX)+", "+QString::number(centerY)+")");
    }

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(QBrush(Qt::red));
    QSvgRenderer endLineRenderer;
kDebug() << generateSVG(QSize(20, 20));
    endLineRenderer.load(generateSVG(QSize(20, 20)));
    endLineRenderer.render(&painter, rect);

}

QPointF KoLineEnd::computeAngle(const KoPathPoint* point, const QMatrix& matrix)
{
    KoPathShape * path = point->parent();
    KoPathPointIndex index = path->pathPointIndex(point);

    /// check if it is a start point
    if (point->properties() & KoPathPoint::StartSubpath) {
        if (point->activeControlPoint2()) {
            return matrix.map(point->point()) - matrix.map(point->controlPoint2());
        } else {
            KoPathPoint * next = path->pointByIndex(KoPathPointIndex(index.first, index.second + 1));
            if (! next)
                return QPointF();
            else if (next->activeControlPoint1())
                return matrix.map(point->point()) - matrix.map(next->controlPoint1());
            else
                return matrix.map(point->point()) - matrix.map(next->point());
        }  
    } else {
        if (point->activeControlPoint1()) {
            return matrix.map(point->point()) - matrix.map(point->controlPoint1());
        } else {
            KoPathPoint * prev = path->pointByIndex(KoPathPointIndex(index.first, index.second - 1));
            if (! prev)
                return QPointF();
            else if (prev->activeControlPoint2())
                return matrix.map(point->point()) - matrix.map(prev->controlPoint2());
            else
                return matrix.map(point->point()) - matrix.map(prev->point());
        }  
    }
}
#include "KoLineEnd.moc"
