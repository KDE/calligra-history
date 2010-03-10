/* This file is part of the KDE project
 *
 * Copyright (C) 2007 Emanuele Tamponi <emanuele@valinor.it>
 * Copyright (C) 2009 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef MIXERTOOL_H_
#define MIXERTOOL_H_

#include <KoToolBase.h>

class KoPointerEvent;
class MixerCanvas;
class QRegion;
class KisPaintInformation;

class MixerTool : public KoToolBase
{
    Q_OBJECT

public:

    enum State {
        MIXING,
        PANNING,
        PICKING,
        HOVER
    };

    MixerTool(MixerCanvas *mixer);
    ~MixerTool();

public:


    void setDirty(const QRegion &region);

    // KoToolBase Implementation.

public slots:

    void setState(State state);

    void setRadius(qreal radius);

    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);

    virtual void deactivate();

    virtual void resourceChanged(int key, const QVariant & res);

public:

    /// reimplemented from superclass
    virtual void paint(QPainter &painter, const KoViewConverter &converter);

    /// reimplemented from superclass
    virtual void mousePressEvent(KoPointerEvent *event);

    /// reimplemented from superclass
    virtual void mouseMoveEvent(KoPointerEvent *event);

    /// reimplemented from superclass
    virtual void mouseReleaseEvent(KoPointerEvent *event);

    /// reimplemented from superclass
    virtual void mouseDoubleClickEvent(KoPointerEvent *) {}

private:

    struct Private;
    Private* const m_d;
};


#endif // MIXERTOOL_H_
