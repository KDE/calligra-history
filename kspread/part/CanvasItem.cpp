/* This file is part of the KDE project
   Copyright 2009 Thomas Zander <zander@kde.org>
   Copyright 2006-2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
   Copyright 2006 Robert Knight <robertknight@gmail.com>
   Copyright 2006 Inge Wallin <inge@lysator.liu.se>
   Copyright 1999-2002,2004 Laurent Montel <montel@kde.org>
   Copyright 2002-2005 Ariya Hidayat <ariya@kde.org>
   Copyright 1999-2004 David Faure <faure@kde.org>
   Copyright 2004-2005 Meni Livne <livne@kde.org>
   Copyright 2001-2003 Philipp Mueller <philipp.mueller@gmx.de>
   Copyright 2002-2003 Norbert Andres <nandres@web.de>
   Copyright 2003 Hamish Rodda <rodda@kde.org>
   Copyright 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright 2003 Lukas Tinkl <lukas@kde.org>
   Copyright 2000-2002 Werner Trobin <trobin@kde.org>
   Copyright 2002 Harri Porten <porten@kde.org>
   Copyright 2002 John Dailey <dailey@vt.edu>
   Copyright 2002 Daniel Naber <daniel.naber@t-online.de>
   Copyright 1999-2000 Torben Weis <weis@kde.org>
   Copyright 1999-2000 Stephan Kulow <coolo@kde.org>
   Copyright 2000 Bernd Wuebben <wuebben@kde.org>
   Copyright 2000 Wilco Greven <greven@kde.org>
   Copyright 2000 Simon Hausmann <hausmann@kde.org
   Copyright 1999 Michael Reiher <michael.reiher@gmx.de>
   Copyright 1999 Boris Wedl <boris.wedl@kfunigraz.ac.at>
   Copyright 1999 Reginald Stadlbauer <reggie@kde.org>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Local
#include "CanvasItem.h"

// std
#include <assert.h>
#include <float.h>
#include <stdlib.h>

// Qt
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPoint>
#include <QScrollBar>
#include <QTextStream>
#include <QToolTip>
#include <QWidget>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneDragDropEvent>

// KDE
#include <kcursor.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kmimetype.h>
#include <ksharedptr.h>
#include <kwordwrap.h>
#include <kxmlguifactory.h>

// KOffice
#include <KoCanvasController.h>
#include <KoShapeManager.h>
#include <KoStore.h>
#include <KoToolManager.h>
#include <KoToolProxy.h>
#include <KoXmlWriter.h>
#include <KoZoomHandler.h>
#include <KoPointerEvent.h>
#include <KoShapeController.h>
#include <KoShapeManagerPaintingStrategy.h>
#include <KoResourceManager.h>

// KSpread
#include "CalculationSettings.h"
#include "CellStorage.h"
#include "Doc.h"
#include "Global.h"
#include "HeaderItems.h"
#include "Localization.h"
#include "Map.h"
#include "RowColumnFormat.h"
#include "Sheet.h"
#include "Util.h"
#include "Validity.h"
#include "View.h"

// commands
#include "commands/CopyCommand.h"
#include "commands/DeleteCommand.h"
#include "commands/PasteCommand.h"
#include "commands/StyleCommand.h"

// ui
#include "ui/CellView.h"
#include "ui/Selection.h"
#include "ui/PixmapCachingSheetView.h"
#include "ui/RightToLeftPaintingStrategy.h"

#define MIN_SIZE 10

using namespace KSpread;

class CanvasItem::Private
{
public:
    Selection* selection;
    KoZoomHandler* zoomHandler;
    QHash<const Sheet*, SheetView*> sheetViews;
    Sheet* activeSheet;
    ColumnHeaderItem* columnHeader;
    RowHeaderItem* rowHeader;
};

CanvasItem::CanvasItem(Doc *doc)
        : QGraphicsWidget(0)
        , CanvasBase(doc)
        , d(new Private)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StaticContents);
    //setBackgroundRole(QPalette::Base);

    QGraphicsWidget::setFocusPolicy(Qt::StrongFocus);

    //setMouseTracking(true);
    setAcceptHoverEvents(true);

    installEventFilter(this);   // for TAB key processing, otherwise focus change
    setAcceptDrops(true);
    setAttribute(Qt::WA_InputMethodEnabled, true); // ensure using the InputMethod

    d->rowHeader = 0;
    d->columnHeader = 0;

    d->selection = new Selection(this);

    d->zoomHandler = new KoZoomHandler();
    d->activeSheet = 0;
    setActiveSheet(doc->map()->sheet(0));

    d->selection->setActiveSheet(activeSheet());
    connect(d->selection, SIGNAL(refreshSheetViews()), SLOT(refreshSheetViews()));
    connect(d->selection, SIGNAL(visibleSheetRequested(Sheet*)), this, SLOT(setActiveSheet(Sheet*)));
}

CanvasItem::~CanvasItem()
{
    if (doc()->isReadWrite())
        selection()->emitCloseEditor(true);
    d->selection->emitCloseEditor(false);
    d->selection->endReferenceSelection(false);

    d->activeSheet = 0;

    delete d->selection;
    delete d->zoomHandler;
    delete d;
}

void CanvasItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    KoPointerEvent pev(event, QPointF());
    mousePressed(&pev);
}

void CanvasItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    KoPointerEvent pev(event, QPointF());
    mouseReleased(&pev);
}

void CanvasItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    KoPointerEvent pev(event, QPointF());
    mouseMoved(&pev);
}

void CanvasItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    KoPointerEvent pev(event, QPointF());
    mouseDoubleClicked(&pev);
}

void CanvasItem::paint(QPainter* painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    Q_UNUSED(widget);
    CanvasBase::paint(painter, option->exposedRect);
}

void CanvasItem::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    if (CanvasBase::dragEnter(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void CanvasItem::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    if (CanvasBase::dragMove(event->mimeData(), event->pos())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CanvasItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *)
{
    CanvasBase::dragLeave();
}

void CanvasItem::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (CanvasBase::drop(event->mimeData(), event->pos())) {
        event->setAccepted(true);
    } else {
        event->ignore();
    }
}

Selection* CanvasItem::selection() const
{
    return d->selection;
}

Sheet* CanvasItem::activeSheet() const
{
    return d->activeSheet;
}

KoZoomHandler* CanvasItem::zoomHandler() const
{
    return d->zoomHandler;
}

SheetView* CanvasItem::sheetView(const Sheet* sheet) const
{
    if (!d->sheetViews.contains(sheet)) {
        kDebug(36004) << "Creating SheetView for" << sheet->sheetName();
        d->sheetViews.insert(sheet, new PixmapCachingSheetView(sheet));
        d->sheetViews[ sheet ]->setViewConverter(zoomHandler());
        connect(d->sheetViews[ sheet ], SIGNAL(visibleSizeChanged(const QSizeF&)),
                this, SLOT(setDocumentSize(const QSizeF&)));
        //connect(d->sheetViews[ sheet ], SIGNAL(visibleSizeChanged(const QSizeF&)),
                //d->zoomController, SLOT(setDocumentSize(const QSizeF&)));
        connect(sheet, SIGNAL(visibleSizeChanged()),
                d->sheetViews[ sheet ], SLOT(updateAccessedCellRange()));
    }
    return d->sheetViews[ sheet ];
}

void CanvasItem::refreshSheetViews()
{
    const QList<SheetView*> sheetViews = d->sheetViews.values();
    for (int i = 0; i < sheetViews.count(); ++i) {
        disconnect(sheetViews[i], SIGNAL(visibleSizeChanged(const QSizeF&)),
                   this, SLOT(setDocumentSize(const QSizeF&)));
        //disconnect(sheetViews[i], SIGNAL(visibleSizeChanged(const QSizeF&)),
                   //d->zoomController, SLOT(setDocumentSize(const QSizeF&)));
        disconnect(sheetViews[i]->sheet(), SIGNAL(visibleSizeChanged()),
                   sheetViews[i], SLOT(updateAccessedCellRange()));
    }
    qDeleteAll(d->sheetViews);
    d->sheetViews.clear();
    const QList<Sheet*> sheets = doc()->map()->sheetList();
    for (int i = 0; i < sheets.count(); ++i)
        sheets[i]->cellStorage()->invalidateStyleCache();
}

void CanvasItem::setActiveSheet(Sheet* sheet)
{
    if (sheet == d->activeSheet)
        return;

    if (d->activeSheet != 0 && !d->selection->referenceSelectionMode()) {
        selection()->emitCloseEditor(true);
        //saveCurrentSheetSelection();
    }

    const Sheet* oldSheet = d->activeSheet;
    d->activeSheet = sheet;

    if (d->activeSheet == 0) {
        return;
    }

    // flake
    // Change the active shape controller and its shapes.
    shapeController()->setShapeControllerBase(d->activeSheet);
    shapeManager()->setShapes(d->activeSheet->shapes());
    // Tell the Canvas about the new visible sheet size.
    sheetView(d->activeSheet)->updateAccessedCellRange();

    // If there was no sheet before or the layout directions differ.
    if (!oldSheet || oldSheet->layoutDirection() != d->activeSheet->layoutDirection()) {
        // Propagate the layout direction to the canvas and horz. scrollbar.
        const Qt::LayoutDirection direction = d->activeSheet->layoutDirection();
        setLayoutDirection(direction);
        // XXX d->horzScrollBar->setLayoutDirection(direction);
        // Replace the painting strategy for painting shapes.
        KoShapeManager *const shapeManager = this->shapeManager();
        KoShapeManagerPaintingStrategy *paintingStrategy = 0;
        if (direction == Qt::LeftToRight) {
            paintingStrategy = new KoShapeManagerPaintingStrategy(shapeManager);
        } else {
            paintingStrategy = new RightToLeftPaintingStrategy(shapeManager, this);
        }
        shapeManager->setPaintingStrategy(paintingStrategy);
    }

    /*
    // Restore the old scrolling offset.
    QMap<Sheet*, QPointF>::Iterator it3 = d->savedOffsets.find(d->activeSheet);
    if (it3 != d->savedOffsets.end()) {
        const QPoint offset = zoomHandler()->documentToView(*it3).toPoint();
        d->canvas->setDocumentOffset(offset);
        d->horzScrollBar->setValue(offset.x());
        d->vertScrollBar->setValue(offset.y());
    }*/

    // tell the resource manager of the newly active page
    resourceManager()->setResource(KoCanvasResource::CurrentPage, QVariant(sheet->map()->indexOf(sheet) + 1));

    // Always repaint the visible cells.
    update();
    if (d->rowHeader) d->rowHeader->update();
    if (d->columnHeader) d->columnHeader->update();
    //d->selectAllButton->update();

    if (d->selection->referenceSelectionMode()) {
        d->selection->setActiveSheet(d->activeSheet);
        return;
    }

