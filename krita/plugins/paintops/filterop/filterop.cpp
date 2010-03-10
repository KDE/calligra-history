/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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

#include "filterop.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kis_debug.h>
#include <kpluginfactory.h>

#include <kis_paintop_registry.h>
#include "kis_simple_paintop_factory.h"
#include "kis_filterop.h"
#include "kis_filterop_settings.h"
#include "kis_filterop_settings_widget.h"

K_PLUGIN_FACTORY(FilterOpFactory, registerPlugin<FilterOp>();)
K_EXPORT_PLUGIN(FilterOpFactory("krita"))

FilterOp::FilterOp(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    //setComponentData(FilterOpFactory::componentData());

    // This is not a gui plugin; only load it when the doc is created.
    KisPaintOpRegistry *r = KisPaintOpRegistry::instance();
    r->add(new KisSimplePaintOpFactory<KisFilterOp, KisFilterOpSettings, KisFilterOpSettingsWidget>("filter", i18n("Filter Brush"), "krita-filterop.png"));

}

FilterOp::~FilterOp()
{
}

#include "filterop.moc"

