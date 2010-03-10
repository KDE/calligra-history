/* 
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "kis_hline_iterator.h"


KisHLineIterator2::KisHLineIterator2(KisDataManager *dataManager, qint32 x, qint32 y, qint32 w, qint32 offsetX, qint32 offsetY, bool writable) : KisBaseIterator(dataManager, writable, offsetX, offsetY)
{
    x -= offsetX;
    y -= offsetY;
    Q_ASSERT(dataManager != 0);
    
    m_x = x;
    m_y = y;

    m_left = x;
    m_right = x + w - 1;

    m_havePixels = (w == 0) ? false : true;
    if (m_left > m_right) {
        m_havePixels = false;
        return;
    }

    m_leftCol = xToCol(m_left);
    m_rightCol = xToCol(m_right);
    
    m_row = yToRow(m_y);
    m_yInTile = calcYInTile(m_y, m_row);

    m_leftInLeftmostTile = m_left - m_leftCol * KisTileData::WIDTH;

    m_tilesCacheSize = m_rightCol - m_leftCol + 1;
    m_tilesCache.resize(m_tilesCacheSize);

    m_tileWidth = m_pixelSize * KisTileData::HEIGHT;
    
    // let's prealocate first row 
    for (quint32 i = 0; i < m_tilesCacheSize; i++){
        fetchTileDataForCache(m_tilesCache[i], m_leftCol + i, m_row);
    }
    m_index = 0;
    switchToTile(m_leftInLeftmostTile);
}

bool KisHLineIterator2::nextPixel()
{
    // We won't increment m_x here as integer can overflow here
    if (m_x >= m_right) {
        //return !m_isDoneFlag;
        return m_havePixels = false;
    } else {
        ++m_x;
        m_data += m_pixelSize;
        if (m_data < m_dataRight)
            m_oldData += m_pixelSize;
        else {
            // Switching to the beginning of the next tile
            ++m_index;
            switchToTile(0);
        }
    }

    return m_havePixels;
}


void KisHLineIterator2::nextRow()
{
    m_x = m_left;
    ++m_y;

    if (++m_yInTile < KisTileData::HEIGHT) {
        /* do nothing, usual case */
    } else {
        ++m_row;
        m_yInTile = 0;
        preallocateTiles();
    }
    m_index = 0;
    switchToTile(m_leftInLeftmostTile);

    m_havePixels = true;
}


qint32 KisHLineIterator2::nConseqPixels() const
{
    return (m_dataRight - m_data) / m_pixelSize;
}



bool KisHLineIterator2::nextPixels(qint32 n)
{
    Q_ASSERT_X(!(m_x > 0 && (m_x + n) < 0), "hlineIt+=", "Integer overflow");

    qint32 previousCol = xToCol(m_x);
    // We won't increment m_x here first as integer can overflow
    if (m_x >= m_right || (m_x += n) > m_right) {
        m_havePixels = false;
    } else {
        qint32 col = xToCol(m_x);
        // if we are in the same column in tiles
        if (col == previousCol) {
            m_data += n * m_pixelSize;
        } else {
            qint32 xInTile = calcXInTile(m_x, col);
            m_index += col - previousCol;
            switchToTile(xInTile);
        }
    }
    return m_havePixels;
}



KisHLineIterator2::~KisHLineIterator2()
{
    for (uint i = 0; i < m_tilesCacheSize; i++) {
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
    }
}


quint8 * KisHLineIterator2::rawData()
{
    return m_data;
}


const quint8 * KisHLineIterator2::oldRawData() const
{
    return m_oldData;
}

void KisHLineIterator2::switchToTile(qint32 xInTile)
{
    // The caller must ensure that we are not out of bounds
    Q_ASSERT(m_index < m_tilesCacheSize);
    Q_ASSERT(m_index >= 0);

    m_data = m_tilesCache[m_index].data;
    m_oldData = m_tilesCache[m_index].oldData;

    m_data += m_pixelSize * (m_yInTile * KisTileData::WIDTH);
    m_dataRight = m_data + m_tileWidth;
    m_data  += m_pixelSize * xInTile;
}


void KisHLineIterator2::fetchTileDataForCache(KisTileInfo& kti, qint32 col, qint32 row)
{
    kti.tile = m_dataManager->getTile(col, row, m_writable);
    lockTile(kti.tile);
    kti.data = kti.tile->data();

    // set old data
    kti.oldtile = m_dataManager->getOldTile(col, row);
    lockOldTile(kti.oldtile);
    kti.oldData = kti.oldtile->data();
}

void KisHLineIterator2::preallocateTiles()
{
    for (quint32 i = 0; i < m_tilesCacheSize; ++i){
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
        fetchTileDataForCache(m_tilesCache[i], m_leftCol + i, m_row);
    }
}
