/*
 * Copyright (c) 2008-2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "shivagenerator.h"

#include <QMutex>

#include <KoUpdater.h>

#include <kis_paint_device.h>
#include <filter/kis_filter_configuration.h>
#include <kis_processing_information.h>

#include <GTLCore/Region.h>
#include <OpenShiva/Kernel.h>

#include <ShivaGeneratorConfigWidget.h>
#include <OpenShiva/Source.h>
#include "Version.h"

#include METADATA_HEADER

#include "PaintDeviceImage.h"
#include "QVariantValue.h"
#include "UpdaterProgressReport.h"

#if OPENSHIVA_13_OR_MORE
#include "GTLCore/CompilationMessages.h"
#endif

extern QMutex* shivaMutex;

ShivaGenerator::ShivaGenerator(OpenShiva::Source* kernel) : KisGenerator(KoID(kernel->name().c_str(), kernel->name().c_str()), KoID("basic"), kernel->name().c_str()), m_source(kernel)
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(true);
    setSupportsIncrementalPainting(false);
}

KisConfigWidget * ShivaGenerator::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev, const KisImageWSP image) const
{
    Q_UNUSED(dev);
    Q_UNUSED(image);
    return new ShivaGeneratorConfigWidget(m_source, parent);
}

void ShivaGenerator::generate(KisProcessingInformation dstInfo,
                              const QSize& size,
                              const KisFilterConfiguration* config,
                              KoUpdater* progressUpdater) const
{
    Q_UNUSED(progressUpdater);
    KisPaintDeviceSP dst = dstInfo.paintDevice();
    QPoint dstTopLeft = dstInfo.topLeft();

    UpdaterProgressReport* report = 0;
    if (progressUpdater) {
        progressUpdater->setRange(0, size.height());
        report = new UpdaterProgressReport(progressUpdater);
    }
    
    Q_ASSERT(!dst.isNull());
//     Q_ASSERT(config);
    // TODO implement the generating of pixel
    OpenShiva::Kernel kernel;
    kernel.setSource(*m_source);

    if (config) {
        QMap<QString, QVariant> map = config->getProperties();
        for (QMap<QString, QVariant>::iterator it = map.begin(); it != map.end(); ++it) {
            const GTLCore::Metadata::Entry* entry = kernel.metadata()->parameter(it.key().toAscii().data());
            if (entry && entry->asParameterEntry()) {
#if OPENSHIVA_12
                GTLCore::Value val = qvariantToValue(it.value(), entry->asParameterEntry()->valueType());
#else
                GTLCore::Value val = qvariantToValue(it.value(), entry->asParameterEntry()->type());
#endif
                if(val.isValid())
                {
                    kernel.setParameter(it.key().toAscii().data(), val);
                }
            }
        }
    }
    {
        QMutexLocker l(shivaMutex);
        kernel.compile();
    }
    if (kernel.isCompiled()) {
        PaintDeviceImage pdi(dst);
#if OPENSHIVA_12
        std::list< GTLCore::AbstractImage* > inputs;
        GTLCore::Region region(dstTopLeft.x(), dstTopLeft.y() , size.width(), size.height());
        kernel.evaluatePixeles(region, inputs, &pdi, report );
#else
        std::list< const GTLCore::AbstractImage* > inputs;
        GTLCore::RegionI region(dstTopLeft.x(), dstTopLeft.y() , size.width(), size.height());
        kernel.evaluatePixels(region, inputs, &pdi, report );
#endif
    }
#if OPENSHIVA_13_OR_MORE
    else {
        dbgPlugins << "Error: " << kernel.compilationMessages().toString().c_str();
    }
#endif
}
