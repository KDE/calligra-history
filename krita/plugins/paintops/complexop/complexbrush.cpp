/*
 *  Copyright (c) 2007 Emanuele Tamponi (emanuele@valinor.it)
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

#include "complexbrush.h"
#include "kis_simple_paintop_factory.h"
#include "kis_complexop.h"
#include "kis_complexop_settings.h"

#include <KGenericFactory>
#include <kis_paintop_registry.h>
#include <QStringList>

K_PLUGIN_FACTORY(ComplexBrushFactory, registerPlugin<ComplexBrush>();)
K_EXPORT_PLUGIN(ComplexBrushFactory("krita"))

ComplexBrush::ComplexBrush(QObject *parent, const QVariantList &sl)
        : QObject(parent)
{
    Q_UNUSED(sl)

    //setComponentData(ComplexBrushFactory::componentData());
    KisPaintOpRegistry::instance()->add(new KisSimplePaintOpFactory<KisComplexOp, KisComplexOpSettings, KisComplexOpSettingsWidget>("complex", i18n("Mixing Brush"),
                                                                                                                                    KisPaintOpFactory::categoryExperimental(),"krita-mixing.png"));
}

ComplexBrush::~ComplexBrush()
{
}

#include "complexbrush.moc"
