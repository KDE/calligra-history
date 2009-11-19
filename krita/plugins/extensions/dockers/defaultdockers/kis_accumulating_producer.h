/*
 * This file is part of Krita
 *
 * Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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

#ifndef _KIS_ACCUMULATING_PRODUCER_H_
#define _KIS_ACCUMULATING_PRODUCER_H_

#include <QObject>
//Added by qt3to4:
#include <QCustomEvent>

#include <KoBasicHistogramProducers.h>
#include "kis_cachedhistogram.h"

/**
 * Kept very minimalistic because all options would require much reiterating which we don't want.
 * This class is multithreading! Don't expect it to contain the right data after an
 * addRegionsToBinAsync call, but await it's completed() signal. Also beware! This function
 * _does_ clear() before addRegionsToBinAsync! (hence not conforming to the regular semantics
 * of HistogramProducers if you'd take addRegionsToBinAsync = addRegionToBin, but since that is
 * already violated with the asynchronousity of it that is not really an issue anymore, I think)
 **/
class KisAccumulatingHistogramProducer : public QObject, public KoBasicHistogramProducer
{
    Q_OBJECT
public:
    KisAccumulatingHistogramProducer(KisCachedHistogramObserver::Producers* source);
    ~KisAccumulatingHistogramProducer();
    /// Does _nothing_, use addRegionsToBinAsync
    virtual void addRegionToBin(const quint8 *, const quint8*, quint32, const KoColorSpace *) {}
    virtual void addRegionsToBinAsync();
    virtual QString positionToString(qreal pos) const {
        return m_source->at(0)->positionToString(pos);
    }

    virtual void setView(qreal, qreal) {} // No view support
    virtual qreal maximalZoom() const {
        return 1.0;
    }

    virtual qint32 numberOfBins() {
        return m_source->at(0)->numberOfBins();
    }

    virtual QList<KoChannelInfo *> channels() {
        return m_source->at(0)->channels();
    }

    /// Call this when the 'source' list has changed colorspace
    virtual void changedSourceProducer() {
        m_count = m_source->at(0)->channels().count();
        m_external.clear();
        makeExternalToInternal();
    }

signals:
    void completed();

protected:
    virtual bool event(QEvent* e);
    /// source already converts external to internal
    virtual int externalToInternal(int ext) {
        return ext;
    }
    KisCachedHistogramObserver::Producers* m_source;

    class ThreadedProducer;
    friend class ThreadedProducer;
    ThreadedProducer* m_thread;
};

#endif // _KIS_ACCUMULATING_PRODUCER_H_
