/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_tiled_data_manager_test.h"
#include <qtest_kde.h>

#include "tiles3/kis_tiled_data_manager.h"

#include "tiles_test_utils.h"

bool KisTiledDataManagerTest::checkHole(quint8* buffer,
                                        quint8 holeColor, QRect holeRect,
                                        quint8 backgroundColor, QRect backgroundRect)
{
    for(qint32 y = backgroundRect.y(); y <= backgroundRect.bottom(); y++) {
        for(qint32 x = backgroundRect.x(); x <= backgroundRect.right(); x++) {
            quint8 expectedColor = holeRect.contains(x,y) ? holeColor : backgroundColor;

            if(*buffer != expectedColor) {
                qDebug() << "Expected" << expectedColor << "but found" << *buffer;
                return false;
            }

            buffer++;
        }
    }
    return true;
}

bool KisTiledDataManagerTest::checkTilesShared(KisTiledDataManager *srcDM,
                                               KisTiledDataManager *dstDM,
                                               bool takeOldSrc,
                                               bool takeOldDst,
                                               QRect tilesRect)
{
    for(qint32 row = tilesRect.y(); row <= tilesRect.bottom(); row++) {
        for(qint32 col = tilesRect.x(); col <= tilesRect.right(); col++) {
            KisTileSP srcTile = takeOldSrc ? srcDM->getOldTile(col, row)
                : srcDM->getTile(col, row, false);
            KisTileSP dstTile = takeOldDst ? dstDM->getOldTile(col, row)
                : dstDM->getTile(col, row, false);

            if(srcTile->tileData() != dstTile->tileData()) {
                qDebug() << "Expected tile data (" << col << row << ")"
                         << srcTile->extent()
                         << srcTile->tileData()
                         << "but found" << dstTile->tileData();
                qDebug() << "Expected" << srcTile->data()[0] << "but found" << dstTile->data()[0];
                return false;
            }
        }
    }
    return true;
}

bool KisTiledDataManagerTest::checkTilesNotShared(KisTiledDataManager *srcDM,
                                                  KisTiledDataManager *dstDM,
                                                  bool takeOldSrc,
                                                  bool takeOldDst,
                                                  QRect tilesRect)
{
    for(qint32 row = tilesRect.y(); row <= tilesRect.bottom(); row++) {
        for(qint32 col = tilesRect.x(); col <= tilesRect.right(); col++) {
            KisTileSP srcTile = takeOldSrc ? srcDM->getOldTile(col, row)
                : srcDM->getTile(col, row, false);
            KisTileSP dstTile = takeOldDst ? dstDM->getOldTile(col, row)
                : dstDM->getTile(col, row, false);

            if(srcTile->tileData() == dstTile->tileData()) {
                qDebug() << "Expected tiles not be shared:"<< srcTile->extent();
                return false;
            }
        }
    }
    return true;
}

void KisTiledDataManagerTest::testUnversionedBitBlt()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager srcDM(1, &defaultPixel);
    KisTiledDataManager dstDM(1, &defaultPixel);

    quint8 oddPixel1 = 128;
    quint8 oddPixel2 = 129;

    QRect rect(0,0,512,512);
    QRect cloneRect(81,80,250,250);
    QRect tilesRect(2,2,3,3);

    srcDM.clear(rect, &oddPixel1);
    dstDM.clear(rect, &oddPixel2);

    dstDM.bitBlt(&srcDM, cloneRect);

    quint8 *buffer = new quint8[rect.width()*rect.height()];

    dstDM.readBytes(buffer, rect.x(), rect.y(), rect.width(), rect.height());

    QVERIFY(checkHole(buffer, oddPixel1, cloneRect,
                      oddPixel2, rect));

    delete[] buffer;

    // Test whether tiles became shared
    QVERIFY(checkTilesShared(&srcDM, &dstDM, false, false, tilesRect));
}

