/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org
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

#include "kis_projection.h"

#include <QRegion>
#include <QRect>
#include <QThread>
#include <QMutex>
#include <QApplication>

#include <kis_debug.h>
#include <kglobal.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>

#include "kis_image.h"
#include "kis_group_layer.h"

#include "kis_merge_walker.h"
#include "kis_full_refresh_walker.h"
#include "kis_async_merger.h"

class KisProjection::Private
{
public:

    KisImageUpdater* updater;
    KisImageWSP image;
    bool locked;
    int updateRectSize;
    QRect roi; // Region of interest
    bool useRegionOfInterest; // If false, update all dirty bits, if
    // true, update only region of interest.
};

KisProjection::KisProjection(KisImageWSP image)
        : QThread()
        , m_d(new Private)
{
    m_d->image = image;
    m_d->updater = 0;
    updateSettings();

    m_d->updater = new KisImageUpdater();
    m_d->updater->moveToThread(this);

    // Full refresh should be synchronous
    connect(this, SIGNAL(sigFullRefresh(KisNodeSP, QRect)),
            m_d->updater, SLOT(startFullRefresh(KisNodeSP, QRect)),
            Qt::BlockingQueuedConnection);

    connect(this, SIGNAL(sigUpdateProjection(KisNodeSP, QRect, QRect)), m_d->updater, SLOT(startUpdate(KisNodeSP, QRect, QRect)));
    connect(m_d->updater, SIGNAL(updateDone(QRect)), m_d->image, SLOT(slotProjectionUpdated(QRect)));
}


KisProjection::~KisProjection()
{
    m_d->updater->deleteLater();
    delete m_d;
}

void KisProjection::stop()
{
    quit();
}

void KisProjection::run()
{
    exec(); // start the event loop
}

void KisProjection::lock()
{
    Q_ASSERT(m_d->updater);

    blockSignals(true);
    m_d->updater->blockSignals(true);
    m_d->updater->lock();
}

void KisProjection::unlock()
{
    Q_ASSERT(m_d->updater);

    m_d->updater->unlock();
    m_d->updater->blockSignals(false);
    blockSignals(false);
}

void KisProjection::setRegionOfInterest(const QRect & roi)
{
    m_d->roi = roi;
}


void KisProjection::updateProjection(KisNodeSP node, const QRect& rc)
{
    if (!m_d->image)return;

    // The chunks do not run concurrently (there is only one KisImageUpdater and only
    // one event loop), but it is still useful, since intermediate results are passed
    // back to the main thread where the gui can be updated.
    QRect interestingRect = rc.intersected(m_d->image->bounds());

    if (m_d->useRegionOfInterest) {
        interestingRect = rc.intersected(m_d->roi);
    } else {
        interestingRect = rc;
    }

    int h = interestingRect.height();
    int w = interestingRect.width();
    int x = interestingRect.x();
    int y = interestingRect.y();

    // Note: we're doing columns first, so when we have a small strip left
    // at the bottom, we have as few and as long runs of pixels left
    // as possible.
    if (w <= m_d->updateRectSize && h <= m_d->updateRectSize) {
        emit sigUpdateProjection(node, interestingRect, m_d->image->bounds());
        return;
    }
    int wleft = w;
    int col = 0;
    while (wleft > 0) {
        int hleft = h;
        int row = 0;
        while (hleft > 0) {
            QRect rc2(col + x, row + y, qMin(wleft, m_d->updateRectSize), qMin(hleft, m_d->updateRectSize));

            emit sigUpdateProjection(node, rc2, m_d->image->bounds());

            hleft -= m_d->updateRectSize;
            row += m_d->updateRectSize;

        }
        wleft -= m_d->updateRectSize;
        col += m_d->updateRectSize;
    }
}

void KisProjection::fullRefresh(KisNodeSP root)
{
    // No splitting - trivial, but works :)
    emit sigFullRefresh(root, m_d->image->bounds());
}

void KisProjection::updateSettings()
{
    KConfigGroup cfg = KGlobal::config()->group("");
    m_d->updateRectSize = cfg.readEntry("updaterectsize", 512);
    m_d->useRegionOfInterest = cfg.readEntry("use_region_of_interest", false);
}

KisImageUpdater::KisImageUpdater()
    : m_mutex(QMutex::Recursive)
{
}

void KisImageUpdater::lock()
{
    m_mutex.lock();
}

void KisImageUpdater::unlock()
{
    m_mutex.unlock();
}

void KisImageUpdater::startFullRefresh(KisNodeSP node, const QRect& cropRect)
{
    QMutexLocker locker(&m_mutex);
    KisFullRefreshWalker walker(cropRect);
    KisAsyncMerger merger;

    walker.collectRects(node, cropRect);
    merger.startMerge(walker);

    QRect rect = walker.changeRect();
    emit updateDone(rect);
}

void KisImageUpdater::startUpdate(KisNodeSP node, const QRect& rc, const QRect& cropRect)
{
    QMutexLocker locker(&m_mutex);
    KisMergeWalker walker(cropRect);
    KisAsyncMerger merger;

    walker.collectRects(node, rc);
    merger.startMerge(walker);

    QRect rect = walker.changeRect();
    emit updateDone(rect);
}

#include "kis_projection.moc"
