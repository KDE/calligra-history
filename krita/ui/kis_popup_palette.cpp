/* This file is part of the KDE project
   Copyright 2009 Vera Lukman <shicmap@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#include "kis_popup_palette.h"
#include "kis_paintop_box.h"
#include "ko_favorite_resource_manager.h"
#include <kis_types.h>
#include <QtGui>
#include <QDebug>
#include <QQueue>
#include <QtGui>
#include <math.h>

KisPopupPalette::KisPopupPalette(KoFavoriteResourceManager* manager, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint)
    , m_resourceManager (manager)
    , m_triangleColorSelector (0)
    , m_timer(0)
{
    m_triangleColorSelector  = new KoTriangleColorSelector(this);
    m_triangleColorSelector->move(60, 60);
    m_triangleColorSelector->setVisible(true);

    setAutoFillBackground(true);
    setAttribute(Qt::WA_ContentsPropagated,true);
    //    setAttribute(Qt::WA_TranslucentBackground, true);

    connect(m_triangleColorSelector, SIGNAL(colorChanged(const QColor& )), SLOT(slotChangefGColor(QColor)));
    connect(this, SIGNAL(sigChangeActivePaintop(int)), m_resourceManager, SLOT(slotChangeActivePaintop(int)));
    connect(this, SIGNAL(sigUpdateRecentColor(int)), m_resourceManager, SLOT(slotUpdateRecentColor(int)));
    connect(this, SIGNAL(sigChangefGColor(const KoColor&)), m_resourceManager, SIGNAL(sigSetFGColor(KoColor)));
    connect(m_resourceManager, SIGNAL(sigChangeFGColorSelector(const QColor&)), m_triangleColorSelector, SLOT(setQColor(const QColor&)));

    // This is used to handle a bug:
    // If pop up palette is visible and a new colour is selected, the new colour
    // will be added when the user clicks on the canvas to hide the palette
    // In general, we want to be able to store recent color if the pop up palette
    // is not visible
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(this, SIGNAL(sigTriggerTimer()), this, SLOT(slotTriggerTimer()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotEnableChangeFGColor()));
    connect(this, SIGNAL(sigEnableChangeFGColor(bool)), m_resourceManager, SIGNAL(sigEnableChangeColor(bool)));

    setMouseTracking(true);
    setHoveredBrush(-1);
    setHoveredColor(-1);
    setSelectedBrush(-1);
    setSelectedColor(-1);

    setVisible(true);
    setVisible(false);
}

//setting KisPopupPalette properties
int KisPopupPalette::hoveredBrush() const { return m_hoveredBrush; }
void KisPopupPalette::setHoveredBrush(int x) { m_hoveredBrush = x; }
int KisPopupPalette::selectedBrush() const { return m_selectedBrush; }
void KisPopupPalette::setSelectedBrush(int x) { m_selectedBrush = x; }
int KisPopupPalette::hoveredColor() const { return m_hoveredColor; }
void KisPopupPalette::setHoveredColor(int x) { m_hoveredColor = x; }
int KisPopupPalette::selectedColor() const { return m_selectedColor; }
void KisPopupPalette::setSelectedColor(int x) { m_selectedColor = x; }

void KisPopupPalette::slotChangefGColor(const QColor& newColor)
{
    KoColor color;
    color.fromQColor(newColor);

    emit sigChangefGColor(color);
}

void KisPopupPalette::slotTriggerTimer()
{
    m_timer->start(750);
}

void KisPopupPalette::slotEnableChangeFGColor()
{
    emit sigEnableChangeFGColor(true);
}

void KisPopupPalette::showPopupPalette (const QPoint &p)
{
    if (!isVisible())
    {
        QSize parentSize(parentWidget()->size());
        QPoint pointPalette(p.x() - width()/2, p.y() - height()/2);

        //setting offset point in case the widget is shown outside of canvas region
        int offsetX = 0, offsetY=0;

        if ((offsetX = pointPalette.x() + width() - parentSize.width()) > 0 || (offsetX = pointPalette.x()) < 0)
        {
            pointPalette.setX(pointPalette.x() - offsetX);
        }
        if ((offsetY = pointPalette.y() + height() - parentSize.height()) > 0 || (offsetY = pointPalette.y()) < 0)
        {
            pointPalette.setY(pointPalette.y() - offsetY);
        }
        move(pointPalette);

    }
    showPopupPalette(!isVisible());

}

void KisPopupPalette::showPopupPalette(bool show)
{
    if (show)
        emit sigEnableChangeFGColor(!show);
    else
        emit sigTriggerTimer();

    if(show)
        QApplication::setOverrideCursor(Qt::ArrowCursor);
    else
        QApplication::restoreOverrideCursor();

    setVisible(show);
}

void KisPopupPalette::setVisible(bool b)
{
    QWidget::setVisible(b);
}

QSize KisPopupPalette::sizeHint() const
{
    return QSize(220,220);
}

void KisPopupPalette::resizeEvent(QResizeEvent*)
{
    int side = qMin(width(), height());
    QRegion maskedRegion (width()/2 - side/2, height()/2 - side/2, side, side, QRegion::Ellipse);
    setMask(maskedRegion);
}

void KisPopupPalette::paintEvent(QPaintEvent* e)
{
    float rotationAngle = 0.0;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(e->rect(), palette().brush(QPalette::Window));
    painter.translate(width()/2, height()/2);

    //painting favorite brushes
    QList<QPixmap> pixmaps (m_resourceManager->favoriteBrushPixmaps());

    rotationAngle = 360.0/pixmaps.size();
    //highlight hovered brush
    if (hoveredBrush() > -1)
    {
        painter.rotate((pixmaps.size() - hoveredBrush()) *rotationAngle);
        QPainterPath path(drawDonutPathAngle(brushInnerRadius,brushOuterRadius, pixmaps.size()));
        painter.fillPath(path, palette().color(QPalette::Highlight));
        painter.rotate(hoveredBrush() *rotationAngle);
    }

    //highlight selected brush
    if (selectedBrush() > -1)
    {
        painter.rotate((pixmaps.size() - selectedBrush()) *rotationAngle);
        QPainterPath path(drawDonutPathAngle(brushInnerRadius,brushOuterRadius, pixmaps.size()));
        painter.fillPath(path, palette().color(QPalette::Highlight).darker(130));
        painter.rotate(selectedBrush() *rotationAngle);
    }

    //painting favorite brushes pixmap/icon
    for (int pos = 0; pos < pixmaps.size(); pos++)
    {
        QPixmap pixmap(pixmaps.at(pos));
        QPointF pixmapOffset(pixmap.width()/2, pixmap.height()/2);

        float angle = pos*M_PI*2.0/pixmaps.size();
        QPointF pointTemp(brushRadius*sin(angle),brushRadius*cos(angle));
        painter.drawPixmap(QPoint(pointTemp.x()-pixmapOffset.x(), pointTemp.y()-pixmapOffset.y()), pixmap);
    }

    //painting recent colors : bevel (raised)
    float radius = 51.0;
    //outer radius
    drawArcRisen
            (painter, QColor(Qt::white), radius, 45, cos(M_PI/4)*radius, -1*sin(M_PI/4)*radius, 2*radius+0.5, 2*radius+0.5);
    drawArcRisen
            (painter, QColor(Qt::black), radius, 180+45, -1*cos(M_PI/4)*radius, sin(M_PI/4)*radius, 2*radius+0.5, 2*radius+0.5);

    //inner radius
    radius = 33.5;
    drawArcRisen
            (painter, QColor(Qt::black), radius, 45, cos(M_PI/4)*radius, -1*sin(M_PI/4)*radius, 2*radius+0.5, 2*radius+0.5);
    drawArcRisen
            (painter, QColor(Qt::white), radius, 180+45, -1*cos(M_PI/4)*radius, sin(M_PI/4)*radius, 2*radius, 2*radius);


    //painting recent colors
    painter.setPen(Qt::NoPen);
    rotationAngle = -360.0/m_resourceManager->recentColorsTotal();

    QColor qcolor;
    KoColor kcolor;

    for (int pos = 0; pos < m_resourceManager->recentColorsTotal(); pos++)
    {
        QPainterPath path(drawDonutPathAngle(colorInnerRadius , colorOuterRadius, m_resourceManager->recentColorsTotal()));

        //accessing recent color of index pos
        kcolor = m_resourceManager->recentColorAt(pos);
        kcolor.toQColor(&qcolor);
        painter.fillPath(path, qcolor);

        painter.drawPath(path);
        painter.rotate(rotationAngle);
    }

    painter.setBrush(Qt::transparent);

    if (m_resourceManager->recentColorsTotal() == 0)
    {
        QPainterPath path_ColorDonut(drawDonutPathFull(0,0,colorInnerRadius, colorOuterRadius));
        painter.setPen(QPen(palette().color(QPalette::Window).darker(130), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        painter.drawPath(path_ColorDonut);
    }

    //painting hovered color
    if (hoveredColor() > -1)
    {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));

        if (m_resourceManager->recentColorsTotal() == 1)
        {
            QPainterPath path_ColorDonut(drawDonutPathFull(0,0,colorInnerRadius, colorOuterRadius));
            painter.drawPath(path_ColorDonut);
        }
        else
        {
            painter.rotate((m_resourceManager->recentColorsTotal() + hoveredColor()) *rotationAngle);
            QPainterPath path(drawDonutPathAngle(colorInnerRadius,colorOuterRadius, m_resourceManager->recentColorsTotal()));
            painter.drawPath(path);
            painter.rotate(hoveredColor() *-1 *rotationAngle);
        }
    }

    //painting selected color
    if (selectedColor() > -1)
    {
        painter.setPen(QPen(palette().color(QPalette::Highlight).darker(130), 2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));

        if (m_resourceManager->recentColorsTotal() == 1)
        {
            QPainterPath path_ColorDonut(drawDonutPathFull(0,0,colorInnerRadius, colorOuterRadius));
            painter.drawPath(path_ColorDonut);
        }
        else
        {
            painter.rotate((m_resourceManager->recentColorsTotal() + selectedColor()) *rotationAngle);
            QPainterPath path(drawDonutPathAngle(colorInnerRadius,colorOuterRadius, m_resourceManager->recentColorsTotal()));
            painter.drawPath(path);
            painter.rotate(selectedColor() *-1 *rotationAngle);
        }
    }
}

void KisPopupPalette::drawArcRisen
        (QPainter& painter, QColor color, int radius, int startAngle, float x, float y, float w, float h)
{
    painter.setPen(QPen(color ,2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    QPainterPath path;
    path.moveTo(x,y);
    path.arcTo(-1*radius, -1*radius, w, h, startAngle, 180);
    painter.drawPath(path);
}

QPainterPath KisPopupPalette::drawDonutPathFull(int x, int y, int inner_radius, int outer_radius)
{
    QPainterPath path;
    path.addEllipse(QPointF(x,y), outer_radius, outer_radius);
    path.addEllipse(QPointF(x,y), inner_radius, inner_radius);
    path.setFillRule(Qt::OddEvenFill);

    return path;
}

QPainterPath KisPopupPalette::drawDonutPathAngle(int inner_radius, int outer_radius, int limit)
{
    QPainterPath path;
    path.moveTo(-1*outer_radius * sin(M_PI/limit),
                outer_radius * cos(M_PI/limit));
    path.arcTo((-1*outer_radius), -1*outer_radius, 2*outer_radius,2*outer_radius,-90.0 - 180.0/limit,
               360.0/limit);
    path.arcTo(-1*inner_radius, -1*inner_radius, 2*inner_radius,2*inner_radius,-90.0 + 180.0/limit,
               - 360.0/limit);
    path.closeSubpath();

    return path;
}

void KisPopupPalette::mouseMoveEvent (QMouseEvent* event)
{
    QPointF point = event->posF();
    event->accept();

    QPainterPath pathBrush(drawDonutPathFull(width()/2, height()/2, brushInnerRadius, brushOuterRadius));
    QPainterPath pathColor(drawDonutPathFull(width()/2, height()/2, colorInnerRadius, colorOuterRadius));

    setHoveredBrush(-1);
    setHoveredColor(-1);

    if (pathBrush.contains(point))
    { //in favorite brushes area
        int pos = calculateIndex(point, m_resourceManager->favoriteBrushesTotal());

        if (pos >= 0 && pos < m_resourceManager->favoriteBrushesTotal()
            && isPointInPixmap(point, pos))
            {
            setHoveredBrush(pos);
        }
    }
    else if (pathColor.contains(point))
    {
        int pos = calculateIndex(point, m_resourceManager->recentColorsTotal());

        if (pos >= 0 && pos < m_resourceManager->recentColorsTotal())
        {
            setHoveredColor(pos);
        }
    }
    update();
}

void KisPopupPalette::mousePressEvent(QMouseEvent* event)
{
    QPointF point = event->posF();
    event->accept();

    if (event->button() == Qt::LeftButton)
    {
        QPainterPath pathBrush(drawDonutPathFull(width()/2, height()/2, brushInnerRadius, brushOuterRadius));
        //        QPainterPath pathColor(drawDonutPathFull(width()/2, height()/2, colorInnerRadius, colorOuterRadius));

        if (pathBrush.contains(point))
        { //in favorite brushes area
            int pos = calculateIndex(point, m_resourceManager->favoriteBrushesTotal());
            if (pos >= 0 && pos < m_resourceManager->favoriteBrushesTotal()
                && isPointInPixmap(point, pos))
                {
                setSelectedBrush(pos);
                update();
            }
        }
    }
}

void KisPopupPalette::mouseReleaseEvent ( QMouseEvent * event )
{
    QPointF point = event->posF();
    event->accept();

    if (event->button() == Qt::LeftButton)
    {
        QPainterPath pathBrush(drawDonutPathFull(width()/2, height()/2, brushInnerRadius, brushOuterRadius));
        QPainterPath pathColor(drawDonutPathFull(width()/2, height()/2, colorInnerRadius, colorOuterRadius));
        QPainterPath pathSelCol;
        pathSelCol.addEllipse(QPoint(width()/2, height()/2), 30,30);

        if (pathBrush.contains(point))
        { //in favorite brushes area
            if (hoveredBrush() > -1)
            {
                setSelectedBrush(hoveredBrush());
                emit sigChangeActivePaintop(hoveredBrush());
            }
        }
        else if (pathColor.contains(point))
        {
            int pos = calculateIndex(point, m_resourceManager->recentColorsTotal());

            if (pos >= 0 && pos < m_resourceManager->recentColorsTotal())
            {
                emit sigUpdateRecentColor(pos);
            }
        }
    }
    else if (event->button() == Qt::MidButton)
    {
        showPopupPalette(false);
    }
}

int KisPopupPalette::calculateIndex(QPointF point, int n)
{
    //translate to (0,0)
    point.setX(point.x() - width()/2);
    point.setY(point.y() - height()/2);

    //rotate
    float smallerAngle = M_PI/2 + M_PI/n - atan2 ( point.y(), point.x() );
    float radius = sqrt ( point.x()*point.x() + point.y()*point.y() );
    point.setX( radius * cos(smallerAngle) );
    point.setY( radius * sin(smallerAngle) );

    //calculate brush index
    int pos = floor (acos(point.x()/radius) * n/ (2 * M_PI));
    if (point.y() < 0) pos =  n - pos - 1;

    return pos;
}

bool KisPopupPalette::isPointInPixmap(QPointF& point, int pos)
{
    QPixmap pixmap(m_resourceManager->favoriteBrushPixmap(pos));

    //calculating if the point is inside the pixmap
    float angle = pos*M_PI*2.0/m_resourceManager->favoriteBrushesTotal();
    QPainterPath path;
    path.addRect(brushRadius*sin(angle)-pixmap.width()/2+width()/2,
                 brushRadius*cos(angle)-pixmap.height()/2+height()/2,
                 pixmap.width(), pixmap.height());

    if(path.contains(point) || pixmap.isNull()) return true;
    else return false;
}

KisPopupPalette::~KisPopupPalette()
{
    delete m_triangleColorSelector;
    m_triangleColorSelector = 0;
    m_resourceManager = 0;
}

#include "kis_popup_palette.moc"
