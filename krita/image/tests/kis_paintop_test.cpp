/*
 *  Copyright (c) 2007 Boudewijn Rempt boud@valdyas.org
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

#include "kis_paintop_test.h"

#include <qtest_kde.h>
#include "kis_paintop.h"
#include "kis_painter.h"
#include "kis_paint_device.h"

class TestPaintOp : public KisPaintOp
{
public:

    TestPaintOp(KisPainter * gc)
            : KisPaintOp(gc) {
    }

    double paintAt(const KisPaintInformation&) {
        return 0.0;
    }
    double spacing(double&, double&, double, double) const {
        return 0.5;
    }


};

void KisPaintopTest::testCreation()
{
    KisPainter p;
    TestPaintOp test(&p);
}


QTEST_KDEMAIN(KisPaintopTest, GUI)
#include "kis_paintop_test.moc"
