/*
 *  Copyright (c) 2010 Lukáš Tvrdý lukast.dev@gmail.com
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

#include "kis_hline_iterator_benchmark.h"
#include "kis_benchmark_values.h"

#include "kis_iterators_pixel.h"
#include "kis_paint_device.h"

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>

#include <qtest_kde.h>

#include "kis_iterator_ng.h"

void KisHLineIteratorBenchmark::initTestCase()
{
    m_colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    m_device = new KisPaintDevice(m_colorSpace);
    m_color = new KoColor(m_colorSpace);
    // some random color
    m_color->fromQColor(QColor(0,0,250));
    m_device->fill(0,0,TEST_IMAGE_WIDTH,TEST_IMAGE_HEIGHT,m_color->data());
}

void KisHLineIteratorBenchmark::cleanupTestCase()
{
    delete m_color;
    delete m_device;
}


void KisHLineIteratorBenchmark::benchmarkCreation()
{
    QBENCHMARK{
        KisHLineIteratorPixel it = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);
    }
}

void KisHLineIteratorBenchmark::benchmarkWriteBytes()
{
    KisHLineIteratorPixel it = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!it.isDone()) {
                memcpy(it.rawData(), m_color->data(), m_colorSpace->pixelSize());
                ++it;
            }
            it.nextRow();
        }
    }
}

void KisHLineIteratorBenchmark::benchmarkReadBytes()
{
    KisHLineIteratorPixel it = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!it.isDone()) {
                memcpy(m_color->data(), it.rawData(), m_colorSpace->pixelSize());
                ++it;
            }
            it.nextRow();
        }
    }
}


void KisHLineIteratorBenchmark::benchmarkConstReadBytes()
{
    KisHLineConstIteratorPixel cit = m_device->createHLineConstIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!cit.isDone()) {
                memcpy(m_color->data(), cit.rawData(), m_colorSpace->pixelSize());
                ++cit;
            }
            cit.nextRow();
        }
    }
}

void KisHLineIteratorBenchmark::benchmarkReadWriteBytes(){
    KoColor c(m_colorSpace);
    c.fromQColor(QColor(250,120,0));
    KisPaintDevice dab(m_colorSpace);
    dab.fill(0,0,TEST_IMAGE_WIDTH,TEST_IMAGE_HEIGHT, c.data());
    
    KisHLineIteratorPixel writeIterator = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);
    KisHLineConstIteratorPixel constReadIterator = dab.createHLineConstIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!constReadIterator.isDone()) {
                memcpy(writeIterator.rawData(), constReadIterator.rawData(), m_colorSpace->pixelSize());
                ++constReadIterator;
                ++writeIterator;
            }
            constReadIterator.nextRow();
            writeIterator.nextRow();
        }
    }
}



void KisHLineIteratorBenchmark::benchmarkReadWriteBytes2()
{
    KoColor c(m_colorSpace);
    c.fromQColor(QColor(255,0,0));
    KisPaintDevice dab(m_colorSpace);
    dab.fill(0,0,TEST_IMAGE_WIDTH,TEST_IMAGE_HEIGHT, c.data());

    KisHLineIteratorSP writeIterator = m_device->createHLineIteratorNG(0,0,TEST_IMAGE_WIDTH);
    KisHLineIteratorSP readIterator = dab.createHLineIteratorNG(0,0,TEST_IMAGE_WIDTH);
    
    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            do {
                memcpy(writeIterator->rawData(), readIterator->rawData(), m_colorSpace->pixelSize());
                writeIterator->nextPixel();
            } while (readIterator->nextPixel());
            readIterator->nextRow();
            writeIterator->nextRow();
        }
    }

    ///QImage img = m_device->convertToQImage(m_device->colorSpace()->profile(),0,0,TEST_IMAGE_WIDTH, TEST_IMAGE_HEIGHT);
    //img.save("write.png");
    
}


void KisHLineIteratorBenchmark::benchmarkNoMemCpy()
{
    KisHLineIteratorPixel it = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!it.isDone()) {
                ++it;
            }
            it.nextRow();
        }
    }
}


void KisHLineIteratorBenchmark::benchmarkConstNoMemCpy()
{
    KisHLineConstIteratorPixel cit = m_device->createHLineConstIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!cit.isDone()) {
                ++cit;
            }
            cit.nextRow();
        }
    }
}

void KisHLineIteratorBenchmark::benchmarkTwoIteratorsNoMemCpy(){
    KoColor c(m_colorSpace);
    c.fromQColor(QColor(250,120,0));
    KisPaintDevice dab(m_colorSpace);
    dab.fill(0,0,TEST_IMAGE_WIDTH,TEST_IMAGE_HEIGHT, c.data());
    
    KisHLineIteratorPixel writeIterator = m_device->createHLineIterator(0, 0, TEST_IMAGE_WIDTH);
    KisHLineConstIteratorPixel constReadIterator = dab.createHLineConstIterator(0, 0, TEST_IMAGE_WIDTH);

    QBENCHMARK{
        for (int j = 0; j < TEST_IMAGE_HEIGHT; j++) {
            while (!constReadIterator.isDone()) {
                ++constReadIterator;
                ++writeIterator;
            }
            constReadIterator.nextRow();
            writeIterator.nextRow();
        }
    }
    
}



QTEST_KDEMAIN(KisHLineIteratorBenchmark, GUI)
#include "kis_hline_iterator_benchmark.moc"
