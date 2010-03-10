/* This file is part of the KDE project
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_RANDOM_ACCESSOR_NG_H_
#define _KIS_RANDOM_ACCESSOR_NG_H_

#include "kis_base_accessor.h"

class KRITAIMAGE_EXPORT KisRandomConstAccessorNG : public KisBaseConstAccessor
{
    Q_DISABLE_COPY(KisRandomConstAccessorNG)
public:
    KisRandomConstAccessorNG() {}
    virtual ~KisRandomConstAccessorNG();
    virtual void moveTo(qint32 x, qint32 y) = 0;
};

class KRITAIMAGE_EXPORT KisRandomAccessorNG : public KisRandomConstAccessorNG, public KisBaseAccessor
{
    Q_DISABLE_COPY(KisRandomAccessorNG)
public:
    KisRandomAccessorNG() {}
    virtual ~KisRandomAccessorNG();
};

#endif
