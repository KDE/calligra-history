/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
#include <QRect>
#include <QMatrix>
#include <QImage>
#include <QApplication>
#include <QList>
#include <QTime>
#include <QTimer>
#include <QUndoCommand>
#include <QHash>

#include <QThread>
#include <threadweaver/ThreadWeaver.h>
#include <klocale.h>
#include <kconfiggroup.h>

#include "KoColorProfile.h"
#include "KoColor.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoIntegerMaths.h"
#include <KoStore.h>

#include "kis_global.h"
#include "kis_paint_engine.h"
#include "kis_types.h"
#include "kis_painter.h"
#include "kis_fill_painter.h"
#include "kis_filter.h"
#include "kis_iterator.h"
#include "kis_iterators_pixel.h"
#include "kis_iterator_pixel_trait.h"
#include "kis_random_accessor.h"
#include "kis_random_sub_accessor.h"
#include "kis_selection.h"
#include "kis_node.h"
#include "kis_painterly_overlay.h"
#include "kis_paint_device.h"
#include "kis_datamanager.h"

#include "kis_undo_adapter.h"
#include "kis_selection_component.h"
#include "kis_pixel_selection.h"
#include "kis_paint_device_jobs.h"

//#define CACHE_EXACT_BOUNDS

class KisPaintDevice::Private {

public:

    KisNodeWSP parent;
    qint32 x;
    qint32 y;
    KoColorSpace * colorSpace;
    qint32 pixelSize;
    qint32 nChannels;
    QPaintEngine * paintEngine;

#ifdef CACHE_EXACT_BOUNDS
    QRect exactBounds;
#endif

    KisPainterlyOverlaySP painterlyOverlay;

};

KisPaintDevice::KisPaintDevice(const KoColorSpace * colorSpace, const QString& name)
    : QObject(0)
    , m_d( new Private() )
{
    setObjectName(name);

    Q_ASSERT(colorSpace);

    m_d->x = 0;
    m_d->y = 0;

    m_d->pixelSize = colorSpace->pixelSize();
    m_d->nChannels = colorSpace->channelCount();

    quint8 *defaultPixel = new quint8 [ m_d->pixelSize ];
    memset( defaultPixel, 0, m_d->pixelSize );

    m_datamanager = new KisDataManager(m_d->pixelSize, defaultPixel);
    delete [] defaultPixel;

    Q_CHECK_PTR(m_datamanager);

    m_d->parent = 0;

    m_d->colorSpace = colorSpace->clone();

    m_d->paintEngine = new KisPaintEngine();

}

KisPaintDevice::KisPaintDevice(KisNodeWSP parent, const KoColorSpace * colorSpace, const QString& name)
    : QObject(0)
    , m_d( new Private() )
{
    setObjectName(name);
    Q_ASSERT( colorSpace );

    m_d->x = 0;
    m_d->y = 0;

    m_d->parent = parent;

    m_d->colorSpace = colorSpace->clone();

    m_d->pixelSize = m_d->colorSpace->pixelSize();
    m_d->nChannels = m_d->colorSpace->channelCount();

    quint8 *defaultPixel = new quint8[ m_d->pixelSize ];
    m_d->colorSpace->fromQColor(Qt::black, OPACITY_TRANSPARENT, defaultPixel);

    m_datamanager = new KisDataManager(m_d->pixelSize, defaultPixel);
    delete [] defaultPixel;
    Q_CHECK_PTR(m_datamanager);
    m_d->paintEngine = new KisPaintEngine();

}


KisPaintDevice::KisPaintDevice(const KisPaintDevice& rhs)
    : QObject()
    , KisShared(rhs)
    , QPaintDevice()
    , m_d( new Private() )
{
    if (this != &rhs) {
        m_d->parent = 0;
        if (rhs.m_datamanager) {
            m_datamanager = new KisDataManager(*rhs.m_datamanager);
            Q_CHECK_PTR(m_datamanager);
        }
        else {
            kWarning() << "rhs " << rhs.objectName() << " has no datamanager\n";
        }
        m_d->x = rhs.m_d->x;
        m_d->y = rhs.m_d->y;
        m_d->colorSpace = rhs.m_d->colorSpace->clone();

        m_d->pixelSize = rhs.m_d->pixelSize;

        m_d->nChannels = rhs.m_d->nChannels;
        m_d->paintEngine = rhs.m_d->paintEngine;
    }
}