void KisTiledDataManagerTest::testVersionedBitBlt()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager srcDM1(1, &defaultPixel);
    KisTiledDataManager srcDM2(1, &defaultPixel);
    KisTiledDataManager dstDM(1, &defaultPixel);

    quint8 oddPixel1 = 128;
    quint8 oddPixel2 = 129;
    quint8 oddPixel3 = 130;

    quint8 oddPixel4 = 131;

    QRect rect(0,0,512,512);
    QRect cloneRect(81,80,250,250);
    QRect tilesRect(2,2,3,3);


    KisMementoSP memento1 = srcDM1.getMemento();
    srcDM1.clear(rect, &oddPixel1);

    srcDM2.clear(rect, &oddPixel2);
    dstDM.clear(rect, &oddPixel3);

    KisMementoSP memento2 = dstDM.getMemento();
    dstDM.bitBlt(&srcDM1, cloneRect);

    QVERIFY(checkTilesShared(&srcDM1, &dstDM, false, false, tilesRect));
    QVERIFY(checkTilesNotShared(&srcDM1, &srcDM1, true, false, tilesRect));
    QVERIFY(checkTilesNotShared(&dstDM, &dstDM, true, false, tilesRect));

    dstDM.commit();
    QVERIFY(checkTilesShared(&dstDM, &dstDM, true, false, tilesRect));

    KisMementoSP memento3 = srcDM2.getMemento();
    srcDM2.clear(rect, &oddPixel4);

    KisMementoSP memento4 = dstDM.getMemento();
    dstDM.bitBlt(&srcDM2, cloneRect);

    QVERIFY(checkTilesShared(&srcDM2, &dstDM, false, false, tilesRect));
    QVERIFY(checkTilesNotShared(&srcDM2, &srcDM2, true, false, tilesRect));
    QVERIFY(checkTilesNotShared(&dstDM, &dstDM, true, false, tilesRect));

    dstDM.commit();
    QVERIFY(checkTilesShared(&dstDM, &dstDM, true, false, tilesRect));

    dstDM.rollback(memento4);
    QVERIFY(checkTilesShared(&srcDM1, &dstDM, false, false, tilesRect));
    QVERIFY(checkTilesShared(&dstDM, &dstDM, true, false, tilesRect));
    QVERIFY(checkTilesNotShared(&srcDM1, &srcDM1, true, false, tilesRect));

    dstDM.rollforward(memento4);
    QVERIFY(checkTilesShared(&srcDM2, &dstDM, false, false, tilesRect));
    QVERIFY(checkTilesShared(&dstDM, &dstDM, true, false, tilesRect));
    QVERIFY(checkTilesNotShared(&srcDM1, &srcDM1, true, false, tilesRect));
}

void KisTiledDataManagerTest::testBitBltRough()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager srcDM(1, &defaultPixel);
    KisTiledDataManager dstDM(1, &defaultPixel);

    quint8 oddPixel1 = 128;
    quint8 oddPixel2 = 129;

    QRect rect(0,0,512,512);
    QRect cloneRect(81,80,250,250);
    QRect actualCloneRect(64,64,320,320);
    QRect tilesRect(1,1,4,4);

    srcDM.clear(rect, &oddPixel1);
    dstDM.clear(rect, &oddPixel2);

    dstDM.bitBltRough(&srcDM, cloneRect);

    quint8 *buffer = new quint8[rect.width()*rect.height()];

    dstDM.readBytes(buffer, rect.x(), rect.y(), rect.width(), rect.height());

    QVERIFY(checkHole(buffer, oddPixel1, actualCloneRect,
                      oddPixel2, rect));

    delete[] buffer;

    // Test whether tiles became shared
    QVERIFY(checkTilesShared(&srcDM, &dstDM, false, false, tilesRect));
}

void KisTiledDataManagerTest::testTransactions()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager dm(1, &defaultPixel);

    quint8 oddPixel1 = 128;
    quint8 oddPixel2 = 129;
    quint8 oddPixel3 = 130;

    KisTileSP tile00;
    KisTileSP oldTile00;

    // Create a named transaction: versioning is enabled
    KisMementoSP memento1 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel1);

    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel1, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(defaultPixel, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    // Create an anonymous transaction: versioning is disabled
    dm.commit();
    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel1, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel1, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    dm.clear(0, 0, 64, 64, &oddPixel2);

    // Versioning is disabled, i said! >:)
    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel2, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel2, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    // And the last round: named transaction:
    KisMementoSP memento2 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel3);

    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel3, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel2, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

}