#if 0
    /* see if there was a previous selection on this other sheet */
    QMap<Sheet*, QPoint>::Iterator it = d->savedAnchors.find(d->activeSheet);
    QMap<Sheet*, QPoint>::Iterator it2 = d->savedMarkers.find(d->activeSheet);

    // restore the old anchor and marker
    const QPoint newAnchor = (it == d->savedAnchors.end()) ? QPoint(1, 1) : *it;
    const QPoint newMarker = (it2 == d->savedMarkers.end()) ? QPoint(1, 1) : *it2;

#endif
    d->selection->clear();
    d->selection->setActiveSheet(d->activeSheet);
    d->selection->setOriginSheet(d->activeSheet);
    //d->selection->initialize(QRect(newMarker, newAnchor));

    // Auto calculation state for the INFO function.
    const bool autoCalc = d->activeSheet->isAutoCalculationEnabled();
    doc()->map()->calculationSettings()->setAutoCalculationEnabled(autoCalc);
}

ColumnHeader* CanvasItem::columnHeader() const
{
    if (!d->columnHeader)
        d->columnHeader = new ColumnHeaderItem(0, const_cast<CanvasItem*>(this));
    return d->columnHeader;
}

RowHeader* CanvasItem::rowHeader() const
{
    if (!d->rowHeader)
        d->rowHeader = new RowHeaderItem(0, const_cast<CanvasItem*>(this));
    return d->rowHeader;
}

void CanvasItem::setCursor(const QCursor &cursor)
{
    QGraphicsWidget::setCursor(cursor);
}

#include "CanvasItem.moc"