KisPaintDevice::~KisPaintDevice()
{
    delete m_d->colorSpace;
    delete m_d->paintEngine;
    delete m_d;
}

QPaintEngine * KisPaintDevice::paintEngine () const
{
    return m_d->paintEngine;
}

int KisPaintDevice::metric( PaintDeviceMetric metric ) const
{
    QRect rc = exactBounds();
    int depth = colorSpace()->pixelSize() - 1;

    switch (metric) {
    case PdmWidth:
        return rc.width();
        break;

    case PdmHeight:
        return rc.height();
        break;

    case PdmWidthMM:
        return qRound( rc.width() * ( 72 / 25.4 ) );
        break;

    case PdmHeightMM:
        return qRound( rc.height() * ( 72 / 25.4 ) );
        break;

    case PdmNumColors:
        return depth * depth;
        break;

    case PdmDepth:
        return qRound( colorSpace()->pixelSize() * 8 ); // in bits
        break;

    case PdmDpiX:
        return 72;
        break;

    case PdmDpiY:
        return 72;
        break;

    case PdmPhysicalDpiX:
        return 72;
        break;

    case PdmPhysicalDpiY:
        return 72;
        break;
    default:
        qWarning("QImage::metric(): Unhandled metric type %d", metric);
        break;
    }
    return 0;
}

void KisPaintDevice::setDirty(const QRect & rc)
{

#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds |= rc;
#endif
    if ( m_d->parent.isValid() )
        m_d->parent->setDirty( rc );
}

void KisPaintDevice::setDirty( const QRegion & region )
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds |= region.boundingRect();
#endif
    if ( m_d->parent.isValid() )
        m_d->parent->setDirty( region );
}

void KisPaintDevice::setDirty()
{
    if ( m_d->parent.isValid() )
        m_d->parent->setDirty();
}

void KisPaintDevice::move(qint32 x, qint32 y)
{
    QRect dirtyRegion = extent();

    m_d->x = x;
    m_d->y = y;

    dirtyRegion |= extent();

    setDirty(dirtyRegion);

}

void KisPaintDevice::move(const QPoint& pt)
{
    move(pt.x(), pt.y());
}

QRect KisPaintDevice::extent() const
{
    qint32 x, y, w, h;
    m_datamanager->extent(x, y, w, h);
    x += m_d->x;
    y += m_d->y;

    return QRect(x, y, w, h);
}

