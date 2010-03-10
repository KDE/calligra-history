/* Copyright (c) Dmitry Kazakov <dimula73@gmail.com>, 2009
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

#ifndef __KIS_ASYNC_MERGER_H
#define __KIS_ASYNC_MERGER_H

#include <QDebug>

//#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoUpdater.h>

#include "kis_types.h"
#include "kis_paint_device.h"
#include "kis_node_visitor.h"
#include "kis_painter.h"
#include "kis_layer.h"
#include "kis_group_layer.h"
#include "kis_adjustment_layer.h"
#include "generator/kis_generator_layer.h"
#include "kis_external_layer_iface.h"
#include "kis_paint_layer.h"
#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "kis_selection.h"
#include "kis_transaction.h"
//#include "kis_iterators_pixel.h"
#include "kis_clone_layer.h"
#include "kis_processing_information.h"
//#include "kis_node.h"
//#include "kis_projection.h"
#include "kis_node_progress_proxy.h"


#include "kis_merge_walker.h"


//#define DEBUG_MERGER

#ifdef DEBUG_MERGER
#define DEBUG_NODE_ACTION(message, type, node, rect)            \
    qDebug() << message << type << ":" << node->name() << rect
#else
#define DEBUG_NODE_ACTION(message, type, node, rect)
#endif


class KisUpdateOriginalVisitor : public KisNodeVisitor
{
public:
    KisUpdateOriginalVisitor(QRect updateRect, KisPaintDeviceSP projection)
        : m_updateRect(updateRect), m_projection(projection)
    {
    }

    ~KisUpdateOriginalVisitor() {
    }

public:
    using KisNodeVisitor::visit;

    bool visit(KisAdjustmentLayer* layer) {
        Q_ASSERT(m_projection);
        if (!layer->visible()) return true;

        KisFilterConfiguration *filterConfig = layer->filter();
        if (!filterConfig) return true;

        KisFilterSP filter = KisFilterRegistry::instance()->value(filterConfig->name());
        if (!filter) return false;

        KisPaintDeviceSP originalDevice = layer->original();
        originalDevice->clear(m_updateRect);

        if(!m_projection) return true;
        QRect applyRect = m_updateRect & m_projection->extent();

        /**
         * FIXME: check whether it's right to leave a selection to
         * a projection mechanism, not for the filter
         */
        KisConstProcessingInformation srcCfg(m_projection, applyRect.topLeft(), 0);
        KisProcessingInformation dstCfg(originalDevice, applyRect.topLeft(), 0);

        Q_ASSERT(layer->nodeProgressProxy());

        KoProgressUpdater updater(layer->nodeProgressProxy());
        updater.start(100, filter->name());
        QPointer<KoUpdater> updaterPtr = updater.startSubtask();

        KisTransaction* transaction =
            new KisTransaction("", originalDevice);
        filter->process(srcCfg, dstCfg, applyRect.size(),
                        filterConfig, updaterPtr);
        delete transaction;

        updaterPtr->setProgress(100);

        return true;
    }

    bool visit(KisExternalLayer*) {
        return true;
    }

    bool visit(KisGeneratorLayer*) {
        return true;
    }

    bool visit(KisPaintLayer*) {
        return true;
    }

    bool visit(KisGroupLayer*) {
        return true;
    }

    bool visit(KisCloneLayer*) {
        return true;
    }

    bool visit(KisNode*) {
        return true;
    }
    bool visit(KisFilterMask*) {
        return true;
    }
    bool visit(KisTransparencyMask*) {
        return true;
    }
    bool visit(KisTransformationMask*) {
        return true;
    }
    bool visit(KisSelectionMask*) {
        return true;
    }

private:
    QRect m_updateRect;
    KisPaintDeviceSP m_projection;
};


class KisAsyncMerger
{
public:
    KisAsyncMerger() {
    }

    ~KisAsyncMerger() {
    }

    /**
     * FIXME: Check node<->layer transitions
     */

