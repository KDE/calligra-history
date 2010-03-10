/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_DATAMANAGER_H_
#define KIS_DATAMANAGER_H_

#include <qglobal.h>

class QRect;
class KoStore;


// Change the following line to switch (at compiletime) to different datamanager
#include <config-tiles.h> // For the next define
#include KIS_TILED_DATA_MANAGER_HEADER
#include KIS_MEMENTO_HEADER

#define ACTUAL_DATAMGR KisTiledDataManager

/**
 * KisDataManager defines the interface that modules responsible for
 * storing and retrieving data must inmplement. Data modules, like
 * the tile manager, are responsible for:
 *
 * * Storing undo/redo data
 * * Offering ordered and unordered iterators over rects of pixels
 * * (eventually) efficiently loading and saving data in a format
 * that may allow deferred loading.
 *
 * A datamanager knows nothing about the type of pixel data except
 * how many quint8's a single pixel takes.
 */
class KisDataManager : public ACTUAL_DATAMGR
{

public:

    /**
     * Create a new datamanger where every pixel will take pixelSize bytes and will be initialized
     * by default with defPixel. The value of defPixel is copied, the caller still owns the pointer.
     *
     * Note that if pixelSize > size of the defPixel array, we will happily read beyond the
     * defPixel array.
     */
    KisDataManager(quint32 pixelSize, const quint8 *defPixel) : ACTUAL_DATAMGR(pixelSize, defPixel), m_exactBoundsValid(false) {}
    KisDataManager(const KisDataManager& dm) : ACTUAL_DATAMGR(dm), m_exactBoundsValid(false) { }

public:
    /**
     * Sets the default pixel. New data will be initialised with this pixel. The pixel is copied: the
     * caller still owns the pointer.
     */
    inline void setDefaultPixel(const quint8 *defPixel) {
        return ACTUAL_DATAMGR::setDefaultPixel(defPixel);
    }

    /**
     * Get a pointer to the default pixel.
     */
    inline const quint8 *defaultPixel() const {
        return ACTUAL_DATAMGR::defaultPixel();
    }

    /**
     * Reguests a memento from the data manager. There is only one memento active
     * at any given moment for a given paint device and all and any
     * write actions on the datamanger builds undo data into this memento
     * necessary to rollback the transaction.
     */
    inline KisMementoSP getMemento() {
        return ACTUAL_DATAMGR::getMemento();
    }

    /**
     * Restores the image data to the state at the time of the getMemento() call.
     *
     * Note that rollback should be performed with mementos in the reverse order of
     * their creation, as mementos only store incremental changes
     */
    inline void rollback(KisMementoSP memento) {
        ACTUAL_DATAMGR::rollback(memento);
    }

    /**
     * Restores the image data to the state at the time of the rollback call of the memento.
     *
     * Note that rollforward must only be called when an rollback have previously been performed, and
     * no intermittent actions have been performed (though it's ok to rollback other mementos and
     * roll them forward again)
     */
    inline void rollforward(KisMementoSP memento) {
        ACTUAL_DATAMGR::rollforward(memento);
    }

    /**
     * @returns true if there is a memento active. This means that
     * iterators can rely on the oldData() function.
     */
    inline bool hasCurrentMemento() const {
        return ACTUAL_DATAMGR::hasCurrentMemento();
    }

public:

    /**
     * Reads and writes the tiles from/onto a KoStore (which is simply a file within a zip file)
     *
     */
    inline bool write(KoStore *store) {
        return ACTUAL_DATAMGR::write(store);
    }

    inline bool read(KoStore *store) {
        return ACTUAL_DATAMGR::read(store);
    }


public:

    /**
     * Returns the number of bytes a pixel takes
     */
    inline quint32 pixelSize() {
        return ACTUAL_DATAMGR::pixelSize();
    }

    /**
     * Return the extent of the data in x,y,w,h.
     */
    inline void extent(qint32 &x, qint32 &y, qint32 &w, qint32 &h) const {
        return ACTUAL_DATAMGR::extent(x, y, w, h);
    }

    QRect extent() const {
        return ACTUAL_DATAMGR::extent();
    }

public:

    /**
      * Crop or extend the data to x, y, w, h.
      */
    inline void setExtent(qint32 x, qint32 y, qint32 w, qint32 h) {
        return ACTUAL_DATAMGR::setExtent(x, y, w, h);
    }

    inline void setExtent(const QRect & rect) {
        setExtent(rect.x(), rect.y(), rect.width(), rect.height());
    }

public:

