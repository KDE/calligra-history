/*
 *  Copyright (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_BASE_RECTS_WALKER_H
#define __KIS_BASE_RECTS_WALKER_H

#include <QStack>

#include "kis_layer.h"
#include "kis_mask.h"

class KisBaseRectsWalker;
typedef KisSharedPtr<KisBaseRectsWalker> KisBaseRectsWalkerSP;

class KRITAIMAGE_EXPORT KisBaseRectsWalker : public KisShared
{
public:
    typedef qint32 NodePosition;
    enum NodePositionValues {
        /**
         * There are two different sets of values.
         * The first describes the position of the node to the graph,
         * the second shows the position to the filthy node
         */

        N_NORMAL     = 0x00,
        N_TOPMOST    = 0x01,
        N_BOTTOMMOST = 0x02,

        N_ABOVE_FILTHY = 0x04,
        N_FILTHY_ORIGINAL   = 0x08, // not used actually
        N_FILTHY_PROJECTION = 0x10,
        N_FILTHY = 0x20,
        N_BELOW_FILTHY = 0x40
    };

    #define GRAPH_POSITION_MASK     0x03
    #define POSITION_TO_FILTHY_MASK 0x7C

    struct JobItem {
        KisNodeSP m_node;
        NodePosition m_position;

        /**
         * The rect that should be prepared on this node.
         * E.g. area where the filter applies on filter layer
         * or an area of a paint layer that will be copied to
         * the projection.
         */
        QRect m_applyRect;
    };

    typedef QStack<JobItem> NodeStack;

public:
    KisBaseRectsWalker(QRect cropRect) {
        setCropRect(cropRect);
    }

    virtual ~KisBaseRectsWalker() {
    }

    void collectRects(KisNodeSP node, const QRect& requestedRect) {
        clear();
        m_nodeChecksum = calculateChecksum(node, requestedRect);
        m_resultChangeRect = requestedRect;
        m_requestedRect = requestedRect;
        m_startNode = node;
        startTrip(node);
    }

    inline void recalculate(const QRect& requestedRect) {
        Q_ASSERT(m_startNode);
        collectRects(m_startNode, requestedRect);
    }

    bool checksumValid() {
        Q_ASSERT(m_startNode);
        return m_nodeChecksum == calculateChecksum(m_startNode, m_requestedRect);
    }

    inline void setCropRect(QRect cropRect) {
        m_cropRect = cropRect;
    }

    inline QRect cropRect() const{
        return m_cropRect;
    }

    // return a reference for efficiency reasons
    inline NodeStack& nodeStack() {
        return m_mergeTask;
    }

    inline const QRect& accessRect() const {
        return m_resultAccessRect;
    }

    inline const QRect& changeRect() const {
        return m_resultChangeRect;
    }

    inline bool needRectVaries() const {
        return m_needRectVaries;
    }

    inline bool changeRectVaries() const {
        return m_changeRectVaries;
    }

    inline KisNodeSP startNode() const {
        return m_startNode;
    }

    inline const QRect& requestedRect() const {
        return m_requestedRect;
    }

protected:

    /**
     * Initiates collecting of rects.
     * Should be implemented in derived classes
     */
    virtual void startTrip(KisNodeSP startWith) = 0;

