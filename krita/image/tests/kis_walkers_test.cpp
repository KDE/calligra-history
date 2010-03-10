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

#include "kis_walkers_test.h"

#include "kis_base_rects_walker.h"
#include "kis_merge_walker.h"
#include "kis_full_refresh_walker.h"

#include <qtest_kde.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpace.h>
#include "kis_image.h"
#include "kis_paint_layer.h"
#include "kis_group_layer.h"
#include "kis_adjustment_layer.h"
#include "kis_selection.h"

#define DEBUG_VISITORS

QString nodeTypeString(KisMergeWalker::NodePosition position);
QString nodeTypePostfix(KisMergeWalker::NodePosition position);

/************** Test Implementation Of A Walker *********************/
class KisTestWalker : public KisMergeWalker
{
public:
    KisTestWalker()
        :KisMergeWalker(QRect())
    {
    }

    QStringList popResult() {
        QStringList order(m_order);
        m_order.clear();
        return order;
    }

    using KisMergeWalker::startTrip;

protected:

    void registerChangeRect(KisNodeSP node) {
#ifdef DEBUG_VISITORS
        qDebug()<< "FW:"<< node->name();
#endif
        m_order.append(node->name());
    }

    void registerNeedRect(KisNodeSP node, NodePosition position) {
#ifdef DEBUG_VISITORS
        qDebug()<< "BW:"<< node->name() <<'\t'<< nodeTypeString(position);
#endif
        m_order.append(node->name() + nodeTypePostfix(position));
    }

protected:
    QStringList m_order;
};

/************** Debug And Verify Code *******************************/
void reportStartWith(QString nodeName)
{
    qDebug();
    qDebug() << "Start with:" << nodeName;
}

QString nodeTypeString(KisMergeWalker::NodePosition position)
{
    QString string;

    if(position & KisMergeWalker::N_TOPMOST)
        string="TOP";
    else if(position & KisMergeWalker::N_BOTTOMMOST)
        string="BOT";
    else
        string="NOR";

    if(position & KisMergeWalker::N_ABOVE_FILTHY)
        string+="_ABOVE ";
    else if(position & KisMergeWalker::N_FILTHY)
        string+="_FILTH*";
    else if(position & KisMergeWalker::N_FILTHY_PROJECTION)
        string+="_PROJE*";
    else if(position & KisMergeWalker::N_FILTHY_ORIGINAL)
        string+="_ORIGI*_WARNINIG!!!: NOT USED";
    else if(position & KisMergeWalker::N_BELOW_FILTHY)
        string+="_BELOW ";
    else
        qFatal("Impossible happened");

    return string;
}

QString nodeTypePostfix(KisMergeWalker::NodePosition position)
{
    QString string="_";

    if(position & KisMergeWalker::N_TOPMOST)
        string+="T";
    else if(position & KisMergeWalker::N_BOTTOMMOST)
        string+="B";
    else
        string+="N";

    if(position & KisMergeWalker::N_ABOVE_FILTHY)
        string+="A";
    else if(position & KisMergeWalker::N_FILTHY)
        string+="F";
    else if(position & KisMergeWalker::N_FILTHY_PROJECTION)
        string+="P";
    else if(position & KisMergeWalker::N_FILTHY_ORIGINAL)
        string+="O";
    else if(position & KisMergeWalker::N_BELOW_FILTHY)
        string+="B";
    else
        qFatal("Impossible happened");

    return string;
}

void KisWalkersTest::verifyResult(KisBaseRectsWalker &walker, QStringList reference,
                                  QRect accessRect, bool changeRectVaries,
                                  bool needRectVaries)
{
    KisMergeWalker::NodeStack &list = walker.nodeStack();
    QStringList::const_iterator iter = reference.constBegin();

    if(reference.size() != list.size()) {
        qDebug() << "*** Seems like the walker returned stack of wrong size";
        qDebug() << "*** We are going to crash soon... just wait...";
    }

    foreach(KisMergeWalker::JobItem item, list) {
#ifdef DEBUG_VISITORS
        qDebug() << item.m_node->name() << '\t'
                 << item.m_applyRect << '\t'
                 << nodeTypeString(item.m_position);
#endif

        QVERIFY(item.m_node->name() == *iter);

        iter++;
    }

#ifdef DEBUG_VISITORS
    qDebug() << "Result AR:\t" << walker.accessRect();
#endif

    QVERIFY(walker.accessRect() == accessRect);
    QVERIFY(walker.changeRectVaries() == changeRectVaries);
    QVERIFY(walker.needRectVaries() == needRectVaries);
}


