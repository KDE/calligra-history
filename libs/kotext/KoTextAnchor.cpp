/* This file is part of the KDE project
 * Copyright (C) 2007, 2009-2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010 Ko Gmbh <casper.boemann@kogmbh.com>
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

#include "KoTextAnchor.h"
#include "KoInlineObject_p.h"
#include "KoTextDocumentLayout.h"
#include "KoTextShapeContainerModel.h"
#include "KoTextShapeData.h"
#include "KoStyleStack.h"
#include "KoOdfLoadingContext.h"

#include <KoShapeContainer.h>
#include <KoXmlWriter.h>
#include <KoXmlReader.h>
#include <KoXmlNS.h>
#include <KoShapeSavingContext.h>
#include <KoShapeLoadingContext.h>
#include <KoUnit.h>

#include <QTextInlineObject>
#include <QFontMetricsF>
#include <QPainter>
#include <KDebug>

#include "changetracker/KoChangeTracker.h"
#include "changetracker/KoChangeTrackerElement.h"
#include "styles/KoCharacterStyle.h"
#include "KoTextDocument.h"
#include <KoGenChanges.h>

// #define DEBUG_PAINTING

class KoTextAnchorPrivate : public KoInlineObjectPrivate
{
public:
    KoTextAnchorPrivate(KoTextAnchor *p, KoShape *s)
            : parent(p),
            shape(s),
            document(0),
            position(-1),
            model(0),
            behaveAsCharacter(false),
            verticalPos(KoTextAnchor::VTop),
            verticalRel(KoTextAnchor::VLine),
            horizontalPos(KoTextAnchor::HLeft),
            horizontalRel(KoTextAnchor::HChar),
            anchorType("char")
    {
        Q_ASSERT(shape);
    }

    void relayout()
    {
        if (document && shape->parent()) {
            KoTextShapeData *data  = qobject_cast<KoTextShapeData*>(shape->parent()->userData());
            Q_ASSERT(data);
            data->foul();
            KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(document->documentLayout());
            if (lay)
                lay->interruptLayout();
            data->fireResizeEvent();
        }
    }

    /// as multiple shapes can hold 1 text flow; the anchored shape can be moved between containers and thus models
    void setContainer(KoShapeContainer *container)
    {
        if (container == 0) {
            if (model)
                model->removeAnchor(parent);
            model = 0;
            shape->setParent(0);
            return;
        }
        KoTextShapeContainerModel *theModel = dynamic_cast<KoTextShapeContainerModel*>(container->model());
        if (theModel != model) {
            if (model)
                model->removeAnchor(parent);
            if (shape->parent() != container) {
                if (shape->parent()) {
                    shape->parent()->removeShape(shape);
                }
                container->addShape(shape);
            }
            model = theModel;
            model->addAnchor(parent);
        }
        Q_ASSERT(model == theModel);
    }

    QDebug printDebug(QDebug dbg) const
    {
#ifndef NDEBUG
        dbg.nospace() << "KoTextAnchor";
        dbg.space() << "offset:" << distance;
        dbg.space() << "shape:" << shape->name();
#endif
        return dbg.space();
    }

    KoTextAnchor * const parent;
    KoShape * const shape;
    const QTextDocument *document;
    int position;
    QTextCharFormat format;
    KoTextShapeContainerModel *model;
    QPointF distance;
    bool behaveAsCharacter;
    KoTextAnchor::VerticalPos verticalPos;
    KoTextAnchor::VerticalRel verticalRel;
    KoTextAnchor::HorizontalPos horizontalPos;
    KoTextAnchor::HorizontalRel horizontalRel;
    QString anchorType;
};

KoTextAnchor::KoTextAnchor(KoShape *shape)
    : KoInlineObject(*(new KoTextAnchorPrivate(this, shape)), false)
{
}

KoTextAnchor::~KoTextAnchor()
{
    Q_D(KoTextAnchor);
    if (d->model)
        d->model->removeAnchor(this);
}

KoShape *KoTextAnchor::shape() const
{
    Q_D(const KoTextAnchor);
    return d->shape;
}

void KoTextAnchor::setHorizontalPos(HorizontalPos hp)
{
    Q_D(KoTextAnchor);
    d->horizontalPos = hp;
}

KoTextAnchor::HorizontalPos KoTextAnchor::horizontalPos()
{
    Q_D(const KoTextAnchor);
    return d->horizontalPos;
}

void KoTextAnchor::setHorizontalRel(HorizontalRel hr)
{
    Q_D(KoTextAnchor);
    d->horizontalRel = hr;
}

KoTextAnchor::HorizontalRel KoTextAnchor::horizontalRel()
{
    Q_D(const KoTextAnchor);
    return d->horizontalRel;
}

void KoTextAnchor::setVerticalPos(VerticalPos vp)
{
    Q_D(KoTextAnchor);
    d->verticalPos = vp;
}

KoTextAnchor::VerticalPos KoTextAnchor::verticalPos()
{
    Q_D(const KoTextAnchor);
    return d->verticalPos;
}

void KoTextAnchor::setVerticalRel(VerticalRel vr)
{
    Q_D(KoTextAnchor);
    d->verticalRel = vr;
}

KoTextAnchor::VerticalRel KoTextAnchor::verticalRel()
{
    Q_D(const KoTextAnchor);
    return d->verticalRel;
}

void KoTextAnchor::updatePosition(const QTextDocument *document, QTextInlineObject object, int posInDocument, const QTextCharFormat &format)
{
    Q_UNUSED(object);
    Q_UNUSED(format);
    Q_D(KoTextAnchor);
    d->document = document;
    d->position = posInDocument;
    d->format = format;
    d->setContainer(dynamic_cast<KoShapeContainer*>(shapeForPosition(document, posInDocument)));
}

void KoTextAnchor::resize(const QTextDocument *document, QTextInlineObject object, int posInDocument, const QTextCharFormat &format, QPaintDevice *pd)
{
    Q_UNUSED(document);
    Q_UNUSED(object);
    Q_UNUSED(posInDocument);
    Q_UNUSED(format);
    Q_UNUSED(pd);
    Q_D(KoTextAnchor);

    // important detail; top of anchored shape is at the baseline.
    QFontMetricsF fm(format.font(), pd);
    if (d->behaveAsCharacter == true) {
        d->distance.setX(0);
        object.setWidth(d->shape->size().width());
        if (d->verticalRel == VBaseline) {
            // baseline implies special meaning of the position attribute:
            switch (d->verticalPos) {
            case VFromTop:
                object.setAscent(qMax((qreal) 0, -d->distance.y()));
                object.setDescent(qMax((qreal) 0, d->shape->size().height() + d->distance.y()));
                break;
            case VTop:
                object.setAscent(d->shape->size().height());
                object.setDescent(0);
                break;
            case VMiddle:
                object.setAscent(d->shape->size().height()/2);
                object.setDescent(d->shape->size().height()/2);
                break;
            case VBottom:
                object.setAscent(0);
                object.setDescent(d->shape->size().height());
                break;
            default:
                break;
            }
        } else {
            qreal boundTop = fm.ascent();
            qreal boundBottom = 0;
            if (d->verticalRel == VChar) {
                boundBottom = fm.descent();
            }
            switch (d->verticalPos) {
            case VTop:
                object.setAscent(boundTop);
                object.setDescent(qMax((qreal) 0, d->shape->size().height() - boundTop));
                break;
            case VMiddle:
                object.setAscent(d->shape->size().height()/2);
                object.setDescent(d->shape->size().height()/2);
                break;
            case VBottom:
                object.setAscent(0);
                object.setDescent(d->shape->size().height());
                break;
            default:
                break;
            }
        }
    } else {
        object.setWidth(0);
        object.setAscent(fm.ascent());
        object.setDescent(fm.descent());
    }
}

void KoTextAnchor::paint(QPainter &painter, QPaintDevice *, const QTextDocument *document, const QRectF &rect, QTextInlineObject , int , const QTextCharFormat &)
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);

    // This section of code is to indicate changes done to KoTextAnchors. Once the new approach is complete this can be removed
    // In this approach we draw a rectangle around the shape with the appropriate change indication color.
    Q_D(KoTextAnchor);
    int changeId = d->format.property(KoCharacterStyle::ChangeTrackerId).toInt();
    bool drawChangeRect = false;

    QRectF changeRect = rect;
    changeRect.adjust(0,0,1,0);
    QPen changePen;
    changePen.setWidth(2);

    // we never paint ourselves; the shape can do that.
#ifdef DEBUG_PAINTING
    painter.setOpacity(0.5);
    QRectF charSpace = rect;
    if (charSpace.width() < 10)
        charSpace.adjust(-5, 0, 5, 0);
    painter.fillRect(charSpace, QColor(Qt::green));
#endif

    KoChangeTracker *changeTracker = KoTextDocument(document).changeTracker();
    if (!changeTracker)
        return;

    KoChangeTrackerElement *changeElement = changeTracker->elementById(changeId);
    if (changeElement && changeElement->getChangeType() == KoGenChange::DeleteChange) {
        changePen.setColor(changeTracker->getDeletionBgColor());
        drawChangeRect = true;
    } else if (changeElement && changeElement->getChangeType() == KoGenChange::InsertChange) {
        changePen.setColor(changeTracker->getInsertionBgColor());
        drawChangeRect = true;
    }

    painter.setPen(changePen);
    if (drawChangeRect && changeTracker->displayChanges())
        painter.drawRect(changeRect);

    // End of Change Visualization Section. Can be removed once the new approach is finalized
}

int KoTextAnchor::positionInDocument() const
{
    Q_D(const KoTextAnchor);
    return d->position;
}

const QTextDocument *KoTextAnchor::document() const
{
    Q_D(const KoTextAnchor);
    return d->document;
}

const QPointF &KoTextAnchor::offset() const
{
    Q_D(const KoTextAnchor);
    return d->distance;
}

void KoTextAnchor::setOffset(const QPointF &offset)
{
    Q_D(KoTextAnchor);
    if (d->distance == offset)
        return;
    d->distance = offset;
    d->relayout();
}

void KoTextAnchor::saveOdf(KoShapeSavingContext &context)
{
    Q_D(KoTextAnchor);

    shape()->setAdditionalAttribute("text:anchor-type", d->anchorType);

    // vertical-pos
    switch (d->verticalPos) {
    case VBelow:
        shape()->setAdditionalStyleAttribute("style:vertical-pos", "below");
        break;
    case VBottom:
        shape()->setAdditionalStyleAttribute("style:vertical-pos", "bottom");
        break;
    case VFromTop:
        shape()->setAdditionalStyleAttribute("style:vertical-pos", "from-top");
        break;
    case VMiddle:
        shape()->setAdditionalStyleAttribute("style:vertical-pos", "middle");
        break;
    case VTop:
        shape()->setAdditionalStyleAttribute("style:vertical-pos", "top");
        break;
    default:
        break;
    }

    // vertical-rel
    switch (d->verticalRel) {
    case VBaseline:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "baseline");
        break;
    case VChar:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "char");
        break;
    case VFrame:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "frame");
        break;
    case VFrameContent:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "frame-content");
        break;
    case VLine:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "line");
        break;
    case VPage:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "page");
        break;
    case VPageContent:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "page-content");
        break;
    case VParagraph:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "paragraph");
        break;
    case VParagraphContent:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "paragraph-content");
        break;
    case VText:
        shape()->setAdditionalStyleAttribute("style:vertical-rel", "text");
        break;
    default:
        break;
    }

    // horizontal-pos
    switch (d->horizontalPos) {
    case HCenter:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "center");
        break;
    case HFromInside:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "from-inside");
        break;
    case HFromLeft:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "from-left");
        break;
    case HInside:
        shape()->setAdditionalStyleAttribute("style:horizontal-posl", "inside");
        break;
    case HLeft:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "left");
        break;
    case HOutside:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "outside");
        break;
    case HRight:
        shape()->setAdditionalStyleAttribute("style:horizontal-pos", "right");
        break;
    default:
        break;
    }

    // horizontal-rel
    switch (d->horizontalRel) {
    case HChar:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "char");
        break;
    case HPage:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "page");
        break;
    case HPageContent:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "page-content");
        break;
    case HPageStartMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "page-start-margin");
        break;
    case HPageEndMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "page-end-margin");
        break;
    case HFrame:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "frame");
        break;
    case HFrameContent:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "frame-content");
        break;
    case HFrameEndMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "frame-end-margin");
        break;
    case HFrameStartMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "frame-start-margin");
        break;
    case HParagraph:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "paragraph");
        break;
    case HParagraphContent:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "paragraph-content");
        break;
    case HParagraphEndMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "paragraph-end-margin");
        break;
    case HParagraphStartMargin:
        shape()->setAdditionalStyleAttribute("style:horizontal-rel", "paragraph-start-margin");
        break;
    default:
        break;
    }

    if (shape()->parent()) {// an anchor may not yet have been layout-ed
        QTransform parentMatrix = shape()->parent()->absoluteTransformation(0).inverted();
        QTransform shapeMatrix = shape()->absoluteTransformation(0);;

        qreal dx = d->distance.x() - shapeMatrix.dx()*parentMatrix.m11()
                                   - shapeMatrix.dy()*parentMatrix.m21();
        qreal dy = d->distance.y() - shapeMatrix.dx()*parentMatrix.m12()
                                   - shapeMatrix.dy()*parentMatrix.m22();
        context.addShapeOffset(shape(), QTransform(parentMatrix.m11(),parentMatrix.m12(),
                                                parentMatrix.m21(),parentMatrix.m22(),
                                                dx,dy));
    }

    shape()->saveOdf(context);

    context.removeShapeOffset(shape());
}

bool KoTextAnchor::loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    Q_D(KoTextAnchor);
    d->distance = shape()->position();

    if (! shape()->hasAdditionalAttribute("text:anchor-type"))
        return false;
    d->anchorType = shape()->additionalAttribute("text:anchor-type");

    if (d->anchorType == "as-char")
        d->behaveAsCharacter = true;

    // load settings from graphic style
    KoStyleStack &styleStack = context.odfLoadingContext().styleStack();
    styleStack.save();
    if (element.hasAttributeNS(KoXmlNS::draw, "style-name")) {
        context.odfLoadingContext().fillStyleStack(element, KoXmlNS::draw, "style-name", "graphic");
        styleStack.setTypeProperties("graphic");
    }
    QString verticalPos = styleStack.property(KoXmlNS::style, "vertical-pos");
    QString verticalRel = styleStack.property(KoXmlNS::style, "vertical-rel");
    QString horizontalPos = styleStack.property(KoXmlNS::style, "horizontal-pos");
    QString horizontalRel = styleStack.property(KoXmlNS::style, "horizontal-rel");
    styleStack.restore();

    // vertical-pos
    if (verticalPos == "below") {//svg:y attribute is ignored
        d->verticalPos = VBelow;
        d->distance.setY(0);
    } else if (verticalPos == "bottom") {//svg:y attribute is ignored
        d->verticalPos = VBottom;
        d->distance.setY(-shape()->size().height());
    } else if (verticalPos == "from-top") {
        d->verticalPos = VFromTop;
    } else if (verticalPos == "middle") {//svg:y attribute is ignored
        d->verticalPos = VMiddle;
        d->distance.setY(-(shape()->size().height()/2));
    } else if (verticalPos == "top") {//svg:y attribute is ignored
        d->verticalPos = VTop;
        d->distance.setY(0);
    }

    // vertical-rel
    if (verticalRel == "baseline")
        d->verticalRel = VBaseline;
    else if (verticalRel == "char")
        d->verticalRel = VChar;
    else if (verticalRel == "frame")
        d->verticalRel = VFrame;
    else if (verticalRel == "frame-content")
        d->verticalRel = VFrameContent;
    else if (verticalRel == "line")
        d->verticalRel = VLine;
    else if (verticalRel == "page")
        d->verticalRel = VPage;
    else if (verticalRel == "page-content")
        d->verticalRel = VPageContent;
    else if (verticalRel == "paragraph")
        d->verticalRel = VParagraph;
    else if (verticalRel == "paragraph-content")
        d->verticalRel = VParagraphContent;
    else if (verticalRel == "text")
        d->verticalRel = VText;

    // horizontal-pos
    if (horizontalPos == "center") {//svg:x attribute is ignored
        d->horizontalPos = HCenter;
        d->distance.setX(-(shape()->size().width()/2));
    } else if (horizontalPos == "from-inside") {
        d->horizontalPos = HFromInside;
    } else if (horizontalPos == "from-left") {
        d->horizontalPos = HFromLeft;
    } else if (horizontalPos == "inside") {//svg:x attribute is ignored
        d->horizontalPos = HInside;
        d->distance.setX(0);
    } else if (horizontalPos == "left") {//svg:x attribute is ignored
        d->horizontalPos = HLeft;
        d->distance.setX(0);
    }else if (horizontalPos == "outside") {//svg:x attribute is ignored
        d->horizontalPos = HOutside;
        d->distance.setX(-shape()->size().width());
    }else if (horizontalPos == "right") {//svg:x attribute is ignored
        d->horizontalPos = HRight;
        d->distance.setX(-shape()->size().width());
    }

    // horizontal-rel
    if (horizontalRel == "char")
        d->horizontalRel = HChar;
    else if (horizontalRel == "page")
        d->horizontalRel = HPage;
    else if (horizontalRel == "page-content")
        d->horizontalRel = HPageContent;
    else if (horizontalRel == "page-start-margin")
        d->horizontalRel = HPageStartMargin;
    else if (horizontalRel == "page-end-margin")
        d->horizontalRel = HPageEndMargin;
    else if (horizontalRel == "frame")
        d->horizontalRel = HFrame;
    else if (horizontalRel == "frame-content")
        d->horizontalRel = HFrameContent;
    else if (horizontalRel == "frame-end-margin")
        d->horizontalRel = HFrameEndMargin;
    else if (horizontalRel == "frame-start-margin")
        d->horizontalRel = HFrameStartMargin;
    else if (horizontalRel == "paragraph")
        d->horizontalRel = HParagraph;
    else if (horizontalRel == "paragraph-content")
        d->horizontalRel = HParagraphContent;
    else if (horizontalRel == "paragraph-end-margin")
        d->horizontalRel = HParagraphEndMargin;
    else if (horizontalRel == "paragraph-start-margin")
        d->horizontalRel = HParagraphStartMargin;

    // if svg:x or svg:y should be ignored set new position
    shape()->setPosition(d->distance);

    if (element.hasAttributeNS(KoXmlNS::koffice, "anchor-type")) {
        QString anchorType = element.attributeNS(KoXmlNS::koffice, "anchor-type"); // our enriched properties
        QStringList types = anchorType.split('|');
        if (types.count() > 1) {
            QString vertical = types[0];
            QString horizontal = types[1];
            if (vertical == "TopOfFrame") {
                d->verticalRel = VPageContent;
                d->verticalPos = VTop;
            } else if (vertical == "TopOfParagraph") {
                d->verticalRel = VParagraph;
                d->verticalPos = VTop;
            } else if (vertical == "AboveCurrentLine") {
                d->verticalRel = VLine;
                d->verticalPos = VTop;
            } else if (vertical == "BelowCurrentLine") {
                d->verticalRel = VLine;
                d->verticalPos = VBottom;
            } else if (vertical == "BottomOfParagraph") {
                d->verticalRel = VParagraph;
                d->verticalPos = VBottom;
            } else if (vertical == "BottomOfFrame") {
                d->verticalRel = VPageContent;
                d->verticalPos = VBottom;
            } else if (vertical == "VerticalOffset") {
                d->verticalRel = VLine;
                d->verticalPos = VTop;
            }

            if (horizontal == "Left") {
                d->horizontalRel = HPageContent;
                d->horizontalPos = HLeft;
            } else if (horizontal == "Right") {
                d->horizontalRel = HPageContent;
                d->horizontalPos = HRight;
            } else if (horizontal == "Center") {
                d->horizontalRel = HPageContent;
                d->horizontalPos = HCenter;
            } else if (horizontal == "ClosestToBinding") {
                d->horizontalRel = HPageContent;
                d->horizontalPos = HInside;
            } else if (horizontal == "FurtherFromBinding") {
                d->horizontalRel = HPageContent;
                d->horizontalPos = HOutside;
            } else if (horizontal == "HorizontalOffset") {
                d->horizontalRel = HChar;
                d->horizontalPos = HLeft;
            }
        }
        d->distance = QPointF();
     }

    return true;
}

void KoTextAnchor::setBehavesAsCharacter(bool aschar)
{
    Q_D(KoTextAnchor);
    d->behaveAsCharacter = aschar;
}

bool KoTextAnchor::behavesAsCharacter() const
{
    Q_D(const KoTextAnchor);
    return d->behaveAsCharacter;
}

void KoTextAnchor::detachFromModel()
{
    Q_D(KoTextAnchor);
    d->model = 0;
}
