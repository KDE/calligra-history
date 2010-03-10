/*
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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
#ifndef _KIS_BOUNDARY_H_
#define _KIS_BOUNDARY_H_

#include <QList>
#include <QPair>
#include <QPainter>

#include <krita_export.h>

#include "kis_types.h"

/**
 * Generates an 'outline' for a paint device. It should look a bit like the outline of a
 * marching ants selection.
 *
 * It's not really optimized, so it's not recommended to do big things with it and expect
 * it to be fast.
 *
 * Usage: construct a KisBoundary, and then run a generateBoundary(w, h) on it. After that,
 * you can use the KisBoundaryPainter::paint method to let it paint the outline, or get a pixmap.
 **/
class BRUSH_EXPORT KisBoundary
{
public:
    KisBoundary(KisFixedPaintDeviceSP dev);
    ~KisBoundary();
    void generateBoundary();
    
    void paint(QPainter& painter) const;

private:
    struct Private;
    Private* const d;
};

#endif // _KIS_BOUNDARY_H_
