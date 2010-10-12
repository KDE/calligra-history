/*
 * Copyright 2010 Inge Wallin <inge@lysator.liu.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Bitmap.h"

#include <QDataStream>
//#include <QColor>
#include <QImage>
//#include <QRect> // also provides QSize
//#include <QString>

#include <KDebug>

#include "EmfEnums.h"


namespace Libemf
{

static void soakBytes( QDataStream &stream, int numBytes )
{
    quint8 scratch;
    for ( int i = 0; i < numBytes; ++i ) {
        stream >> scratch;
    }
}


Bitmap::Bitmap( QDataStream &stream, 
                quint32 recordSize,  // total size of the EMF record
                quint32 usedBytes,   // used bytes of the EMF record before the bitmap part
                quint32 offBmiSrc, // offset to start of bitmapheader
                quint32 cbBmiSrc,  // size of bitmap header
                quint32 offBitsSrc,// offset to source bitmap
                quint32 cbBitsSrc) // size of source bitmap
    : m_hasImage(false)
    , m_header(0)
      //, m_image(0)
    , m_imageIsValid(false)
{
    // If necessary read away garbage before the bitmap header.
    if (offBmiSrc > usedBytes) {
        //kDebug(31000) << "soaking " << offBmiSrc - usedBytes << "bytes before the header";
        soakBytes(stream, offBmiSrc - usedBytes);
        usedBytes = offBmiSrc;
    }

    // Create the header
    m_header = new BitmapHeader(stream, cbBmiSrc);
    usedBytes += cbBmiSrc;

    // If necessary read away garbage between the bitmap header and the picture.
    if (offBitsSrc > usedBytes) {
        //kDebug(31000) << "soaking " << offBmiSrc - usedBytes << "bytes between the header and the image";
        soakBytes(stream, offBitsSrc - usedBytes);
        usedBytes = offBitsSrc;
    }

    // Read the image data
    if (cbBitsSrc > 0) {
        //kDebug(31000) << "reading bitmap (" << cbBitsSrc << "bytes)";
        m_imageData.resize( cbBitsSrc );
        stream.readRawData( m_imageData.data(), cbBitsSrc );
        m_hasImage = true;

        usedBytes += cbBitsSrc;
    }

    // If necessary, read away garbage after the image.
    if (recordSize > usedBytes) {
        //kDebug(31000) << "soaking " << recordSize - usedBytes << "bytes after the image";
        soakBytes(stream, recordSize - usedBytes);
        usedBytes = recordSize;
    }
}

Bitmap::~Bitmap()
{
    delete m_header;
    //delete m_image;
}


#if 1
QImage Bitmap::image()
{
    if (!m_hasImage) {
        return QImage();
    }

    if (m_imageIsValid) {
        return m_image;
    }

    QImage::Format format = QImage::Format_Invalid;

    // Start by determining which QImage format we are going to use.
    if (m_header->bitCount() == BI_BITCOUNT_1) {
        format = QImage::Format_Mono;
    } else if ( m_header->bitCount() == BI_BITCOUNT_4 ) {
        if ( m_header->compression() == BI_RGB ) {
            format = QImage::Format_RGB555;
        } else {
            //kDebug(33100) << "Unexpected compression format for BI_BITCOUNT_4:"
            //              << m_header->compression();
            //Q_ASSERT( 0 );
            return QImage();
        }
    } else if ( m_header->bitCount() == BI_BITCOUNT_5 ) {
        format = QImage::Format_RGB888;
    } else {
        //kDebug(31000) << "Unexpected format:" << m_header->bitCount();
        //Q_ASSERT(0);
        return QImage();
    }

    // According to MS-WMF 2.2.2.3, the sign of the height decides if
    // this is a compressed bitmap or not.
    if (m_header->height() > 0) {
        // This bitmap is a top-down bitmap without compression.
        m_image = QImage( (const uchar*)m_imageData.constData(),
                          m_header->width(), m_header->height(), format );

        // The WMF images are in the BGR color order.
        if (format == QImage::Format_RGB888)
            m_image = m_image.rgbSwapped();

        // We have to mirror this bitmap in the X axis since WMF images are stored bottom-up.
        m_image = m_image.mirrored(false, true);
    } else {
        // This bitmap is a bottom-up bitmap which uses compression.
        switch (m_header->compression()) {
        case BI_RGB:
            m_image = QImage( (const uchar*)m_imageData.constData(),
                              m_header->width(), -m_header->height(), format );
            // The WMF images are in the BGR color order.
            m_image = m_image.rgbSwapped();
            break;

        // These compressions are not yet supported, so return an empty image.
        case BI_RLE8:
        case BI_RLE4:
        case BI_BITFIELDS:
        case BI_JPEG:
        case BI_PNG:
        case BI_CMYK:
        case BI_CMYKRLE8:
        case BI_CMYKRLE4:
        default:
            m_image = QImage(m_header->width(), m_header->height(), format);
            break;
        }
    }

    m_imageIsValid = true;
    return m_image;
}
#else
QImage *Bitmap::image()
{
    if (!m_hasImage) {
        return 0;
    }

    if (m_image) {
        return m_image;
    }

    QImage::Format format = QImage::Format_Invalid;
    if ( m_header->bitCount() == BI_BITCOUNT_4 ) {
        if ( m_header->compression() == 0x00 ) {
            format = QImage::Format_RGB555;
        } else {
            //kDebug(33100) << "Unexpected compression format for BI_BITCOUNT_4:"
            //              << m_header->compression();
            //Q_ASSERT( 0 );
            return 0;
        }
    } else if ( m_header->bitCount() == BI_BITCOUNT_5 ) {
        format = QImage::Format_RGB888;
    } else {
        kDebug(33100) << "Unexpected format:" << m_header->bitCount();
        //Q_ASSERT( 0 );
        return 0;
    }
    m_image = new QImage( (const uchar*)m_imageData.constData(),
                          m_header->width(), m_header->height(), format );

    return m_image;
}
#endif


} // namespace Libemf