QRect KisPaintDevice::exactBounds() const
{
#ifdef CACHE_EXACT_BOUNDS
    return m_d->exactBounds;
#else
    // Solution n°2
    qint32  x, y, w, h, boundX2, boundY2, boundW2, boundH2;
    QRect rc = extent();
    x = boundX2 = rc.x();
    y = boundY2 = rc.y();
    w = boundW2 = rc.width();
    h = boundH2 = rc.height();

    const quint8* defaultPixel = m_datamanager->defaultPixel();
    bool found = false;
    {
        KisHLineConstIterator it = const_cast<KisPaintDevice *>(this)->createHLineConstIterator(x, y, w);
        for (qint32 y2 = y; y2 < y + h ; ++y2) {
            while (!it.isDone() && found == false) {
                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                    boundY2 = y2;
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
            it.nextRow();
        }
    }

    found = false;

    for (qint32 y2 = y + h; y2 >= y ; --y2) {
        KisHLineConstIterator it = const_cast<KisPaintDevice *>(this)->createHLineConstIterator(x, y2, w);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                boundH2 = y2 - boundY2 + 1;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }
    found = false;

    {
        KisVLineConstIterator it = const_cast<KisPaintDevice *>(this)->createVLineConstIterator(x, boundY2, boundH2);
        for (qint32 x2 = x; x2 < x + w ; ++x2) {
            while (!it.isDone() && found == false) {
                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {
                    boundX2 = x2;
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
            it.nextCol();
        }
    }

    found = false;

    // Look for right edge )
    {

        for (qint32 x2 = x + w; x2 >= x; --x2) {

            KisVLineConstIterator it = const_cast<KisPaintDevice *>(this)->createVLineConstIterator(x2, boundY2, boundH2);
            while (!it.isDone() && found == false) {

                if (memcmp(it.rawData(), defaultPixel, m_d->pixelSize) != 0) {

                    boundW2 = x2 - boundX2 + 1;
                    found = true;
                    break;
                }
                ++it;
            }
            if (found) break;
        }
    }
    return QRect(boundX2, boundY2, boundW2, boundH2);
#endif
}


void KisPaintDevice::crop(qint32 x, qint32 y, qint32 w, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = QRect( x, y, w, h );
#endif
    m_datamanager->setExtent(x - m_d->x, y - m_d->y, w, h);
}


void KisPaintDevice::crop(const QRect & r)
{
    QRect rc( r );
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = rc;
#endif
    rc.translate(-m_d->x, -m_d->y);
    m_datamanager->setExtent(rc);
}

void KisPaintDevice::clear()
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = QRect( 0, 0, 0, 0 );
#endif
    m_datamanager->clear();
}

void KisPaintDevice::fill(qint32 x, qint32 y, qint32 w, qint32 h, const quint8 *fillPixel)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, h );
#endif
    m_datamanager->clear(x, y, w, h, fillPixel);
}


void KisPaintDevice::clear( const QRect & rc )
{
#ifdef CACHE_EXACT_BOUNDS
    if ( rc.contains( m_d->exactBounds) )
         m_d->exactBounds = QRect();
#endif
    m_datamanager->clear( rc.x(), rc.y(), rc.width(), rc.height(), m_datamanager->defaultPixel() );
}

void KisPaintDevice::mirrorX( KisSelection * selection )
{
    KisPaintDeviceSP dst = this;
    if ( !m_datamanager->hasCurrentMemento() )
        dst = new KisPaintDevice( colorSpace() );

    QRect r;
    if ( selection ) {
        r = selection->selectedExactRect();
    }
    else {
        r = exactBounds();
    }
    {
        KisHLineConstIteratorPixel srcIt = createHLineConstIterator(r.x(), r.top(), r.width(), selection);
        KisHLineIteratorPixel dstIt = dst->createHLineIterator(r.x(), r.top(), r.width());

        for (qint32 y = r.top(); y <= r.bottom(); ++y) {

            dstIt += r.width() - 1;

            while (!srcIt.isDone()) {
                if (srcIt.isSelected()) {
                    if ( m_datamanager->hasCurrentMemento() )
                        memcpy( dstIt.rawData(), srcIt.oldRawData(), m_d->pixelSize );
                    else
                        memcpy( dstIt.rawData(), srcIt.rawData(), m_d->pixelSize );
                }
                ++srcIt;
                --dstIt;

            }
            srcIt.nextRow();
            dstIt.nextRow();
        }
    }
    if ( !m_datamanager->hasCurrentMemento() )
        m_datamanager = dst->dataManager();

    setDirty(r);
}

void KisPaintDevice::mirrorY( KisSelection * selection )
{
    KisPaintDeviceSP dst = this;
    if ( !m_datamanager->hasCurrentMemento() )
        dst = new KisPaintDevice( colorSpace() );

    /* Read a line from bottom to top and and from top to bottom and write their values to each other */
    QRect r;
    if ( selection ) {
        r = selection->selectedExactRect();
    }
    else {
        r = exactBounds();
    }

    qint32 y1, y2;
    for (y1 = r.top(), y2 = r.bottom(); y1 <= r.bottom(); ++y1, --y2) {

        KisHLineIteratorPixel itTop = dst->createHLineIterator(r.x(), y1, r.width(), selection);
        KisHLineConstIteratorPixel itBottom = createHLineConstIterator(r.x(), y2, r.width());

        while (!itTop.isDone() && !itBottom.isDone()) {
            if (itBottom.isSelected()) {
                if ( m_datamanager->hasCurrentMemento() )
                    memcpy(itTop.rawData(), itBottom.oldRawData(), m_d->pixelSize);
                else
                    memcpy(itTop.rawData(), itBottom.rawData(), m_d->pixelSize);
            }
            ++itBottom;
            ++itTop;
        }
    }

    if ( !m_datamanager->hasCurrentMemento() )
        m_datamanager = dst->dataManager();

    setDirty(r);
}

