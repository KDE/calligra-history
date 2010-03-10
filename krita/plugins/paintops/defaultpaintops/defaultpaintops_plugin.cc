/*
 * defaultpaintops_plugin.cc -- Part of Krita
 *
 * Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
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

#include "defaultpaintops_plugin.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kis_debug.h>
#include <kpluginfactory.h>

#include "kis_simple_paintop_factory.h"
#include "kis_brushop.h"
#include "kis_brushop_settings_widget.h"
#include "kis_duplicateop_factory.h"
#include "kis_eraseop.h"
#include "kis_eraseop_settings_widget.h"
#include "kis_penop.h"
#include "kis_penop_settings_widget.h"
#include "kis_smudgeop.h"
#include "kis_smudgeop_settings_widget.h"
#include "kis_global.h"
#include "kis_paintop_registry.h"
#include "kis_brush_based_paintop_settings.h"

K_PLUGIN_FACTORY(DefaultPaintOpsPluginFactory, registerPlugin<DefaultPaintOpsPlugin>();)
K_EXPORT_PLUGIN(DefaultPaintOpsPluginFactory("krita"))


DefaultPaintOpsPlugin::DefaultPaintOpsPlugin(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    //setComponentData(DefaultPaintOpsPluginFactory::componentData());

    KisPaintOpRegistry *r = KisPaintOpRegistry::instance();
    r->add(new KisSimplePaintOpFactory<KisBrushOp, KisBrushBasedPaintOpSettings, KisBrushOpSettingsWidget>("paintbrush", i18n("Pixel Brush"), "krita-paintbrush.png"));
    r->add(new KisDuplicateOpFactory);
    r->add(new KisSimplePaintOpFactory<KisEraseOp, KisBrushBasedPaintOpSettings, KisEraseOpSettingsWidget>("eraser", i18n("Pixel Eraser"), "krita-eraser.png"));
    r->add(new KisSimplePaintOpFactory<KisPenOp, KisBrushBasedPaintOpSettings, KisPenOpSettingsWidget>("pencil", "Pixel Pencil", "krita-pencil.png"));
    r->add(new KisSimplePaintOpFactory<KisSmudgeOp, KisBrushBasedPaintOpSettings, KisSmudgeOpSettingsWidget>("smudge", i18n("Smudge Brush"), "krita-smudgebrush.png","smudge-finger"));
}

DefaultPaintOpsPlugin::~DefaultPaintOpsPlugin()
{
}

#include "defaultpaintops_plugin.moc"
