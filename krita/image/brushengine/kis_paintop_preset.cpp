/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
 * Copyright (C) Sven Langkamp <sven.langkamp@gmail.com>, (C) 2009
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "kis_paintop_preset.h"

#include <QFile>
#include <QSize>
#include <QImage>
#include <QImageWriter>
#include <QImageReader>
#include <QDomDocument>
#include <QBuffer>

#include <KoColorSpaceRegistry.h>
#include <KoInputDevice.h>

#include "kis_types.h"
#include "kis_paintop_settings.h"
#include "kis_paintop_registry.h"
#include "kis_painter.h"
#include "kis_paint_information.h"
#include "kis_paint_device.h"
#include "kis_image.h"

struct KisPaintOpPreset::Private {
    KisPaintOpSettingsSP settings;
    QImage image;
};


KisPaintOpPreset::KisPaintOpPreset()
        : KoResource(QString())
        , m_d(new Private)
{
    m_d->settings = 0;
}

KisPaintOpPreset::KisPaintOpPreset(const QString & fileName)
        : KoResource(fileName)
        , m_d(new Private)
{
}

KisPaintOpPreset::~KisPaintOpPreset()
{
    delete m_d;
}

KisPaintOpPreset* KisPaintOpPreset::clone() const
{
    KisPaintOpPreset * preset = new KisPaintOpPreset();
    if (settings()) {
        preset->setSettings(settings()->clone());
    }
    preset->setPaintOp(paintOp());
    preset->setName(name());
    preset->setValid(valid());

    return preset;
}

void KisPaintOpPreset::setPaintOp(const KoID & paintOp)
{
    m_d->settings->setProperty("paintop", paintOp.id());
}

KoID KisPaintOpPreset::paintOp() const
{
    return KoID(m_d->settings->getString("paintop"), name());
}

void KisPaintOpPreset::setSettings(KisPaintOpSettingsSP settings)
{
    Q_ASSERT(!settings->getString("paintop", "").isEmpty());
    m_d->settings = settings->clone();
    setValid(true);
}

KisPaintOpSettingsSP KisPaintOpPreset::settings() const
{
    Q_ASSERT(!m_d->settings->getString("paintop", "").isEmpty());

    return m_d->settings;
}

bool KisPaintOpPreset::load()
{
    dbgImage << "Load preset " << filename();
    if (filename().isEmpty())
        return false;

    QImageReader reader(filename(), "PNG");

    QString version = reader.text("version");
    QString preset = reader.text("preset");
    
    dbgImage << version << preset;
    
    if (version != "2.2") {
        return false;
    }
    
    if (!reader.read(&m_d->image))
    {
        dbgImage << "Fail to decode PNG";
        return false;
    }
    
    QDomDocument doc;
    if (!doc.setContent(preset)) {
        return false;
    }
    fromXML(doc.documentElement());
    if(!m_d->settings)
        return false;
        
    return true;
}

bool KisPaintOpPreset::save()
{
    
    if (filename().isEmpty())
        return false;
    
    QString paintopid = m_d->settings->getString("paintop", "");
    if (paintopid.isEmpty())
        return false;
    
    QImageWriter writer(filename(), "PNG");

    QDomDocument doc;
    QDomElement root = doc.createElement("Preset");
    toXML(doc, root);
    doc.appendChild(root);

    writer.setText("version", "2.2");
    writer.setText("preset", doc.toString());
    
    kDebug() << "preset: " << doc.toString();
    
    QImage img;
    
    if(m_d->image.isNull())
    {
        img = QImage(1,1, QImage::Format_RGB32);
    } else {
        img = m_d->image;
    }
    
    return writer.write(img);
}

void KisPaintOpPreset::toXML(QDomDocument& doc, QDomElement& elt) const
{
    QString paintopid = m_d->settings->getString("paintop", "");

    elt.setAttribute("paintopid", paintopid);
    elt.setAttribute("name", name());

    m_d->settings->toXML(doc, elt);
}

void KisPaintOpPreset::fromXML(const QDomElement& presetElt)
{
    setName(presetElt.attribute("name"));
    QString paintopid = presetElt.attribute("paintopid");

    if (paintopid.isEmpty())
    {
        dbgImage << "No paintopid attribute";
        return;
    }

    KoID id(paintopid, "");

    KisPaintOpSettingsSP settings = KisPaintOpRegistry::instance()->settings(id, 0);
    if (!settings)
    {
        dbgImage << "No settings for " << paintopid;
        return;
    }
    settings->fromXML(presetElt);
    setSettings(settings);
}

QImage KisPaintOpPreset::image() const
{
    return m_d->image;
}

void KisPaintOpPreset::setImage(QImage image)
{
    m_d->image = image;
}

