/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004-2008 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef KIS_COMPLEXOP_SETTINGS_H_
#define KIS_COMPLEXOP_SETTINGS_H_

#include <kis_paintop_settings.h>
#include <kis_types.h>
#include "kis_complexop_settings_widget.h"

class QDomElement;
class QPointF;
class QPainter;
class KoViewConverter;

class KisComplexOpSettings : public KisPaintOpSettings
{
    
public:

    KisComplexOpSettings();
    virtual ~KisComplexOpSettings();

    bool paintIncremental();
    virtual void paintOutline(const QPointF& pos, KisImageWSP image, QPainter &painter, const KoViewConverter &converter, OutlineMode _mode) const;

public:

    KisComplexOpSettingsWidget *m_options;

};


#endif // KIS_COMPLEXOP_SETTINGS_H_
