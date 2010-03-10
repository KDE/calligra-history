/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#include "kis_image_node_to_bottom_command.h"
#include <QString>

#include <klocale.h>

#include "kis_image.h"
#include "kis_undo_adapter.h"
#include "../kis_image.h"

KisImageNodeToBottomCommand::KisImageNodeToBottomCommand(KisImageWSP image, KisNodeSP node)
        : KisImageCommand(i18n("Lower"), image), m_node(node)
{
    m_prevParent = m_node->parent();
    m_prevAbove = m_node->prevSibling();
}

void KisImageNodeToBottomCommand::redo()
{
    m_image->lock();
    m_image->toBottom(m_node);
    m_image->unlock();

    m_image->refreshGraph(m_prevParent);
    m_prevParent->setDirty(m_image->bounds());
}

void KisImageNodeToBottomCommand::undo()
{
    m_image->lock();
    m_image->moveNode(m_node, m_prevParent, m_prevAbove);
    m_image->unlock();

    m_image->refreshGraph(m_prevParent);
    m_prevParent->setDirty(m_image->bounds());
}