bool KisPaintDevice::write(KoStore *store)
{
    bool retval = m_datamanager->write(store);
    emit ioProgress(100);

        return retval;
}

bool KisPaintDevice::read(KoStore *store)
{
    bool retval = m_datamanager->read(store);
    emit ioProgress(100);

        return retval;
}

void KisPaintDevice::convertTo(const KoColorSpace * dstColorSpace, KoColorConversionTransformation::Intent renderingIntent)
{
    if (   colorSpace()->id() == dstColorSpace->id()
        && colorSpace()->profile() == dstColorSpace->profile() )
    {
        return;
    }

    KisPaintDevice dst(dstColorSpace);
    dst.setX(x());
    dst.setY(y());


    qint32 x, y, w, h;
    QRect rc = exactBounds();
    x = rc.x();
    y = rc.y();
    w = rc.width();
    h = rc.height();


    // For now, default multithreading to false: our backend cannot handle this type
    // of multithreading yet.

    KConfigGroup cfg = KGlobal::config()->group("");
    bool threadColorSpaceConversion = cfg.readEntry( "thread_colorspace_conversion", false );
    ThreadWeaver::Weaver * weaver = 0;

    if ( threadColorSpaceConversion ) {
        weaver = new Weaver();
        weaver->setMaximumNumberOfThreads( cfg.readEntry("threadsafe",  QThread::idealThreadCount() ) );
    }

    // Experiment with these versions to see which is faster,  but it
    // seems that lcms dwarfs everything.
#if 0

    // Because this is anew paint device, its tiles should always be
    // aligned with ours.
    {
        KisRectConstIteratorPixel srcIter = createRectConstIterator( x, y, w, h );
        KisRectIteratorPixel dstIter = dst.createRectIterator( x, y, w, h );
        while ( !srcIter.isDone() && !dstIter.isDone() ) {

            qint32 numContiguousSrcPixels = srcIter.nConseqPixels();
            qint32 numContiguousDstPixels = dstIter.nConseqPixels();

            // Because the tiles should be aligned
            Q_ASSERT( numContiguousDstPixels == numContiguousSrcPixels );

            if ( threadColorSpaceConversion ) {
                ConversionJob * job = new ConversionJob(srcIter.rawData(), dstIter.rawData(), colorSpace(), dstColorSpace, numContiguousSrcPixels, renderingIntent, 0);
                weaver->enqueue( job );
            }
            else
                m_d->colorSpace->convertPixelsTo(srcIter.rawData(), dstIter.rawData(), dstColorSpace, numContiguousSrcPixels, renderingIntent);

            srcIter+=numContiguousSrcPixels;
            dstIter+=numContiguousDstPixels;
        }
    }
#else
    for (qint32 row = y; row < y + h; ++row) {

        qint32 column = x;
        qint32 columnsRemaining = w;

        while (columnsRemaining > 0) {

            qint32 numContiguousDstColumns = dst.numContiguousColumns(column, row, row);
            qint32 numContiguousSrcColumns = numContiguousColumns(column, row, row);

            qint32 columns = qMin(numContiguousDstColumns, numContiguousSrcColumns);
            columns = qMin(columns, columnsRemaining);

            KisHLineConstIteratorPixel srcIt = createHLineConstIterator(column, row, columns);
            KisHLineIteratorPixel dstIt = dst.createHLineIterator(column, row, columns);

            const quint8 *srcData = srcIt.rawData();
            quint8 *dstData = dstIt.rawData();

            if ( threadColorSpaceConversion ) {
                ConversionJob * job = new ConversionJob(srcData, dstData, colorSpace(), dstColorSpace, columns, renderingIntent, 0);
                weaver->enqueue( job );
            }
            else
                m_d->colorSpace->convertPixelsTo(srcData, dstData, dstColorSpace, columns, renderingIntent);

            column += columns;
            columnsRemaining -= columns;
        }
    }
#endif

    if ( threadColorSpaceConversion ) {
        weaver->finish();
    }

    KisDataManagerSP oldData = m_datamanager;
    const KoColorSpace *oldColorSpace = m_d->colorSpace;

    setDataManager(dst.m_datamanager, dstColorSpace);

// XXX: make undoable
//     if (undoAdapter() && undoAdapter()->undo()) {
//         undoAdapter()->addCommand(new KisConvertLayerTypeCmd(KisPaintDeviceSP(this), oldData, oldColorSpace, m_datamanager, m_d->colorSpace));
//     }

    delete oldColorSpace;
    delete weaver;
    // XXX: emit colorSpaceChanged(dstColorSpace);
}

