/*
 * This file is part of the Krita
 *
 * Copyright (c) 2004 Casper Boemann <cbr@boemann.dk>
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

#include "kis_tilediterator.h"
#include "kis_debug.h"

KisTiledVLineIterator::KisTiledVLineIterator(KisTiledDataManager *dataManager,
        qint32 x,  qint32 y,
        qint32 h, bool writable) :
        KisTiledIterator(dataManager), m_tilesCacheSize(0)
{
    m_lineStride = m_pixelSize * KisTileData::WIDTH;
    m_writable = writable;

    m_x = x;
    m_y = y;

    m_top = y;
    m_bottom = y + h - 1;

    m_isDoneFlag = !h;
    if (m_top > m_bottom) {
        m_isDoneFlag = true;
        return;
    }

    m_topRow = yToRow(m_top);
    m_bottomRow = yToRow(m_bottom);

    m_col = xToCol(m_x);
    m_xInTile = calcXInTile(m_x, m_col);

    qint32 topInTopmostTile = calcTopInTile(m_topRow);

    m_tilesCacheSize = m_bottomRow - m_topRow + 1;

    m_tilesCache.resize(m_tilesCacheSize);

    // let's prealocate first row
    for (quint32 i = 0; i < m_tilesCacheSize; i++){
        m_tilesCache[i] = fetchTileDataForCache( m_col, m_topRow + i);
    }

    switchToTile(m_topRow, topInTopmostTile);
}

KisTiledVLineIterator::KisTiledVLineIterator(const KisTiledVLineIterator& rhs)
        : KisTiledIterator(rhs)
{
    if (this != &rhs) {
        m_top = rhs.m_top;
        m_bottom = rhs.m_bottom;
        m_topRow = rhs.m_topRow;
        m_bottomRow = rhs.m_bottomRow;
        m_xInTile = rhs.m_xInTile;
        m_yInTile = rhs.m_yInTile;
        m_topInTile = rhs.m_topInTile;
        m_bottomInTile = rhs.m_bottomInTile;
        m_isDoneFlag = rhs.m_isDoneFlag;
        m_lineStride = m_pixelSize * KisTileData::WIDTH;
    }
}

KisTiledVLineIterator& KisTiledVLineIterator::operator=(const KisTiledVLineIterator & rhs)
{
    if (this != &rhs) {
        KisTiledIterator::operator=(rhs);

        m_top = rhs.m_top;
        m_bottom = rhs.m_bottom;
        m_topRow = rhs.m_topRow;
        m_bottomRow = rhs.m_bottomRow;
        m_xInTile = rhs.m_xInTile;
        m_yInTile = rhs.m_yInTile;
        m_topInTile = rhs.m_topInTile;
        m_bottomInTile = rhs.m_bottomInTile;
        m_isDoneFlag = rhs.m_isDoneFlag;
        m_lineStride = m_pixelSize * KisTileData::WIDTH;
    }
    return *this;
}

KisTiledVLineIterator::~KisTiledVLineIterator()
{
    for (uint i = 0; i < m_tilesCacheSize; i++) {
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
    }
}

void KisTiledVLineIterator::switchToTile(qint32 row, qint32 yInTile)
{
    // The caller must ensure that we are not out of bounds
    Q_ASSERT(row <= m_bottomRow);
    Q_ASSERT(row >= m_topRow);

    m_topInTile = calcTopInTile(row);
    m_bottomInTile = calcBottomInTile(row);

    m_row = row;
    m_yInTile = yInTile;
    m_offset = calcOffset(m_xInTile, m_yInTile);

    fetchTileData(m_col, row);
}

KisTiledVLineIterator & KisTiledVLineIterator::operator ++ ()
{
    // We won't increment m_y here as integer can overflow here
    if (m_y >= m_bottom) {
        m_isDoneFlag = true;
    } else {
        m_y++;
        if (++m_yInTile <= m_bottomInTile)
            m_offset += m_lineStride;
        else
            // Switching to the beginning of the next tile
            switchToTile(m_row + 1, 0);
    }

    return *this;
}

void KisTiledVLineIterator::nextCol()
{
    qint32 topInTopmostTile = calcTopInTile(m_topRow);

    m_y = m_top;
    m_x++;

    if (++m_xInTile < KisTileData::WIDTH) {
        /* do nothing, usual case */
    } else {
        m_col++;
        m_xInTile = 0;
        preallocateTiles(m_col);
    }

    switchToTile(m_topRow, topInTopmostTile);

    m_isDoneFlag = false;
}

void KisTiledVLineIterator::fetchTileData(qint32 col, qint32 row){

    Q_UNUSED(col);
    // check if we have the cached column and row
    int index = row - m_topRow;

    // setup correct data
    m_data = m_tilesCache[index].data;
    m_oldData = m_tilesCache[index].oldData;
}

KisTiledVLineIterator::KisTileInfo KisTiledVLineIterator::fetchTileDataForCache(qint32 col, qint32 row)
{
    KisTileInfo kti;
    kti.tile = m_dataManager->getTile(col, row, m_writable);
    lockTile(kti.tile);
    kti.data = kti.tile->data();

    // set old data
    kti.oldtile = m_dataManager->getOldTile(col, row);
    lockOldTile(kti.oldtile);
    kti.oldData = kti.oldtile->data();
    return kti;
}

void KisTiledVLineIterator::preallocateTiles(qint32 col)
{
    for (uint i = 0; i < m_tilesCacheSize; i++){
        unlockTile(m_tilesCache[i].tile);
        unlockTile(m_tilesCache[i].oldtile);
        m_tilesCache[i] = fetchTileDataForCache(col , m_topRow + i);
    }
}
