/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2007,2010 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  Outline algorithm based of the limn of fontutils
 *  Copyright (c) 1992 Karl Berry <karl@cs.umb.edu>
 *  Copyright (c) 1992 Kathryn Hargreaves <letters@cs.umb.edu>
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

#include "kis_outline_generator.h"
#include "kdebug.h"

#include <KoColorSpace.h>

KisOutlineGenerator::KisOutlineGenerator(const KoColorSpace* cs, quint8 defaultOpacity)
    : m_cs(cs), m_defaultOpacity(defaultOpacity)
{
}

QVector<QPolygon> KisOutlineGenerator::outline(quint8* buffer, qint32 xOffset, qint32 yOffset, qint32 width, qint32 height)
{
  
    quint8* marks = new quint8[width*height];
    for (int i = 0; i < width*height; i++) {
        marks[i] = 0;
    }
    QVector<QPolygon> paths;

    int nodes = 0;
    for (qint32 y = 0; y < height; y++) {
        for (qint32 x = 0; x < width; x++) {
            
            if (m_cs->opacityU8(buffer + (y*width+x)*m_cs->pixelSize()) == m_defaultOpacity)
                continue;

            EdgeType startEdge = TopEdge;

            EdgeType edge = startEdge;
            while (edge != NoEdge && (marks[y*width+x] & (1 << edge) || !isOutlineEdge(edge, x, y, buffer, width, height))) {
                edge = nextEdge(edge);
                if (edge == startEdge)
                    edge = NoEdge;
            }

            if (edge != NoEdge) {
                QPolygon path;
                path << QPoint(x + xOffset, y + yOffset);

// XXX: Unused? (BSAR)
//                bool clockwise = edge == BottomEdge;

                qint32 row = y, col = x;
                EdgeType currentEdge = edge;
                EdgeType lastEdge = currentEdge;
                do {
                    //While following a strait line no points nead to be added
                    if (lastEdge != currentEdge) {
                        appendCoordinate(&path, col + xOffset, row + yOffset, currentEdge);
                        nodes++;
                        lastEdge = currentEdge;
                    }

                    marks[row*width+col] |= 1 << currentEdge;
                    nextOutlineEdge(&currentEdge, &row, &col, buffer, width, height);
                } while (row != y || col != x || currentEdge != edge);

                paths.push_back(path);
            }
        }
    }
    delete[] marks;

    return paths;
}

bool KisOutlineGenerator::isOutlineEdge(EdgeType edge, qint32 x, qint32 y, quint8* buffer, qint32 bufWidth, qint32 bufHeight)
{
    if (m_cs->opacityU8(buffer + (y*bufWidth+x)*m_cs->pixelSize()) == m_defaultOpacity)
        return false;

    switch (edge) {
    case LeftEdge:
        return x == 0 || m_cs->opacityU8(buffer + (y*bufWidth+(x - 1))*m_cs->pixelSize()) == m_defaultOpacity;
    case TopEdge:
        return y == 0 || m_cs->opacityU8(buffer + ((y - 1)*bufWidth+x)*m_cs->pixelSize()) == m_defaultOpacity;
    case RightEdge:
        return x == bufWidth - 1 || m_cs->opacityU8(buffer + (y*bufWidth+(x + 1))*m_cs->pixelSize()) == m_defaultOpacity;
    case BottomEdge:
        return y == bufHeight - 1 || m_cs->opacityU8(buffer + ((y + 1)*bufWidth+x)*m_cs->pixelSize()) == m_defaultOpacity;
    case NoEdge:
        return false;
    }
    return false;
}

#define TRY_PIXEL(deltaRow, deltaCol, test_edge)                                                \
    {                                                                                               \
        int test_row = *row + deltaRow;                                                             \
        int test_col = *col + deltaCol;                                                             \
        if ( (0 <= (test_row) && (test_row) < height && 0 <= (test_col) && (test_col) < width) &&   \
                isOutlineEdge (test_edge, test_col, test_row, buffer, width, height))                  \
        {                                                                                           \
            *row = test_row;                                                                        \
            *col = test_col;                                                                        \
            *edge = test_edge;                                                                      \
            break;                                                                                  \
        }                                                                                       \
    }

void KisOutlineGenerator::nextOutlineEdge(EdgeType *edge, qint32 *row, qint32 *col, quint8* buffer, qint32 width, qint32 height)
{
    int original_row = *row;
    int original_col = *col;

    switch (*edge) {
    case RightEdge:
        TRY_PIXEL(-1, 0, RightEdge);
        TRY_PIXEL(-1, 1, BottomEdge);
        break;

    case TopEdge:
        TRY_PIXEL(0, -1, TopEdge);
        TRY_PIXEL(-1, -1, RightEdge);
        break;

    case LeftEdge:
        TRY_PIXEL(1, 0, LeftEdge);
        TRY_PIXEL(1, -1, TopEdge);
        break;

    case BottomEdge:
        TRY_PIXEL(0, 1, BottomEdge);
        TRY_PIXEL(1, 1, LeftEdge);
        break;

    default:
        break;

    }

    if (*row == original_row && *col == original_col)
        *edge = nextEdge(*edge);
}

void KisOutlineGenerator::appendCoordinate(QPolygon * path, int x, int y, EdgeType edge)
{
    switch (edge) {
    case TopEdge:
        x++;
        break;
    case RightEdge:
        x++;
        y++;
        break;
    case BottomEdge:
        y++;
        break;
    case LeftEdge:
    case NoEdge:
        break;

    }
    *path << QPoint(x, y);
}