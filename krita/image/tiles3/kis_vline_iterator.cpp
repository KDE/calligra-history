/* 
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
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

#include "kis_vline_iterator.h"

#include <iostream>

KisVLineIterator2::KisVLineIterator2(KisDataManager *dataManager, qint32 x, qint32 y, qint32 h, qint32 offsetX, qint32 offsetY, bool writable) : KisBaseIterator(dataManager, writable, offsetX, offsetY)
{
    x -= offsetX;
    y -= offsetY;
    Q_ASSERT(dataManager != 0);
    m_lineStride = m_pixelSize * KisTileData::WIDTH;
    
    m_x = x;
    m_y = y;

    m_top = y;
    m_bottom = y + h - 1;

    m_havePixels = (h == 0) ? false : true;
    if (m_top > m_bottom) {
        m_havePixels = false;
        return;
    }

    m_topRow = yToRow(m_top);
    m_bottomRow = yToRow(m_bottom);
    
    m_column = xToCol(m_x);
    m_xInTile = calcXInTile(m_x, m_column);

    m_topInTopmostTile = m_top - m_topRow * KisTileData::WIDTH;

    m_tilesCacheSize = m_bottomRow - m_topRow + 1;
    m_tilesCache.resize(m_tilesCacheSize);

    m_tileSize = m_lineStride * KisTileData::HEIGHT;
    
    // let's prealocate first row 
    for (quint32 i = 0; i < m_tilesCacheSize; i++){
        fetchTileDataForCache(m_tilesCache[i], m_column, m_topRow + i);
    }
    m_index = 0;
    switchToTile(m_topInTopmostTile);
}

bool KisVLineIterator2::nextPixel()
{
    // We won't increment m_x here as integer can overflow here
    if (m_y >= m_bottom) {
        //return !m_isDoneFlag;
        return m_havePixels = false;
    } else {
        ++m_y;
        m_data += m_lineStride;
        if (m_data < m_dataBottom)
            m_oldData += m_lineStride;
        else {
            // Switching to the beginning of the next tile
            ++m_index;
            switchToTile(0);
        }
    }

    return m_havePixels;
}


void KisVLineIterator2::nextColumn()
{
    m_y = m_top;
    ++m_x;

    if (++m_xInTile < KisTileData::HEIGHT) {
        /* do nothing, usual case */
    } else {
        ++m_column;
        m_xInTile = 0;
        preallocateTiles();
    }
    m_index = 0;
    switchToTile(m_topInTopmostTile);

    m_havePixels = true;
}


qint32 KisVLineIterator2::nConseqPixels() const
{
    return 1;
}

bool KisVLineIterator2::nextPixels(qint32 n)
{
    Q_ASSERT_X(!(m_y > 0 && (m_y + n) < 0), "vlineIt+=", "Integer overflow");

    qint32 previousRow = yToRow(m_y);
    // We won't increment m_x here first as integer can overflow
    if (m_y >= m_bottom || (m_y += n) > m_bottom) {
        m_havePixels = false;
    } else {
        qint32 row = yToRow(m_y);
        // if we are in the same column in tiles
        if (row == previousRow) {
            m_data += n * m_pixelSize;
        } else {
            qint32 yInTile = calcYInTile(m_y, row);
            m_index += row - previousRow;
            switchToTile(yInTile);
        }
    }
    return m_havePixels;
}



KisVLineIterator2::~KisVLineIterator2()
{
    for (uint i = 0; i < m_tilesCacheSize; i++) {
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
    }
}


quint8 * KisVLineIterator2::rawData()
{
    return m_data;
}


const quint8 * KisVLineIterator2::oldRawData() const
{
    return m_oldData;
}

void KisVLineIterator2::switchToTile(qint32 yInTile)
{
    // The caller must ensure that we are not out of bounds
    Q_ASSERT(m_index < m_tilesCacheSize);
    Q_ASSERT(m_index >= 0);

    m_data = m_tilesCache[m_index].data;
    m_oldData = m_tilesCache[m_index].oldData;
    m_data += m_pixelSize * m_xInTile;
    m_dataBottom = m_data + m_tileSize;
    m_data  += m_pixelSize * yInTile * KisTileData::WIDTH;
}


void KisVLineIterator2::fetchTileDataForCache(KisTileInfo& kti, qint32 col, qint32 row)
{
    kti.tile = m_dataManager->getTile(col, row, m_writable);
    lockTile(kti.tile);
    kti.data = kti.tile->data();

    // set old data
    kti.oldtile = m_dataManager->getOldTile(col, row);
    lockOldTile(kti.oldtile);
    kti.oldData = kti.oldtile->data();
}

void KisVLineIterator2::preallocateTiles()
{
    for (quint32 i = 0; i < m_tilesCacheSize; ++i){
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
        fetchTileDataForCache(m_tilesCache[i], m_column, m_topRow + i );
    }
}
