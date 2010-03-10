/*
 *  Copyright (c) 2010 Boudewijn Rempt <boud@valdyas.org>
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


#include "kis_global.h"
#include "kis_default_bounds.h"


struct KisDefaultBounds::Private
{
    KisImageWSP image;
};


KisDefaultBounds::KisDefaultBounds(KisImageWSP image)
    : m_d(new Private())
{
    m_d->image = image;
}

KisDefaultBounds::KisDefaultBounds(const KisDefaultBounds& rhs)
    : m_d(new Private())
{
    m_d->image = rhs.m_d->image;
}

KisDefaultBounds& KisDefaultBounds::operator=(const KisDefaultBounds& rhs)
{
    m_d->image = rhs.m_d->image;
    return *this;
}

KisDefaultBounds::~KisDefaultBounds()
{
    delete m_d;
}

QRect KisDefaultBounds::bounds() const
{
    /**
     * By default return infinite rect to cover everything
     */
    return m_d->image ? m_d->image->bounds() :
        QRect(qint32_MIN/2, qint32_MIN/2, qint32_MAX, qint32_MAX);
}