void KisPaintDevice::setProfile(const KoColorProfile * profile)
{
    if (profile == 0) return;

    const KoColorSpace * dstSpace =
            KoColorSpaceRegistry::instance()->colorSpace( colorSpace()->id(), profile);
    if (dstSpace)
        m_d->colorSpace = dstSpace->clone();
    emit profileChanged(profile);
}

void KisPaintDevice::setDataManager(KisDataManagerSP data, const KoColorSpace * colorSpace)
{
    m_datamanager = data;
    m_d->colorSpace = colorSpace->clone();
    m_d->pixelSize = m_d->colorSpace->pixelSize();
    m_d->nChannels = m_d->colorSpace->channelCount();

    setDirty(extent());
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds = extent();
#endif

}

void KisPaintDevice::convertFromQImage(const QImage& image, const QString &srcProfileName,
                                       qint32 offsetX, qint32 offsetY)
{
    Q_UNUSED( srcProfileName );

    QImage img = image;

    if (img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_ARGB32);
    }
#ifdef __GNUC__
#warning "KisPaintDevice::convertFromQImage. re-enable use of srcProfileName"
#endif
#if 0

    // XXX: Apply import profile
    if (colorSpace() == KoColorSpaceRegistry::instance() ->colorSpace("RGBA",0)) {
        writeBytes(img.bits(), 0, 0, img.width(), img.height());
    }
    else {
#endif
#if 0
        quint8 * dstData = new quint8[img.width() * img.height() * pixelSize()];
        KoColorSpaceRegistry::instance()
                ->colorSpace("RGBA", srcProfileName)->
                        convertPixelsTo(img.bits(), dstData, colorSpace(), img.width() * img.height());
        writeBytes(dstData, offsetX, offsetY, img.width(), img.height());
#endif
        KisPainter p( this );
        p.bitBlt(offsetX, offsetY, colorSpace()->compositeOp( COMPOSITE_OVER ),
                 &image, OPACITY_OPAQUE,
                 0, 0, image.width(), image.height());
        p.end();
#ifdef CACHE_EXACT_BOUNDS
        m_d->exactBounds = image.rect();
#endif
//    }
}

QImage KisPaintDevice::convertToQImage(const KoColorProfile *  dstProfile, float exposure)
{
    qint32 x1;
    qint32 y1;
    qint32 w;
    qint32 h;

    x1 = - x();
    y1 = - y();

    QRect rc = exactBounds();
    x1 = rc.x();
    y1 = rc.y();
    w = rc.width();
    h = rc.height();

    return convertToQImage(dstProfile, x1, y1, w, h, exposure);
}

