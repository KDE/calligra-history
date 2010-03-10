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

#ifndef KIS_POPUP_PALETTE_H
#define KIS_POPUP_PALETTE_H

#define brushInnerRadius 80.0
#define brushOuterRadius 100.0
#define colorInnerRadius 55.0
#define colorOuterRadius 75.0
#define brushRadius (brushInnerRadius+brushOuterRadius)/2

#include <kis_types.h>
#include <QtGui/QWidget>
#include <QQueue>
#include <KoColor.h>
#include <KoTriangleColorSelector.h>

class KisFavoriteBrushData;
class KoFavoriteResourceManager;
class QWidget;
class KisTriangleColorSelector;

class KisPopupPalette : public QWidget
{
    Q_OBJECT
    Q_PROPERTY (int hoveredBrush READ hoveredBrush WRITE setHoveredBrush);
    Q_PROPERTY (int selectedBrush READ selectedBrush WRITE setSelectedBrush);
    Q_PROPERTY (int hoveredColor READ hoveredColor WRITE setHoveredColor);
    Q_PROPERTY (int selectedColor READ selectedColor WRITE setSelectedColor);

public:
    KisPopupPalette(KoFavoriteResourceManager* , QWidget *parent=0);
    ~KisPopupPalette();
    QSize sizeHint() const;

    void showPopupPalette (const QPoint&);
    void showPopupPalette (bool b);

    //functions to set up selectedBrush
    void setSelectedBrush( int x );
    int selectedBrush() const;
    //functions to set up selectedColor
    void setSelectedColor( int x );
    int selectedColor() const;

protected:
    void paintEvent (QPaintEvent*);
    void resizeEvent (QResizeEvent*);
    void mouseReleaseEvent (QMouseEvent*);
    void mouseMoveEvent (QMouseEvent*);
    void mousePressEvent (QMouseEvent*);

    //functions to calculate index of favorite brush or recent color in array
    //n is the total number of favorite brushes or recent colors
    int calculateIndex(QPointF, int n);

    //functions to set up hoveredBrush
    void setHoveredBrush( int x );
    int hoveredBrush() const;
    //functions to set up hoveredColor
    void setHoveredColor( int x );
    int hoveredColor() const;


private:
    void setVisible(bool b);

    QPainterPath drawDonutPathFull(int, int, int, int);
    QPainterPath drawDonutPathAngle(int, int, int);
    void drawArcRisen
            (QPainter& painter, QColor color, int radius, int startAngle, float x, float y, float w, float h);
    bool isPointInPixmap(QPointF&, int pos);

private:
    int m_hoveredBrush;
    int m_selectedBrush;
    int m_hoveredColor;
    int m_selectedColor;
    KoFavoriteResourceManager* m_resourceManager;
    KoTriangleColorSelector* m_triangleColorSelector;

    QTimer* m_timer;

signals:
    void sigChangeActivePaintop(int);
    void sigUpdateRecentColor(int);
    void sigChangefGColor(const KoColor&);

    // These are used to handle a bug:
    // If pop up palette is visible and a new colour is selected, the new colour
    // will be added when the user clicks on the canvas to hide the palette
    // In general, we want to be able to store recent color if the pop up palette
    // is not visible
    void sigEnableChangeFGColor(bool);
    void sigTriggerTimer();

private slots:
    void slotChangefGColor(const QColor& newColor);

    void slotTriggerTimer();
    void slotEnableChangeFGColor();
};

#endif // KIS_POPUP_PALETTE_H
