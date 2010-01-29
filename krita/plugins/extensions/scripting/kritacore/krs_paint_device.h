/*
 *  Copyright (c) 2005,2007 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KROSS_KRITACOREKRSPAINTDEVICE_H
#define KROSS_KRITACOREKRSPAINTDEVICE_H

#include "krs_const_paint_device.h"

#include <kis_types.h>
#include <kis_paint_layer.h>
#include "krosskritacore_export.h"

class KisDoc2;
class KisTransaction;

namespace Scripting
{

class Image;

/**
 * A PaintDevice is a layer within a \a Image where you are able
 * to perform paint-operations on.
 */
class KROSSKRITACORE_EXPORT PaintDevice : public ConstPaintDevice
{
    Q_OBJECT
public:
    explicit PaintDevice(KisPaintDeviceSP layer, KisDoc2* doc = 0);
    virtual ~PaintDevice();

public slots:

    /**
     * Convert the image to a colorspace.
     * This function takes one argument :
     *  - the name of the destination colorspace
     * This function returns true if convert to the
     * colorspace was successfully else (e.g. if the
     * colorspace is not available, please check your
     * installation in that case) false is returned.
     *
     * For example (in Ruby) :
     * @code
     * # set the colorspace to "CMYK"
     * image.convertToColorSpace("CMYK")
     * # following line will print "CMYK" now.
     * image.colorSpaceId()
     * @endcode
     */
    bool convertToColorSpace(const QString& model, const QString& depth);

    /**
     * Create an iterator over a layer, it will iterate on a rectangle area.
     * This function takes four arguments :
     *  - x
     *  - y
     *  - width of the rectangle
     *  - height of the rectangle
     */
    QObject* createRectIterator(uint x, uint y, uint width, uint height);

    /**
     * Create an iterator over a layer, it will iterate on a row.
     * This function takes three arguments :
     *  - x start in the row
     *  - y vertical position of the row
     *  - width of the row
     */
    QObject* createHLineIterator(uint x, uint y, uint width);

    /**
     * Create an iterator over a layer, it will iterate on a column.
     * This function takes three arguments :
     *  - x horizontal position of the column
     *  - y start in the column
     *  - height of the column
     */
    QObject* createVLineIterator(uint x, uint y, uint height);

    /**
     * This function create a \a Painter which will allow you to some
     * painting on the layer.
     */
    QObject* createPainter();

    /**
     * Uses this function to create a new undo entry. The \p name
     * is the displayed undo-name. You should always close the
     * paint-operation with \a endPainting() .
     *
     * For example (in Ruby) :
     * @code
     * require "Krita"
     * layer = Krita.image().activePaintDevice()
     * layer.beginPainting("invert")
     * iterator = layer.createRectIterator(0, 0, layer.width(), layer.height())
     * while (not iterator.isDone())
     *     iterator.invertColor()
     *     iterator.next()
     * end
     * layer.endPainting()
     * @endcode
     */
    void beginPainting(const QString& name);

    /**
     * Uses this function to close the current undo entry and add it to
     * the history. This function closes the with \a beginPainting()
     * started painting-operation.
     */
    void endPainting();

    /**
     * Untransform a fast \a Wavelet into this layer.
     * It takes one argument :
     *  - a wavelet object
     * It returns true on success else (e.g. cause no valid \a Wavelet
     * object was passed as argument) false is returned.
     *
     * For example (in Ruby) :
     * @code
     * wavelet = layer.fastWaveletTransformation()
     * layer.fastWaveletUntransformation(wavelet)
     * @endcode
     */
    bool fastWaveletUntransformation(QObject* wavelet);

#if 0
//Disabled yet cause it's not wanted to expose the Krita internals.

    /**
     * Returns or sets the raw-bytes the layer has.
     *
     * Please note, that it is NOT recommed to use that
     * functionality since they bypass Krita's undo/redo
     * mechanism as well as the integrated swapping-technology
     * and may consume a lot of RAM cause the whole image
     * is readed into it (what provides us the possibility
     * to work direct on the mem-data from within
     * scripting-languages).
     *
     * The returned array of bytes has n items where n is
     * height * width * number of bytes per pixel. Each pixel
     * is represented by >= 1 bytes depending on the used
     * colorspace (e.g. for RGBA we have 4 bytes per pixel).
     *
     * For example (in Python) :
     * @code
     * import Krita, struct, array
     * layer = Krita.image().activePaintDevice()
     * ba = layer.bytes()
     * a = array.array('B', [ (255 - struct.unpack("B",ba[i])[0]) for i in range(len(ba)) ] )
     * layer.setBytes( a.tostring() )
     * @endcode
     */
    QByteArray bytes();
    bool setBytes(const QByteArray& bytearray);
#endif

public:
    inline KisPaintDeviceSP paintDevice() {
        return m_device;
    }
    inline const KisPaintDeviceSP paintDevice() const {
        return m_device;
    }
private:
    KisPaintDeviceSP m_device;
    KisTransaction* m_cmd;
};

}

#endif