QImage KisPaintDevice::convertToQImage(const KoColorProfile *  dstProfile, qint32 x1, qint32 y1, qint32 w, qint32 h, float exposure)
{
    if (w < 0)
        return QImage();

    if (h < 0)
        return QImage();

    quint8 * data;
    try {
        data = new quint8 [w * h * m_d->pixelSize];
    } catch(std::bad_alloc)
    {
        kWarning() << "KisPaintDevice::convertToQImage std::bad_alloc for " << w << " * " << h << " * " << m_d->pixelSize;
        //delete[] data; // data is not allocated, so don't free it
        return QImage();
    }
    Q_CHECK_PTR(data);

    // XXX: Is this really faster than converting line by line and building the QImage directly?
    //      This copies potentially a lot of data.
    readBytes(data, x1, y1, w, h);
    QImage image = colorSpace()->convertToQImage(data, w, h, dstProfile, KoColorConversionTransformation::IntentPerceptual);
    delete[] data;

    return image;
}

KisPaintDeviceSP KisPaintDevice::createThumbnailDevice(qint32 w, qint32 h) const
{
    KisPaintDeviceSP thumbnail = KisPaintDeviceSP(new KisPaintDevice(colorSpace(), "thumbnail"));
    thumbnail->clear();

    int srcw, srch;
    const QRect e = extent();
    srcw = e.width();
    srch = e.height();

    if (w > srcw)
    {
        w = srcw;
        h = qint32(double(srcw) / w * h);
    }
    if (h > srch)
    {
        h = srch;
        w = qint32(double(srch) / h * w);
    }

    if (srcw > srch)
        h = qint32(double(srch) / srcw * w);
    else if (srch > srcw)
        w = qint32(double(srcw) / srch * h);

    KisRandomConstAccessorPixel iter = createRandomConstAccessor(0,0);
    for (qint32 y = 0; y < h; ++y) {
        qint32 iY = (y * srch ) / h;
        for (qint32 x = 0; x < w; ++x) {
            qint32 iX = (x * srcw ) / w;
            iter.moveTo(iX, iY);
            thumbnail->setPixel(x, y, KoColor(iter.rawData(), m_d->colorSpace));
        }
    }

    return thumbnail;

}

KisPainterlyOverlaySP KisPaintDevice::painterlyOverlay()
{
    return m_d->painterlyOverlay;
}

void KisPaintDevice::createPainterlyOverlay()
{
    if ( !m_d->painterlyOverlay ) {
        m_d->painterlyOverlay = new KisPainterlyOverlay();
        emit ( painterlyOverlayCreated() );
    }

}

void KisPaintDevice::removePainterlyOverlay()
{
    m_d->painterlyOverlay = 0;
}


QImage KisPaintDevice::createThumbnail(qint32 w, qint32 h)
{
    KisPaintDeviceSP dev = createThumbnailDevice( w, h );
    return dev->convertToQImage( KoColorSpaceRegistry::instance()->rgb8()->profile() );
}

KisRectIteratorPixel KisPaintDevice::createRectIterator(qint32 left, qint32 top, qint32 w, qint32 h, const KisSelection * selection)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( left, top, w, h );
#endif
    KisDataManager* selectionDm = 0;

    if ( selection )
        selectionDm = selection->dataManager().data();

    return KisRectIteratorPixel(m_datamanager.data(), selectionDm, left, top, w, h, m_d->x, m_d->y);
}

KisRectConstIteratorPixel KisPaintDevice::createRectConstIterator(qint32 left, qint32 top, qint32 w, qint32 h, const KisSelection * selection) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = const_cast< KisDataManager*>(selection->dataManager().data());

    return KisRectConstIteratorPixel(dm, selectionDm, left, top, w, h, m_d->x, m_d->y);
}

KisHLineIteratorPixel  KisPaintDevice::createHLineIterator(qint32 x, qint32 y, qint32 w, const KisSelection * selection)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, 1 );
#endif

    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = selection->dataManager().data();

    return KisHLineIteratorPixel(m_datamanager.data(), selectionDm, x, y, w, m_d->x, m_d->y);
}

KisHLineConstIteratorPixel  KisPaintDevice::createHLineConstIterator(qint32 x, qint32 y, qint32 w, const KisSelection * selection) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = const_cast< KisDataManager*>(selection->dataManager().data());

    return KisHLineConstIteratorPixel(dm, selectionDm, x, y, w, m_d->x, m_d->y);
}