    void startMerge(KisBaseRectsWalker &walker) {
        KisMergeWalker::NodeStack &nodeStack = walker.nodeStack();

        const bool useTempProjections = walker.needRectVaries();

        while(!nodeStack.isEmpty()) {
            KisMergeWalker::JobItem item = nodeStack.pop();
            if(isRootNode(item.m_node)) continue;

            KisLayerSP currentNode = dynamic_cast<KisLayer*>(item.m_node.data());
            // All the masks should be filtered by the walkers
            Q_ASSERT(currentNode);

            QRect applyRect = item.m_applyRect;

            if(!m_currentProjection)
                setupProjection(currentNode, applyRect, useTempProjections);

            KisUpdateOriginalVisitor originalVisitor(applyRect,
                                                     m_currentProjection);

            if(item.m_position & KisMergeWalker::N_FILTHY) {
                DEBUG_NODE_ACTION("Updating", "N_FILTHY", currentNode, applyRect);
                currentNode->accept(originalVisitor);
                currentNode->updateProjection(applyRect);
            }
            else if(item.m_position & KisMergeWalker::N_ABOVE_FILTHY) {
                DEBUG_NODE_ACTION("Updating", "N_ABOVE_FILTHY", currentNode, applyRect);
                currentNode->accept(originalVisitor);
                if(dependOnLowerNodes(currentNode))
                    currentNode->updateProjection(applyRect);
            }
            else if(item.m_position & KisMergeWalker::N_FILTHY_PROJECTION) {
                DEBUG_NODE_ACTION("Updating", "N_FILTHY_PROJECTION", currentNode, applyRect);
                currentNode->updateProjection(applyRect);
            }
            else /*if(item.m_position & KisMergeWalker::N_BELOW_FILTHY)*/ {
                DEBUG_NODE_ACTION("Updating", "N_BELOW_FILTHY", currentNode, applyRect);
                /* nothing to do */
            }

            compositeWithProjection(currentNode, applyRect);

            if(item.m_position & KisMergeWalker::N_TOPMOST) {
                writeProjection(currentNode, useTempProjections, applyRect);
                resetProjection();
            }
        }
    }

private:
    static inline bool isRootNode(KisNodeSP node) {
        return !node->parent();
    }

    static inline bool dependOnLowerNodes(KisNodeSP node) {
        return qobject_cast<KisAdjustmentLayer*>(node.data());
    }

    void resetProjection() {
        m_currentProjection = 0;
    }

    void setupProjection(KisNodeSP currentNode, const QRect& rect, bool useTempProjection) {
        KisPaintDeviceSP parentOriginal = currentNode->parent()->original();

        if (parentOriginal != currentNode->projection()) {
            if (useTempProjection) {
                m_currentProjection =
                    new KisPaintDevice(parentOriginal->colorSpace());
            }
            else {
                parentOriginal->clear(rect);
                m_currentProjection = parentOriginal;
            }
        }
        else {
            /**
             * It happened so that our parent uses our own projection as
             * its original. It means obligeChild mechanism works.
             * We won't initialise m_currentProjection. This will cause
             * writeProjection() and compositeWithProjection() do nothing
             * when called.
             */
            /* NOP */
        }
    }

    void writeProjection(KisNodeSP topmostNode, bool useTempProjection, QRect rect) {
        Q_UNUSED(useTempProjection);
        if (!m_currentProjection) return;

        KisPaintDeviceSP parentOriginal = topmostNode->parent()->original();

        if(m_currentProjection != parentOriginal) {
            KisPainter gc(parentOriginal);
            gc.setCompositeOp(parentOriginal->colorSpace()->compositeOp(COMPOSITE_COPY));
            gc.bitBlt(rect.topLeft(), m_currentProjection, rect);
        }
        DEBUG_NODE_ACTION("Writing projection", "", topmostNode->parent(), rect);
    }

    bool compositeWithProjection(KisLayerSP layer, const QRect &rect) {

        if (!m_currentProjection) return true;
        if (!layer->visible()) return true;

        KisPaintDeviceSP device = layer->projection();
        if (!device) return true;

        QRect needRect = rect & device->extent();
        if(needRect.isEmpty()) return true;

        KisPainter gc(m_currentProjection);
        gc.setChannelFlags(layer->channelFlags());
        gc.setCompositeOp(layer->compositeOp());
        gc.setOpacity(layer->opacity());
        gc.bitBlt(needRect.topLeft(), device, needRect);

        DEBUG_NODE_ACTION("Compositing projection", "", layer, needRect);
        return true;
    }

private:
    KisPaintDeviceSP m_currentProjection;
};


#endif /* __KIS_ASYNC_MERGER_H */

