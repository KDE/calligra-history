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

#ifndef _KIS_HLINE_ITERATOR_H_
#define _KIS_HLINE_ITERATOR_H_

#include "kis_base_iterator.h"

#include "kis_iterator_ng.h"

class KisHLineIterator2 : public KisHLineIteratorNG, public KisBaseIterator {
    KisHLineIterator2(const KisHLineIterator2&);
    KisHLineIterator2& operator=(const KisHLineIterator2&);

public:
    struct KisTileInfo {
        KisTileSP tile;
        KisTileSP oldtile;
        quint8* data;
        quint8* oldData;
    };


public:    
    KisHLineIterator2(KisDataManager *dataManager, qint32 x, qint32 y, qint32 w, qint32 offsetX, qint32 offsetY, bool writable);
    ~KisHLineIterator2();
    
    virtual bool nextPixel();
    virtual void nextRow();
    virtual const quint8* oldRawData() const;
    virtual quint8* rawData();
    virtual qint32 nConseqPixels() const;
    virtual bool nextPixels(qint32 n);
    
private:
    qint32 m_x;        // current x position
    qint32 m_y;        // current y position
    qint32 m_row;    // current row in tilemgr
    qint32 m_index;    // current col in tilemgr
    qint32 m_tileWidth;
    quint8 *m_data;
    quint8 *m_dataRight;
    quint8 *m_oldData;
    bool m_havePixels;
    
    qint32 m_right;
    qint32 m_left;
    qint32 m_leftCol;
    qint32 m_rightCol;

    qint32 m_leftInLeftmostTile;
    qint32 m_yInTile;

    QVector<KisTileInfo> m_tilesCache;
    quint32 m_tilesCacheSize;
    
private:

    void switchToTile(qint32 xInTile);
    void fetchTileDataForCache(KisTileInfo& kti, qint32 col, qint32 row);
    void preallocateTiles();
};
#endif