KisVLineIteratorPixel  KisPaintDevice::createVLineIterator(qint32 x, qint32 y, qint32 h, const KisSelection * selection)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, 1, h );
#endif

    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = selection->dataManager().data();

    return KisVLineIteratorPixel(m_datamanager.data(), selectionDm, x, y, h, m_d->x, m_d->y);
}

KisVLineConstIteratorPixel  KisPaintDevice::createVLineConstIterator(qint32 x, qint32 y, qint32 h, const KisSelection * selection) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = const_cast< KisDataManager*>(selection->dataManager().data());

    return KisVLineConstIteratorPixel(dm, selectionDm, x, y, h, m_d->x, m_d->y);
}

KisRandomAccessorPixel KisPaintDevice::createRandomAccessor(qint32 x, qint32 y, const KisSelection * selection)
{
    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = selection->dataManager().data();

    return KisRandomAccessorPixel(m_datamanager.data(), selectionDm, x, y, m_d->x, m_d->y);
}

KisRandomConstAccessorPixel KisPaintDevice::createRandomConstAccessor(qint32 x, qint32 y, const KisSelection * selection) const
{
    KisDataManager* dm = const_cast< KisDataManager*>(m_datamanager.data()); // TODO: don't do this
    KisDataManager* selectionDm = 0;

    if(selection)
        selectionDm = const_cast< KisDataManager*>(selection->dataManager().data());

    return KisRandomConstAccessorPixel(dm, selectionDm, x, y, m_d->x, m_d->y);
}

KisRandomSubAccessorPixel KisPaintDevice::createRandomSubAccessor( const KisSelection * selection ) const
{
    KisPaintDevice* pd = const_cast<KisPaintDevice*>(this);
    return KisRandomSubAccessorPixel(pd);
}

void KisPaintDevice::clearSelection( KisSelectionSP selection )
{
    QRect r = selection->selectedExactRect();

    if (r.isValid()) {

        KisHLineIterator devIt = createHLineIterator(r.x(), r.y(), r.width());
        KisHLineConstIterator selectionIt = selection->createHLineIterator(r.x(), r.y(), r.width());

        for (qint32 y = 0; y < r.height(); y++) {

            while (!devIt.isDone()) {
                // XXX: Optimize by using stretches

                m_d->colorSpace->applyInverseAlphaU8Mask( devIt.rawData(), selectionIt.rawData(), 1);

                ++devIt;
                ++selectionIt;
            }
            devIt.nextRow();
            selectionIt.nextRow();
        }

        setDirty(r);
    }
}

void KisPaintDevice::applySelectionMask(KisSelectionSP mask)
{
    QRect r = mask->selectedExactRect();
    crop(r);

    KisHLineIterator pixelIt = createHLineIterator(r.x(), r.top(), r.width());
    KisHLineConstIterator maskIt = mask->createHLineIterator(r.x(), r.top(), r.width());

    for (qint32 y = r.top(); y <= r.bottom(); ++y) {

        while (!pixelIt.isDone()) {
            // XXX: Optimize by using stretches

            m_d->colorSpace->applyAlphaU8Mask( pixelIt.rawData(), maskIt.rawData(), 1);

            ++pixelIt;
            ++maskIt;
        }
        pixelIt.nextRow();
        maskIt.nextRow();
    }
}

bool KisPaintDevice::pixel(qint32 x, qint32 y, QColor *c, quint8 *opacity)
{
    KisHLineConstIteratorPixel iter = createHLineIterator(x, y, 1);

    const quint8 *pix = iter.rawData();

    if (!pix) return false;

    colorSpace()->toQColor(pix, c, opacity);

    return true;
}


bool KisPaintDevice::pixel(qint32 x, qint32 y, KoColor * kc)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);

    quint8 *pix = iter.rawData();

    if (!pix) return false;

    kc->setColor(pix, m_d->colorSpace);

    return true;
}

KoColor KisPaintDevice::colorAt(qint32 x, qint32 y) const
{
    KisRandomConstAccessorPixel iter = createRandomConstAccessor(x, y);
    return KoColor(iter.rawData(), m_d->colorSpace);
}

