/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include "kis_selected_transaction_data.h"

#include "kis_selection.h"
#include "kis_pixel_selection.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_undo_adapter.h"

KisSelectedTransactionData::KisSelectedTransactionData(const QString& name, KisNodeSP node, QUndoCommand* parent)
        : KisTransactionData(name, node->paintDevice(), parent)
        , m_selTransaction(0)
        , m_hadSelection(false /*device->hasSelection()*/)
{
    m_layer = dynamic_cast<KisLayer*>(node.data());
    while (!m_layer && node->parent()) {
        m_layer = dynamic_cast<KisLayer*>(node->parent().data());
        node = node->parent();
    }

    if (m_layer->selection())
        m_selTransaction = new KisTransactionData(name, KisPaintDeviceSP(m_layer->selection()->getOrCreatePixelSelection().data()));
//     if(! m_hadSelection) {
//         m_device->deselect(); // let us not be the cause of select
//     }
}

KisSelectedTransactionData::~KisSelectedTransactionData()
{
    delete m_selTransaction;
}

void KisSelectedTransactionData::redo()
{
    // Both transactions will block on the first redo
    KisTransactionData::redo();

    if (m_selTransaction)
        m_selTransaction->redo();
//     if(m_redoHasSelection)
//         m_device->selection();
//     else
//         m_device->deselect();

    if (m_layer->selection()) {
        m_layer->selection()->updateProjection();
    }

    m_layer->setDirty(m_layer->image()->bounds());
    m_layer->image()->undoAdapter()->emitSelectionChanged();
}

void KisSelectedTransactionData::undo()
{
//     m_redoHasSelection = m_device->hasSelection();

    KisTransactionData::undo();

    if (m_selTransaction)
        m_selTransaction->undo();
//     if(m_hadSelection)
//         m_device->selection();
//     else
//         m_device->deselect();
//
    if (m_layer->selection()) {
        m_layer->selection()->updateProjection();
    }

    m_layer->setDirty(m_layer->image()->bounds());
    m_layer->image()->undoAdapter()->emitSelectionChanged();
}

void KisSelectedTransactionData::undoNoUpdate()
{
//     m_redoHasSelection = m_device->hasSelection();
//
//     KisTransactionData::undoNoUpdate();
//     m_selTransaction->undoNoUpdate();
//     if(m_hadSelection)
//         m_device->selection();
//     else
//         m_device->deselect();
}

void KisSelectedTransactionData::endTransaction()
{
    KisTransactionData::endTransaction();
    if(m_selTransaction)
        m_selTransaction->endTransaction();
}

KisLayerSP KisSelectedTransactionData::layer()
{
    return m_layer;
}