    /**
     * Clear the specified rect to the specified value.
     */
    inline void clear(qint32 x, qint32 y,
                      qint32 w, qint32 h,
                      quint8 def) {
        ACTUAL_DATAMGR::clear(x, y, w, h, def);
    }

    /**
     * Clear the specified rect to the specified pixel value.
     */
    inline void clear(qint32 x, qint32 y,
                      qint32 w, qint32 h,
                      const quint8 * def) {
        ACTUAL_DATAMGR::clear(x, y, w, h, def);
    }


    /**
     * Clear all back to default values.
     */
    inline void clear() {
        ACTUAL_DATAMGR::clear();
    }

public:

    /**
     * Write the specified data to x, y. There is no checking on pixelSize!
     */
    inline void setPixel(qint32 x, qint32 y, const quint8 * data) {
        ACTUAL_DATAMGR::setPixel(x, y, data);
    }


    /**
     * Copy the bytes in the specified rect to a chunk of memory.
     * The pixelSize in bytes is w * h * pixelSize
     */
    inline void readBytes(quint8 * data,
                          qint32 x, qint32 y,
                          qint32 w, qint32 h) const {
        ACTUAL_DATAMGR::readBytes(data, x, y, w, h);
    }

    /**
     * Copy the bytes to the specified rect. w * h * pixelSize bytes
     * will be read, whether the caller prepared them or not.
     */
    inline void writeBytes(const quint8 * data,
                           qint32 x, qint32 y,
                           qint32 w, qint32 h) {
        ACTUAL_DATAMGR::writeBytes(data, x, y, w, h);
    }


    /**
     * Copy the bytes in the paint device into a vector of arrays of bytes,
     * where the number of arrays is the number of channels in the
     * paint device. If the specified area is larger than the paint
     * device's extent, the default pixel will be read.
     *
     * @param channelsize a vector with for every channel its size in bytes
     */
    QVector<quint8*> readPlanarBytes(QVector<qint32> channelsizes, qint32 x, qint32 y, qint32 w, qint32 h) {
        return ACTUAL_DATAMGR::readPlanarBytes(channelsizes, x, y, w, h);
    }

    /**
     * Write the data in the separate arrays to the channels. If there
     * are less vectors than channels, the remaining channels will not
     * be copied. If any of the arrays points to 0, the channel in
     * that location will not be touched. If the specified area is
     * larger than the paint device, the paint device will be
     * extended. There are no guards: if the area covers more pixels
     * than there are bytes in the arrays, krita will happily fill
     * your paint device with areas of memory you never wanted to be
     * read. Krita may also crash.
     *
     * @param planes a vector with a byte array for every plane
     * @param channelsize a vector with for every channel its size in
     * bytes
     *
     * XXX: what about undo?
     */
    void writePlanarBytes(QVector<quint8*> planes, QVector<qint32> channelsizes,  qint32 x, qint32 y, qint32 w, qint32 h) {
        ACTUAL_DATAMGR::writePlanarBytes(planes, channelsizes, x, y, w, h);
    }


    /**
     * Get the number of contiguous columns starting at x, valid for all values
     * of y between minY and maxY.
     */
    inline qint32 numContiguousColumns(qint32 x, qint32 minY, qint32 maxY) const {
        return ACTUAL_DATAMGR::numContiguousColumns(x, minY, maxY);
    }


    /**
     * Get the number of contiguous rows starting at y, valid for all
     * values of x between minX and maxX.
     */
    inline qint32 numContiguousRows(qint32 y, qint32 minX, qint32 maxX) const {
        return ACTUAL_DATAMGR::numContiguousRows(y, minX, maxX);
    }


    /**
     * Get the row stride at pixel (x, y). This is the number of bytes
     * to add to a pointer to pixel (x, y) to access (x, y + 1).
     */
    inline qint32 rowStride(qint32 x, qint32 y) const {
        return ACTUAL_DATAMGR::rowStride(x, y);
    }
public:
    void invalidateExactBounds() {
        m_exactBoundsValid = false;
    }
    void setExactBounds(const QRect& rect) const {
        m_exactBoundsValid = true;
        m_exactBounds = rect;
    }
    bool validExactBounds() const {
        return m_exactBoundsValid;
    }
    QRect exactBounds() const {
        Q_ASSERT(m_exactBoundsValid);
        return m_exactBounds;
    }
protected:
    friend class KisRectIterator;
    friend class KisHLineIterator;
    friend class KisVLineIterator;
private:
    mutable bool m_exactBoundsValid;
    mutable QRect m_exactBounds;
};


#endif // KIS_DATAMANAGER_H_