protected:
    static inline KisNode::PositionToFilthy getPositionToFilthy(qint32 position) {
        qint32 positionToFilthy = position & POSITION_TO_FILTHY_MASK;
        // We do not use N_FILTHY_ORIGINAL yet, so...
        Q_ASSERT(!(positionToFilthy & N_FILTHY_ORIGINAL));

        return static_cast<KisNode::PositionToFilthy>(positionToFilthy);
    }

    static inline qint32 getGraphPosition(qint32 position) {
        return position & GRAPH_POSITION_MASK;
    }

    static inline bool isLayer(KisNodeSP node) {
        return qobject_cast<KisLayer*>(node.data());
    }

    static inline bool isMask(KisNodeSP node) {
        return qobject_cast<KisMask*>(node.data());
    }

    inline void clear() {
        m_resultAccessRect = /*m_resultChangeRect =*/
            m_childNeedRect = m_lastNeedRect = QRect();

        m_needRectVaries = m_changeRectVaries = false;
        m_mergeTask.clear();

        // Not needed really. Think over removing.
        //m_startNode = 0;
        //m_requestedRect = QRect();
    }

    inline void pushJob(KisNodeSP node, NodePosition position, QRect applyRect) {
        JobItem item = {node, position, applyRect};
        m_mergeTask.push(item);
    }

    inline QRect cropThisRect(const QRect& rect) {
        return m_cropRect.isValid() ? rect & m_cropRect : rect;
    }

    /**
     * Used by KisFullRefreshWalker as it has a special changeRect strategy
     */
    inline void setExplicitChangeRect(const QRect &changeRect, bool changeRectVaries) {
        m_resultChangeRect = changeRect;
        m_changeRectVaries = changeRectVaries;
    }

    /**
     * Called for every node we meet on a forward way of the trip.
     */
    virtual void registerChangeRect(KisNodeSP node, NodePosition position) {
        // We do not work with masks here. It is KisLayer's job.
        if(!isLayer(node)) return;

        QRect currentChangeRect = node->changeRect(m_resultChangeRect,
                                                   getPositionToFilthy(position));
        currentChangeRect = cropThisRect(currentChangeRect);

        if(!m_changeRectVaries)
            m_changeRectVaries = currentChangeRect != m_resultChangeRect;

        m_resultChangeRect = currentChangeRect;
    }

    /**
     * Called for every node we meet on a backward way of the trip.
     */
    virtual void registerNeedRect(KisNodeSP node, NodePosition position) {
        // We do not work with masks here. It is KisLayer's job.
        if(!isLayer(node)) return;

        if(m_mergeTask.isEmpty())
            m_resultAccessRect = m_childNeedRect =
                m_lastNeedRect = m_resultChangeRect;

        QRect currentNeedRect;

        if(position & N_TOPMOST)
            m_lastNeedRect = m_childNeedRect;

        if(position & (N_FILTHY | N_ABOVE_FILTHY)) {
            if(!m_lastNeedRect.isEmpty())
                pushJob(node, position, m_lastNeedRect);
            //else /* Why push empty rect? */;

            m_lastNeedRect = node->needRect(m_lastNeedRect,
                                            getPositionToFilthy(position));
            m_lastNeedRect = cropThisRect(m_lastNeedRect);
            m_childNeedRect = m_lastNeedRect;
        }
        else if(position & (N_BELOW_FILTHY | N_FILTHY_PROJECTION)) {
            if(!m_lastNeedRect.isEmpty()) {
                pushJob(node, position, m_lastNeedRect);
                m_lastNeedRect = node->needRect(m_lastNeedRect,
                                                getPositionToFilthy(position));
                m_lastNeedRect = cropThisRect(m_lastNeedRect);
            }
        }
        else {
            // N_FILTHY_ORIGINAL is not used so it goes there
            qFatal("KisBaseRectsWalker: node position(%d) is out of range", position);
        }

        if(!m_needRectVaries)
            m_needRectVaries = m_resultAccessRect != m_lastNeedRect;
        m_resultAccessRect |= m_lastNeedRect;
    }

    virtual void adjustMasksChangeRect(KisNodeSP firstMask) {
        KisNodeSP currentNode = firstMask;

        while (currentNode) {
            /**
             * ATTENTION: we miss the first mask
             */

            do {
                currentNode = currentNode->nextSibling();
            } while (currentNode &&
                     (!isMask(currentNode) || !currentNode->visible()));

            if(currentNode) {
                QRect changeRect = currentNode->changeRect(m_resultChangeRect);
                m_changeRectVaries |= changeRect != m_resultChangeRect;
                m_resultChangeRect = changeRect;
            }
        }
    }

    static qint32 calculateChecksum(KisNodeSP node, const QRect &requestedRect) {
        qint32 checksum = 0;
        qint32 x, y, w, h;
        QRect tempRect;

        tempRect = node->changeRect(requestedRect);
        tempRect.getRect(&x, &y, &w, &h);
        checksum += -x - y + w + h;

        tempRect = node->needRect(requestedRect);
        tempRect.getRect(&x, &y, &w, &h);
        checksum += -x - y + w + h;

//        qCritical() << node << requestedRect << "-->" << checksum;

        return checksum;
    }

private:
    /**
     * The result variables.
     * By the end of a recursion they will store a complete
     * data for a successful merge operation.
     */
    QRect m_resultAccessRect;
    QRect m_resultChangeRect;
    bool m_needRectVaries;
    bool m_changeRectVaries;
    NodeStack m_mergeTask;

    /**
     * Used by update optimization framework
     */
    KisNodeSP m_startNode;
    QRect m_requestedRect;

    /**
     * Used for getting know whether the start node
     * properties have changed since the walker was
     * calculated
     */
    qint32 m_nodeChecksum;

    /**
     * Temporary variables
     */
    QRect m_cropRect;

    QRect m_childNeedRect;
    QRect m_lastNeedRect;
};

#endif /* __KIS_BASE_RECTS_WALKER_H */