/************** Actual Testing **************************************/

    /*
      +----------+
      |root      |
      | layer 5  |
      | group    |
      |  paint 4 |
      |  paint 3 |
      |  adj     |
      |  paint 2 |
      | paint 1  |
      +----------+
     */

void KisWalkersTest::testUsualVisiting()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "walker test");

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer3 = new KisPaintLayer(image, "paint3", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer4 = new KisPaintLayer(image, "paint4", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer5 = new KisPaintLayer(image, "paint5", OPACITY_OPAQUE_U8);

    KisLayerSP groupLayer = new KisGroupLayer(image, "group", OPACITY_OPAQUE_U8);
    KisLayerSP adjustmentLayer = new KisAdjustmentLayer(image, "adj", 0, 0);


    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(groupLayer, image->rootLayer());
    image->addNode(paintLayer5, image->rootLayer());

    image->addNode(paintLayer2, groupLayer);
    image->addNode(adjustmentLayer, groupLayer);
    image->addNode(paintLayer3, groupLayer);
    image->addNode(paintLayer4, groupLayer);

    KisTestWalker walker;

    {
        QString order("paint3,paint4,group,paint5,root,"
                      "root_TF,paint5_TA,group_NF,paint1_BB,"
                      "paint4_TA,paint3_NF,adj_NB,paint2_BB");
        QStringList orderList = order.split(",");

        reportStartWith("paint3");
        walker.startTrip(paintLayer3);
        QVERIFY(walker.popResult() == orderList);
    }

    {
        QString order("adj,paint3,paint4,group,paint5,root,"
                      "root_TF,paint5_TA,group_NF,paint1_BB,"
                      "paint4_TA,paint3_NA,adj_NF,paint2_BB");
        QStringList orderList = order.split(",");

        reportStartWith("adj");
        walker.startTrip(adjustmentLayer);
        QVERIFY(walker.popResult() == orderList);
    }

    {
        QString order("group,paint5,root,"
                      "root_TF,paint5_TA,group_NF,paint1_BB");
        QStringList orderList = order.split(",");

        reportStartWith("group");
        walker.startTrip(groupLayer);
        QVERIFY(walker.popResult() == orderList);
    }
}

    /*
      +----------+
      |root      |
      | layer 5  |
      | cplx  2  |
      | group    |
      |  paint 4 |
      |  paint 3 |
      |  cplx  1 |
      |  paint 2 |
      | paint 1  |
      +----------+
     */

void KisWalkersTest::testMergeVisiting()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "walker test");

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer3 = new KisPaintLayer(image, "paint3", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer4 = new KisPaintLayer(image, "paint4", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer5 = new KisPaintLayer(image, "paint5", OPACITY_OPAQUE_U8);

    KisLayerSP groupLayer = new KisGroupLayer(image, "group", OPACITY_OPAQUE_U8);
    KisLayerSP complexRectsLayer1 = new ComplexRectsLayer(image, "cplx1", OPACITY_OPAQUE_U8);
    KisLayerSP complexRectsLayer2 = new ComplexRectsLayer(image, "cplx2", OPACITY_OPAQUE_U8);

    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(groupLayer, image->rootLayer());
    image->addNode(complexRectsLayer2, image->rootLayer());
    image->addNode(paintLayer5, image->rootLayer());

    image->addNode(paintLayer2, groupLayer);
    image->addNode(complexRectsLayer1, groupLayer);
    image->addNode(paintLayer3, groupLayer);
    image->addNode(paintLayer4, groupLayer);

    QRect testRect(10,10,10,10);
    // Empty rect to show we don't need any cropping
    QRect cropRect;

    KisMergeWalker walker(cropRect);

    {
        QString order("root,paint5,cplx2,group,paint1,"
                      "paint4,paint3,cplx1,paint2");
        QStringList orderList = order.split(",");
        QRect accessRect(-7,-7,44,44);

        reportStartWith("paint3");
        walker.collectRects(paintLayer3, testRect);
        verifyResult(walker, orderList, accessRect, true, true);
    }

    {
        QString order("root,paint5,cplx2,group,paint1,"
                      "paint4,paint3,cplx1,paint2");
        QStringList orderList = order.split(",");
        QRect accessRect(-10,-10,50,50);

        reportStartWith("paint2");
        walker.collectRects(paintLayer2, testRect);
        verifyResult(walker, orderList, accessRect, true, true);
    }

    {
        QString order("root,paint5,cplx2,group,paint1");
        QStringList orderList = order.split(",");
        QRect accessRect(3,3,24,24);

        reportStartWith("paint5");
        walker.collectRects(paintLayer5, testRect);
        verifyResult(walker, orderList, accessRect, false, true);
    }

    {
        /**
         * Test cropping
         */
        QString order("root,paint5,cplx2,group,paint1,"
                      "paint4,paint3,cplx1,paint2");
        QStringList orderList = order.split(",");
        QRect accessRect(0,0,40,40);

        reportStartWith("paint2 (with cropping)");
        walker.setCropRect(image->bounds());
        walker.collectRects(paintLayer2, testRect);
        walker.setCropRect(cropRect);
        verifyResult(walker, orderList, accessRect, true, true);
    }

}

    /*
      +----------+
      |root      |
      | layer 5  |
      | cplx  2  |
      | group    |
      |  paint 4 |
      |  paint 3 |
      |  cplx  1 |
      |  paint 2 |
      | paint 1  |
      +----------+
     */

void KisWalkersTest::testFullRefreshVisiting()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "walker test");

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer3 = new KisPaintLayer(image, "paint3", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer4 = new KisPaintLayer(image, "paint4", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer5 = new KisPaintLayer(image, "paint5", OPACITY_OPAQUE_U8);

    KisLayerSP groupLayer = new KisGroupLayer(image, "group", OPACITY_OPAQUE_U8);
    KisLayerSP complexRectsLayer1 = new ComplexRectsLayer(image, "cplx1", OPACITY_OPAQUE_U8);
    KisLayerSP complexRectsLayer2 = new ComplexRectsLayer(image, "cplx2", OPACITY_OPAQUE_U8);

    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(groupLayer, image->rootLayer());
    image->addNode(complexRectsLayer2, image->rootLayer());
    image->addNode(paintLayer5, image->rootLayer());

    image->addNode(paintLayer2, groupLayer);
    image->addNode(complexRectsLayer1, groupLayer);
    image->addNode(paintLayer3, groupLayer);
    image->addNode(paintLayer4, groupLayer);

    QRect testRect(10,10,10,10);
    // Empty rect to show we don't need any cropping
    QRect cropRect;

    KisFullRefreshWalker walker(cropRect);

    {
        QString order("root,paint5,cplx2,group,paint1,"
                      "paint4,paint3,cplx1,paint2");
        QStringList orderList = order.split(",");
        QRect accessRect(-4,-4,38,38);

        reportStartWith("root");
        walker.collectRects(image->rootLayer(), testRect);
        verifyResult(walker, orderList, accessRect, false, true);
    }
}

    /*
      +----------+
      |root      |
      | layer 5  |
      | cache1   |
      | group    |
      |  paint 4 |
      |  paint 3 |
      |  paint 2 |
      | paint 1  |
      +----------+
     */

void KisWalkersTest::testCachedVisiting()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "walker test");

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer3 = new KisPaintLayer(image, "paint3", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer4 = new KisPaintLayer(image, "paint4", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer5 = new KisPaintLayer(image, "paint5", OPACITY_OPAQUE_U8);

    KisLayerSP groupLayer = new KisGroupLayer(image, "group", OPACITY_OPAQUE_U8);
    KisLayerSP cacheLayer1 = new CacheLayer(image, "cache1", OPACITY_OPAQUE_U8);

    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(groupLayer, image->rootLayer());
    image->addNode(cacheLayer1, image->rootLayer());
    image->addNode(paintLayer5, image->rootLayer());

    image->addNode(paintLayer2, groupLayer);
    image->addNode(paintLayer3, groupLayer);
    image->addNode(paintLayer4, groupLayer);

    QRect testRect(10,10,10,10);
    // Empty rect to show we don't need any cropping
    QRect cropRect;

    KisMergeWalker walker(cropRect);

    {
        QString order("root,paint5,cache1,group,paint1,"
                      "paint4,paint3,paint2");
        QStringList orderList = order.split(",");
        QRect accessRect(0,0,30,30);

        reportStartWith("paint3");
        walker.collectRects(paintLayer3, testRect);
        verifyResult(walker, orderList, accessRect, true, true);
    }

    {
        QString order("root,paint5,cache1");
        QStringList orderList = order.split(",");
        QRect accessRect(10,10,10,10);

        reportStartWith("paint5");
        walker.collectRects(paintLayer5, testRect);
        verifyResult(walker, orderList, accessRect, false, true);
    }

}

#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "kis_filter_mask.h"
#include "kis_transparency_mask.h"

    /*
      +----------+
      |root      |
      | paint 2  |
      | paint 1  |
      |  fmask2  |
      |  tmask   |
      |  fmask1  |
      +----------+
     */

void KisWalkersTest::testMasksVisiting()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    KisImageSP image = new KisImage(0, 512, 512, colorSpace, "walker test");

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "paint1", OPACITY_OPAQUE_U8);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "paint2", OPACITY_OPAQUE_U8);

    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(paintLayer2, image->rootLayer());

    KisFilterMaskSP filterMask1 = new KisFilterMask();
    KisFilterMaskSP filterMask2 = new KisFilterMask();
    KisTransparencyMaskSP transparencyMask = new KisTransparencyMask();

    KisFilterSP filter = KisFilterRegistry::instance()->value("blur");
    Q_ASSERT(filter);
    KisFilterConfiguration *configuration1 = filter->defaultConfiguration(0);
    KisFilterConfiguration *configuration2 = filter->defaultConfiguration(0);

    filterMask1->setFilter(configuration1);
    filterMask2->setFilter(configuration2);

    QRect selection1(10, 10, 20, 10);
    QRect selection2(30, 15, 10, 10);
    QRect selection3(20, 10, 20, 10);

    filterMask1->select(selection1, MAX_SELECTED);
    transparencyMask->select(selection2, MAX_SELECTED);
    filterMask2->select(selection3, MAX_SELECTED);

    image->addNode(filterMask1, paintLayer1);
    image->addNode(transparencyMask, paintLayer1);
    image->addNode(filterMask2, paintLayer1);

    QRect testRect(5,5,30,30);
    // Empty rect to show we don't need any cropping
    QRect cropRect;

    KisMergeWalker walker(cropRect);
    {
        QString order("root,paint2,paint1");
        QStringList orderList = order.split(",");
        QRect accessRect(0,0,40,40);

        reportStartWith("tmask");
        walker.collectRects(transparencyMask, testRect);
        verifyResult(walker, orderList, accessRect, false, false);
    }

    KisTestWalker twalker;
    {
        QString order("paint2,root,"
                      "root_TF,paint2_TA,paint1_BP");
        QStringList orderList = order.split(",");

        reportStartWith("tmask");
        twalker.startTrip(transparencyMask);
        QCOMPARE(twalker.popResult(), orderList);
    }
}

QTEST_KDEMAIN(KisWalkersTest, NoGUI)
#include "kis_walkers_test.moc"