bool KisPaintDevice::setPixel(qint32 x, qint32 y, const QColor& c, quint8  opacity)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);

    colorSpace()->fromQColor(c, opacity, iter.rawData());

    return true;
}

bool KisPaintDevice::setPixel(qint32 x, qint32 y, const KoColor& kc)
{
    const quint8 * pix;
    if (kc.colorSpace() != m_d->colorSpace) {
        KoColor kc2 (kc, m_d->colorSpace);
        pix = kc2.data();
    }
    else {
        pix = kc.data();
    }

    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1);
    memcpy(iter.rawData(), pix, m_d->colorSpace->pixelSize());

    return true;
}


qint32 KisPaintDevice::numContiguousColumns(qint32 x, qint32 minY, qint32 maxY) const
{
    return m_datamanager->numContiguousColumns(x - m_d->x, minY - m_d->y, maxY - m_d->y);
}

qint32 KisPaintDevice::numContiguousRows(qint32 y, qint32 minX, qint32 maxX) const
{
    return m_datamanager->numContiguousRows(y - m_d->y, minX - m_d->x, maxX - m_d->x);
}

qint32 KisPaintDevice::rowStride(qint32 x, qint32 y) const
{
    return m_datamanager->rowStride(x - m_d->x, y - m_d->y);
}

void KisPaintDevice::setX(qint32 x)
{
    m_d->x = x;
}

void KisPaintDevice::setY(qint32 y)
{
    m_d->y = y;
}


void KisPaintDevice::readBytes(quint8 * data, qint32 x, qint32 y, qint32 w, qint32 h)
{
    m_datamanager->readBytes(data, x - m_d->x, y - m_d->y, w, h);
}

void KisPaintDevice::readBytes(quint8 * data, const QRect &rect)
{
    readBytes(data, rect.x(), rect.y(), rect.width(), rect.height());
}

void KisPaintDevice::writeBytes(const quint8 * data, qint32 x, qint32 y, qint32 w, qint32 h)
{
#ifdef CACHE_EXACT_BOUNDS
    m_d->exactBounds &= QRect( x, y, w, h );
#endif
    m_datamanager->writeBytes( data, x - m_d->x, y - m_d->y, w, h);
}

void KisPaintDevice::writeBytes(const quint8 * data, const QRect &rect)
{
    writeBytes(data, rect.x(), rect.y(), rect.width(), rect.height());
}

QVector<quint8*> KisPaintDevice::readPlanarBytes( qint32 x, qint32 y, qint32 w, qint32 h)
{
    return m_datamanager->readPlanarBytes( channelSizes(), x, y, w, h );
}

void KisPaintDevice::writePlanarBytes( QVector<quint8*> planes, qint32 x, qint32 y, qint32 w, qint32 h)
{
    m_datamanager->writePlanarBytes( planes, channelSizes(), x, y, w, h );
}


KisDataManagerSP KisPaintDevice::dataManager() const
{
    return m_datamanager;
}


quint32 KisPaintDevice::pixelSize() const
{
    Q_ASSERT(m_d->pixelSize > 0);
    return m_d->pixelSize;
}

quint32 KisPaintDevice::channelCount() const
{
    Q_ASSERT(m_d->nChannels > 0);
    return m_d->nChannels;
;
}

KoColorSpace * KisPaintDevice::colorSpace()
{
    Q_ASSERT(m_d->colorSpace != 0);
    return m_d->colorSpace;
}

const KoColorSpace * KisPaintDevice::colorSpace() const
{
    Q_ASSERT(m_d->colorSpace != 0);
    return m_d->colorSpace;
}


qint32 KisPaintDevice::x() const
{
    return m_d->x;
}

qint32 KisPaintDevice::y() const
{
    return m_d->y;
}

QVector<qint32> KisPaintDevice::channelSizes()
{
    QVector<qint32> sizes;
    QList<KoChannelInfo*> channels = colorSpace()->channels();
    qSort( channels );

    foreach( KoChannelInfo * channelInfo, channels )
    {
        sizes.append( channelInfo->size() );
    }
    return sizes;
}

#include "kis_paint_device.moc"