void KisTiledDataManagerTest::testPurgeHistory()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager dm(1, &defaultPixel);

    quint8 oddPixel1 = 128;
    quint8 oddPixel2 = 129;
    quint8 oddPixel3 = 130;
    quint8 oddPixel4 = 131;

    KisMementoSP memento1 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel1);
    dm.commit();

    KisMementoSP memento2 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel2);

    KisTileSP tile00;
    KisTileSP oldTile00;

    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel2, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel1, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    dm.purgeHistory(memento1);

    /**
     * Nothing nas changed in the visible state of the data manager
     */

    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel2, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel1, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    dm.commit();

    dm.purgeHistory(memento2);

    /**
     * We've removed all the history of the device, so it
     * became "unversioned".
     * NOTE: the return value for getOldTile() when there is no
     * history present is a subject for change
     */

    tile00 = dm.getTile(0, 0, false);
    oldTile00 = dm.getOldTile(0, 0);
    QVERIFY(memoryIsFilled(oddPixel2, tile00->data(), TILESIZE));
    QVERIFY(memoryIsFilled(oddPixel2, oldTile00->data(), TILESIZE));
    tile00 = oldTile00 = 0;

    /**
     * Just test we won't crash when the memento is not
     * present in history anymore
     */

    KisMementoSP memento3 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel3);
    dm.commit();

    KisMementoSP memento4 = dm.getMemento();
    dm.clear(0, 0, 64, 64, &oddPixel4);
    dm.commit();

    dm.rollback(memento4);

    dm.purgeHistory(memento3);
    dm.purgeHistory(memento4);
}

//#include <valgrind/callgrind.h>

void KisTiledDataManagerTest::benchmarkReadOnlyTileLazy()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager dm(1, &defaultPixel);

    const qint32 numTilesToTest = 1000000;

    //CALLGRIND_START_INSTRUMENTATION;

    QBENCHMARK_ONCE {
        for(qint32 i = 0; i < numTilesToTest; i++) {
            KisTileSP tile = dm.getTile(i, i, false);
        }
    }

    //CALLGRIND_STOP_INSTRUMENTATION;
}

class KisSimpleClass : public KisShared
{
    qint64 m_int;
};

typedef KisSharedPtr<KisSimpleClass> KisSimpleClassSP;
void KisTiledDataManagerTest::benchmarkSharedPointers()
{
    const qint32 numIterations = 2 * 1000000;

    //CALLGRIND_START_INSTRUMENTATION;

    QBENCHMARK_ONCE {
        for(qint32 i = 0; i < numIterations; i++) {
            KisSimpleClassSP pointer = new KisSimpleClass;
            pointer = 0;
        }
    }

    //CALLGRIND_STOP_INSTRUMENTATION;
}



/******************* Stress job ***********************/

//#define NUM_CYCLES 9000
#define NUM_CYCLES 10000
#define NUM_TYPES 12

#define TILE_DIMENSION 64


/**
 * The data manager has partial guarantees of reentrancy. That is
 * you can call any arbitrary number of methods concurrently as long
 * as their access areas do not intersect.
 *
 * Though the rule can be quite tricky -- some of the methods always
 * use entire image as their access area, so they cannot be called
 * concurrently in any circumstances.
 * The examples are: clear(), commit(), rollback() and etc...
 */

#define run_exclusive(lock, _i) for(_i = 0, (lock).lockForWrite(); _i < 1; _i++, (lock).unlock())
#define run_concurrent(lock, _i) for(_i = 0, (lock).lockForRead(); _i < 1; _i++, (lock).unlock())
//#define run_exclusive(lock, _i) while(0)
//#define run_concurrent(lock, _i) while(0)


class KisStressJob : public QRunnable
{
public:
    KisStressJob(KisTiledDataManager &dataManager, QRect rect, QReadWriteLock &_lock)
        : m_accessRect(rect), dm(dataManager), lock(_lock)
    {
    }

