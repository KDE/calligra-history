/*
 * Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#include "shivafilter.h"

#include <QMutex>

#include <KoUpdater.h>

#include <kis_paint_device.h>
#include <filter/kis_filter_configuration.h>
#include <kis_processing_information.h>
#include <ShivaGeneratorConfigWidget.h>
#include "PaintDeviceImage.h"
#include "QVariantValue.h"
#include <OpenShiva/Kernel.h>
#include <OpenShiva/Source.h>
#include <OpenShiva/Metadata.h>
#include <GTLCore/Region.h>

#include "Version.h"
#include "UpdaterProgressReport.h"

extern QMutex* shivaMutex;

ShivaFilter::ShivaFilter(OpenShiva::Source* kernel) : KisFilter(KoID(kernel->name().c_str(), kernel->name().c_str()), categoryOther(), kernel->name().c_str()), m_source(kernel)
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(false);
    setSupportsPreview(true);
    setSupportsIncrementalPainting(false);
}

ShivaFilter::~ShivaFilter()
{
}

KisConfigWidget* ShivaFilter::createConfigurationWidget(QWidget* parent, const KisPaintDeviceSP dev, const KisImageWSP image) const
{
    Q_UNUSED(dev);
    Q_UNUSED(image);
    return new ShivaGeneratorConfigWidget(m_source, parent);
}

void ShivaFilter::process(KisConstProcessingInformation srcInfo,
                          KisProcessingInformation dstInfo,
                          const QSize& size,
                          const KisFilterConfiguration* config,
                          KoUpdater* progressUpdater
                         ) const
{
    Q_UNUSED(progressUpdater);
    KisPaintDeviceSP src = srcInfo.paintDevice();
    KisPaintDeviceSP dst = dstInfo.paintDevice();
    QPoint dstTopLeft = dstInfo.topLeft();

    UpdaterProgressReport* report = 0;
    if (progressUpdater) {
        progressUpdater->setRange(0, size.height());
        report = new UpdaterProgressReport(progressUpdater);
    }
    
    Q_ASSERT(!src.isNull());
    Q_ASSERT(!dst.isNull());
//     Q_ASSERT(config);
    // TODO support for selection
    OpenShiva::Kernel kernel;
    kernel.setSource(*m_source);

    if (config) {
        QMap<QString, QVariant> map = config->getProperties();
        for (QMap<QString, QVariant>::iterator it = map.begin(); it != map.end(); ++it) {
            dbgPlugins << it.key() << " " << it.value();
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
        dbgPlugins << "Compile: " << m_source->name().c_str();
        QMutexLocker l(shivaMutex);
        kernel.compile();
    }
    if (kernel.isCompiled()) {
        ConstPaintDeviceImage pdisrc(src);
        PaintDeviceImage pdi(dst);
#if OPENSHIVA_12
        std::list< GTLCore::AbstractImage* > inputs;
        GTLCore::Region region(dstTopLeft.x(), dstTopLeft.y() , size.width(), size.height());
#else
        std::list< const GTLCore::AbstractImage* > inputs;
        GTLCore::RegionI region(dstTopLeft.x(), dstTopLeft.y() , size.width(), size.height());
#endif
        inputs.push_back(&pdisrc);
        dbgPlugins << "Run: " << m_source->name().c_str() << " " <<  dstTopLeft << " " << size;
#if OPENSHIVA_12
        kernel.evaluatePixeles(region, inputs, &pdi, report);
#else
        kernel.evaluatePixels(region, inputs, &pdi, report );
#endif

        }
}
