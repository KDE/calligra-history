/*
 * This file is part of the KDE project
 *
 * Copyright (c) 2005 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
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

#include "kis_dropshadow_plugin.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kis_debug.h>
#include <kgenericfactory.h>
#include <kactioncollection.h>

#include "kis_view2.h"
#include "kis_types.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_layer.h"
#include "kis_statusbar.h"
#include "widgets/kis_progress_widget.h"

#include <KoColorSpace.h>
#include <KoProgressUpdater.h>
#include <KoUpdater.h>

#include "kis_dropshadow.h"
#include "dlg_dropshadow.h"

K_EXPORT_COMPONENT_FACTORY(kritadropshadow, KGenericFactory<KisDropshadowPlugin>("krita"))

KisDropshadowPlugin::KisDropshadowPlugin(QObject *parent, const QStringList &)
        : KParts::Plugin(parent)
{
    if (parent->inherits("KisView2")) {

        setComponentData(KGenericFactory<KisDropshadowPlugin>::componentData());

        setXMLFile(KStandardDirs::locate("data", "kritaplugins/dropshadow.rc"), true);

        m_view = (KisView2*) parent;
        KAction *action  = new KAction(i18n("Add Drop Shadow..."), this);
        actionCollection()->addAction("dropshadow", action);
        connect(action, SIGNAL(triggered()), this, SLOT(slotDropshadow()));
    }
}

KisDropshadowPlugin::~KisDropshadowPlugin()
{
}

void KisDropshadowPlugin::slotDropshadow()
{
    KisImageWSP image = m_view->image();
    if (!image) return;

    KisPaintDeviceSP dev = m_view->activeDevice();
    if (!dev) return;

    DlgDropshadow * dlgDropshadow = new DlgDropshadow(dev->colorSpace()->name(),
            image->colorSpace()->name(),
            m_view, "Dropshadow");
    Q_CHECK_PTR(dlgDropshadow);

    dlgDropshadow->setCaption(i18n("Drop Shadow"));

    if (dlgDropshadow->exec() == QDialog::Accepted) {

        KisDropshadow dropshadow(m_view);

        KoProgressUpdater* updater = m_view->createProgressUpdater();
        updater->start();
        QPointer<KoUpdater> u = updater->startSubtask();
        dropshadow.dropshadow(u,
                              dlgDropshadow->getXOffset(),
                              dlgDropshadow->getYOffset(),
                              dlgDropshadow->getBlurRadius(),
                              dlgDropshadow->getShadowColor(),
                              dlgDropshadow->getShadowOpacity(),
                              dlgDropshadow->allowResizingChecked());

        updater->deleteLater();
    }
    delete dlgDropshadow;

}

#include "kis_dropshadow_plugin.moc"
