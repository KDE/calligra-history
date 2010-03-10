/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
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
#include "kis_brush_registry.h"

#include <QString>

#include <kaction.h>
#include <kglobal.h>
#include <klocale.h>
#include <kparts/plugin.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kparts/componentfactory.h>

#include <KoPluginLoader.h>

#include <kis_debug.h>

#include "kis_brush_server.h"
#include "kis_brush_factory.h"
#include "kis_auto_brush_factory.h"
#include "kis_gbr_brush_factory.h"
#include "kis_abr_brush_factory.h"
#include "kis_text_brush_factory.h"

KisBrushRegistry::KisBrushRegistry()
{
    KisBrushServer::instance();
}

KisBrushRegistry::~KisBrushRegistry()
{
    dbgRegistry << "deleting KisBrushRegistry";
}

KisBrushRegistry* KisBrushRegistry::instance()
{
    K_GLOBAL_STATIC(KisBrushRegistry, s_instance);
    if (!s_instance.exists()) {
        s_instance->add(new KisAutoBrushFactory());
        s_instance->add(new KisGbrBrushFactory());
        s_instance->add(new KisAbrBrushFactory());
        s_instance->add(new KisTextBrushFactory());
        KoPluginLoader::instance()->load("Krita/Brush", "Type == 'Service' and ([X-Krita-Version] == 3)");
    }
    return s_instance;
}


KisBrushSP KisBrushRegistry::getOrCreateBrush(const QDomElement& element)
{
    QString brushType = element.attribute("type");

    if (brushType.isEmpty()) return 0;

    KisBrushFactory* factory = get(brushType);
    if (!factory) return 0;

    KisBrushSP brush = factory->getOrCreateBrush(element);
    return brush;
}

#include "kis_brush_registry.moc"