    void run() {
        qsrand(QTime::currentTime().msec());
        for(qint32 i = 0; i < NUM_CYCLES; i++) {
            qint32 type = qrand() % NUM_TYPES;

            qint32 t;

            switch(type) {
            case 0:
                run_concurrent(lock,t) {
                    quint8 *buf;
                    buf = new quint8[dm.pixelSize()];
                    memcpy(buf, dm.defaultPixel(), dm.pixelSize());
                    dm.setDefaultPixel(buf);
                    delete[] buf;
                }
                break;
            case 1:
            case 2:
                run_concurrent(lock,t) {
                    KisTileSP tile;

                    tile = dm.getTile(m_accessRect.x() / TILE_DIMENSION,
                                      m_accessRect.y() / TILE_DIMENSION, false);
                    tile->lockForRead();
                    tile->unlock();
                    tile = dm.getTile(m_accessRect.x() / TILE_DIMENSION,
                                      m_accessRect.y() / TILE_DIMENSION, true);
                    tile->lockForWrite();
                    tile->unlock();

                    tile = dm.getOldTile(m_accessRect.x() / TILE_DIMENSION,
                                         m_accessRect.y() / TILE_DIMENSION);
                    tile->lockForRead();
                    tile->unlock();
                }
                break;
            case 3:
                run_concurrent(lock,t) {
                    QRect newRect = dm.extent();
                }
                break;
            case 4:
                run_concurrent(lock,t) {
                    dm.clear(m_accessRect.x(), m_accessRect.y(),
                             m_accessRect.width(), m_accessRect.height(), 4);
                }
                break;
            case 5:
                run_concurrent(lock,t) {
                    quint8 *buf;

                    buf = new quint8[m_accessRect.width() * m_accessRect.height() *
                                     dm.pixelSize()];
                    dm.readBytes(buf, m_accessRect.x(), m_accessRect.y(),
                                 m_accessRect.width(), m_accessRect.height());
                    dm.writeBytes(buf, m_accessRect.x(), m_accessRect.y(),
                                  m_accessRect.width(), m_accessRect.height());
                    delete[] buf;
                    }
                break;
            case 6:
                run_concurrent(lock,t) {
                    quint8 oddPixel = 13;
                    KisTiledDataManager srcDM(1, &oddPixel);
                    dm.bitBlt(&srcDM, m_accessRect);
                }
                break;
            case 7:
            case 8:
                run_exclusive(lock,t) {
                    m_memento = dm.getMemento();
                    dm.clear(m_accessRect.x(), m_accessRect.y(),
                             m_accessRect.width(), m_accessRect.height(), 2);
                    dm.commit();

                    dm.rollback(m_memento);
                    dm.rollforward(m_memento);

                    dm.purgeHistory(m_memento);
                    m_memento = 0;
                }
                break;
            case 9:
                run_exclusive(lock,t) {
                    bool b = dm.hasCurrentMemento();
                    Q_UNUSED(b);
                }
                break;
            case 10:
                run_exclusive(lock,t) {
                    dm.clear();
                }
                break;
            case 11:
                run_exclusive(lock,t) {
                    dm.setExtent(m_accessRect);
                }
                break;
            }
        }
    }

private:
    KisMementoSP m_memento;
    QRect m_accessRect;
    KisTiledDataManager &dm;
    QReadWriteLock &lock;
};

void KisTiledDataManagerTest::stressTest()
{
    quint8 defaultPixel = 0;
    KisTiledDataManager dm(1, &defaultPixel);
    QReadWriteLock lock;

    QThreadPool pool;
    pool.setMaxThreadCount(NUM_TYPES);

    QRect accessRect(0,0,100,100);
    for(qint32 i = 0; i < NUM_TYPES; i++) {
        KisStressJob *job = new KisStressJob(dm, accessRect, lock);
        pool.start(job);
        accessRect.translate(100, 0);
    }
    pool.waitForDone();
}

QTEST_KDEMAIN(KisTiledDataManagerTest, NoGUI)
#include "kis_tiled_data_manager_test.moc"

