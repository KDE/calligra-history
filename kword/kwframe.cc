/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2000 Reginald Stadlbauer <reggie@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kwdoc.h"
#include "kwview.h"
#include "kwviewmode.h"
#include "kwcanvas.h"
#include "kwcommand.h"
#include "kwframe.h"
#include "defs.h"
#include <koUtils.h>
#include "kwtextframeset.h"
#include "kwtableframeset.h"
#include "kwanchor.h"
#include "resizehandles.h"
#include <kotextobject.h> // for customItemChar!
#include <qpicture.h>
#include <qpopupmenu.h>

#include <kformulacontainer.h>
#include <kformuladocument.h>
#include <kformulaview.h>

#include <kcursor.h>
#include <klocale.h>
#include <kparts/partmanager.h>
#include <kdebug.h>
#include <float.h>
#include "KWordFrameSetIface.h"
#include <dcopobject.h>
#include "KWordTextFrameSetEditIface.h"
#include "KWordFormulaFrameSetIface.h"
#include "KWordFormulaFrameSetEditIface.h"
#include "KWordPictureFrameSetIface.h"
#include "KWordPartFrameSetIface.h"
#include "KWordPartFrameSetEditIface.h"

/******************************************************************/
/* Class: KWFrame                                                 */
/******************************************************************/

KWFrame::KWFrame(KWFrame * frame)
{
    handles.setAutoDelete(true);
    //kdDebug() << "KWFrame::KWFrame this=" << this << " frame=" << frame << endl;
    copySettings( frame );
    m_minFrameHeight=0;
}

KWFrame::KWFrame(KWFrameSet *fs, double left, double top, double width, double height, RunAround _ra, double _gap )
    : KoRect( left, top, width, height ),
      // Initialize member vars here. This ensures they are all initialized, since it's
      // easier to compare this list with the member vars list (compiler ensures order).
      m_sheetSide( AnySide ),
      m_runAround( _ra ),
      m_frameBehavior( AutoCreateNewFrame ),
      m_newFrameBehavior( ( fs && fs->type() == FT_TEXT ) ? Reconnect : NoFollowup ),
      m_runAroundGap( _gap ),
      bleft( 0 ),
      bright( 0 ),
      btop( 0 ),
      bbottom( 0 ),
      m_minFrameHeight( 0 ),
      m_internalY( 0 ),
      m_zOrder( 0 ),
      m_bCopy( false ),
      selected( false ),
      m_backgroundColor( QBrush( QColor() ) ), // valid brush with invalid color ( default )
      brd_left( QColor(), KoBorder::SOLID, 0 ),
      brd_right( QColor(), KoBorder::SOLID, 0 ),
      brd_top( QColor(), KoBorder::SOLID, 0 ),
      brd_bottom( QColor(), KoBorder::SOLID, 0 ),
      handles(),
      m_framesOnTop(),
      m_frameSet( fs )
{
    //kdDebug() << "KWFrame::KWFrame " << this << " left=" << left << " top=" << top << endl;
    handles.setAutoDelete(true);
}

KWFrame::~KWFrame()
{
    //kdDebug() << "KWFrame::~KWFrame " << this << endl;
    if (selected)
        removeResizeHandles();
}

int KWFrame::pageNum() const
{
    Q_ASSERT( m_frameSet );
    if ( !m_frameSet )
        return 0;
    KWDocument *doc = m_frameSet->kWordDocument();
    int page = static_cast<int>(y() / doc->ptPaperHeight());
    return QMIN( page, doc->getPages()-1 );
}

QCursor KWFrame::getMouseCursor( const KoPoint & docPoint, bool table, QCursor defaultCursor )
{
    if ( !selected && !table )
        return defaultCursor;

    double mx = docPoint.x();
    double my = docPoint.y();

    if ( !table ) {
        if ( mx >= x() && my >= y() && mx <= x() + 6 && my <= y() + 6 )
            return Qt::sizeFDiagCursor;
        if ( mx >= x() && my >= y() + height() / 2 - 3 && mx <= x() + 6 && my <= y() + height() / 2 + 3 )
            return Qt::sizeHorCursor;
        if ( mx >= x() && my >= y() + height() - 6 && mx <= x() + 6 && my <= y() + height() )
            return Qt::sizeBDiagCursor;
        if ( mx >= x() + width() / 2 - 3 && my >= y() && mx <= x() + width() / 2 + 3 && my <= y() + 6 )
            return Qt::sizeVerCursor;
        if ( mx >= x() + width() / 2 - 3 && my >= y() + height() - 6 && mx <= x() + width() / 2 + 3 &&
             my <= y() + height() )
            return Qt::sizeVerCursor;
        if ( mx >= x() + width() - 6 && my >= y() && mx <= x() + width() && my <= y() + 6 )
            return Qt::sizeBDiagCursor;
        if ( mx >= x() + width() - 6 && my >= y() + height() / 2 - 3 && mx <= x() + width() &&
             my <= y() + height() / 2 + 3 )
            return Qt::sizeHorCursor;
        if ( mx >= x() + width() - 6 && my >= y() + height() - 6 && mx <= x() + width() && my <= y() + height() )
            return Qt::sizeFDiagCursor;

        //if ( selected )
        //    return Qt::sizeAllCursor;
    } else { // Tables
        // ### TODO move to KWTableFrameSet
        if ( mx >= x() + width() - 6 && my >= y() && mx <= x() + width() && my <= y() + height() )
            return Qt::sizeHorCursor;
        if ( mx >= x() && my >= y() + height() - 6 && mx <= x() + width() && my <= y() + height() )
            return Qt::sizeVerCursor;
        //return Qt::sizeAllCursor;
    }

    return defaultCursor;
}

KWFrame *KWFrame::getCopy() {
    /* returns a deep copy of self */
    return new KWFrame(this);
}

void KWFrame::copySettings(KWFrame *frm)
{
    setRect(frm->x(), frm->y(), frm->width(), frm->height());
    // Keep order identical as member var order (and init in ctor)
    setSheetSide(frm->sheetSide());
    setRunAround(frm->runAround());
    setFrameBehavior(frm->frameBehavior());
    setNewFrameBehavior(frm->newFrameBehavior());
    setRunAroundGap(frm->runAroundGap());
    setBLeft(frm->bLeft());
    setBRight(frm->bRight());
    setBTop(frm->bTop());
    setBBottom(frm->bBottom());
    setMinFrameHeight(frm->minFrameHeight());
    m_internalY = 0; // internal Y is recalculated
    setZOrder(frm->zOrder());
    setCopy(frm->isCopy());
    selected = false; // don't copy this attribute [shouldn't be an attribute of KWFrame]
    setBackgroundColor( frm->backgroundColor() );
    setLeftBorder(frm->leftBorder());
    setRightBorder(frm->rightBorder());
    setTopBorder(frm->topBorder());
    setBottomBorder(frm->bottomBorder());
    setFrameSet( frm->frameSet() );
}

// Insert all resize handles
void KWFrame::createResizeHandles() {
    removeResizeHandles();
    QPtrList <KWView> pages = frameSet()->kWordDocument()->getAllViews();
    for (int i=pages.count() -1; i >= 0; i--)
        createResizeHandlesForPage(pages.at(i)->getGUI()->canvasWidget());
}

// Insert 8 resize handles which will be drawn in param canvas
void KWFrame::createResizeHandlesForPage(KWCanvas *canvas) {
    removeResizeHandlesForPage(canvas);

    for (unsigned int i=0; i < 8; i++) {
        KWResizeHandle * h = new KWResizeHandle( canvas, (KWResizeHandle::Direction)i, this );
        handles.append( h );
    }
}

// remove all the resize handles which will be drawn in param canvas
void KWFrame::removeResizeHandlesForPage(KWCanvas *canvas) {
    for( unsigned int i=0; i < handles.count(); i++) {
        if(handles.at ( i )->getCanvas() == canvas) {
            handles.remove(i--);
        }
    }
}

// remove all resizeHandles
void KWFrame::removeResizeHandles() {
    //kdDebug() << this << " KWFrame::removeResizeHandles " << handles.count() << " handles" << endl;
    handles.clear();
}

// move the resizehandles to current location of frame
void KWFrame::updateResizeHandles() {
    for (unsigned int i=0; i< handles.count(); i++) {
        handles.at(i)->updateGeometry();
    }
}


void KWFrame::updateRulerHandles(){
    if(isSelected())
        updateResizeHandles();
    else
    {
        KWDocument *doc = frameSet()->kWordDocument();
        if(doc)
            doc->updateRulerFrameStartEnd();
    }

}

void KWFrame::setSelected( bool _selected )
{
    //kdDebug() << this << " KWFrame::setSelected " << _selected << endl;
    bool s = selected;
    selected = _selected;
    if ( selected )
        createResizeHandles();
    else if ( s )
        removeResizeHandles();
}

QRect KWFrame::outerRect() const
{
    KWDocument *doc = frameSet()->kWordDocument();
    QRect outerRect( doc->zoomRect( *this ) );
    if(!(frameSet() && frameSet()->getGroupManager())) {
        outerRect.rLeft() -= KoBorder::zoomWidthX( brd_left.ptWidth, doc, 1 );
        outerRect.rTop() -= KoBorder::zoomWidthY( brd_top.ptWidth, doc, 1 );
        outerRect.rRight() += KoBorder::zoomWidthX( brd_right.ptWidth, doc, 1 );
        outerRect.rBottom() += KoBorder::zoomWidthY( brd_bottom.ptWidth, doc, 1 );
    }
    return outerRect;
}

KoRect KWFrame::outerKoRect() const
{
    KoRect outerRect = *this;
    KWDocument *doc = frameSet()->kWordDocument();
    outerRect.rLeft() -= KoBorder::zoomWidthX( brd_left.ptWidth, doc, 1 ) / doc->zoomedResolutionX();
    outerRect.rTop() -= KoBorder::zoomWidthY( brd_top.ptWidth, doc, 1 ) / doc->zoomedResolutionY();
    outerRect.rRight() += KoBorder::zoomWidthX( brd_right.ptWidth, doc, 1 ) / doc->zoomedResolutionX();
    outerRect.rBottom() += KoBorder::zoomWidthY( brd_bottom.ptWidth, doc, 1 ) / doc->zoomedResolutionY();
    return outerRect;
}

void KWFrame::save( QDomElement &frameElem )
{
    // setAttribute( double ) uses a default precision of 6, and this seems
    // to be 6 digits, even like '123.123' !
    frameElem.setAttribute( "left", QString::number( left(), 'g', DBL_DIG ) );
    frameElem.setAttribute( "top", QString::number( top(), 'g', DBL_DIG ) );
    frameElem.setAttribute( "right", QString::number( right(), 'g', DBL_DIG ) );
    frameElem.setAttribute( "bottom", QString::number( bottom(), 'g', DBL_DIG ) );

    if(runAround()!=RA_NO)
        frameElem.setAttribute( "runaround", static_cast<int>( runAround() ) );

    if(runAroundGap()!=0)
        frameElem.setAttribute( "runaroundGap", runAroundGap() );

    if(leftBorder().ptWidth!=0)
        frameElem.setAttribute( "lWidth", leftBorder().ptWidth );

    if(leftBorder().color.isValid())
    {
        frameElem.setAttribute( "lRed", leftBorder().color.red() );
        frameElem.setAttribute( "lGreen", leftBorder().color.green() );
        frameElem.setAttribute( "lBlue", leftBorder().color.blue() );
    }
    if(leftBorder().style != KoBorder::SOLID)
        frameElem.setAttribute( "lStyle", static_cast<int>( leftBorder().style ) );

    if(rightBorder().ptWidth!=0)
        frameElem.setAttribute( "rWidth", rightBorder().ptWidth );

    if(rightBorder().color.isValid())
    {
        frameElem.setAttribute( "rRed", rightBorder().color.red() );
        frameElem.setAttribute( "rGreen", rightBorder().color.green() );
        frameElem.setAttribute( "rBlue", rightBorder().color.blue() );
    }
    if(rightBorder().style != KoBorder::SOLID)
        frameElem.setAttribute( "rStyle", static_cast<int>( rightBorder().style ) );

    if(topBorder().ptWidth!=0)
        frameElem.setAttribute( "tWidth", topBorder().ptWidth );

    if(topBorder().color.isValid())
    {
        frameElem.setAttribute( "tRed", topBorder().color.red() );
        frameElem.setAttribute( "tGreen", topBorder().color.green() );
        frameElem.setAttribute( "tBlue", topBorder().color.blue() );
    }
    if(topBorder().style != KoBorder::SOLID)
        frameElem.setAttribute( "tStyle", static_cast<int>( topBorder().style ) );

    if(bottomBorder().ptWidth!=0) {
        frameElem.setAttribute( "bWidth", bottomBorder().ptWidth );
    }
    if(bottomBorder().color.isValid()) {
        frameElem.setAttribute( "bRed", bottomBorder().color.red() );
        frameElem.setAttribute( "bGreen", bottomBorder().color.green() );
        frameElem.setAttribute( "bBlue", bottomBorder().color.blue() );
    }
    if(bottomBorder().style != KoBorder::SOLID)
        frameElem.setAttribute( "bStyle", static_cast<int>( bottomBorder().style ) );

    if(backgroundColor().color().isValid())
    {
        frameElem.setAttribute( "bkRed", backgroundColor().color().red() );
        frameElem.setAttribute( "bkGreen", backgroundColor().color().green() );
        frameElem.setAttribute( "bkBlue", backgroundColor().color().blue() );
        frameElem.setAttribute( "bkStyle", (int)backgroundColor().style ());
    }
    if(bLeft() != 0)
        frameElem.setAttribute( "bleftpt", bLeft() );

    if(bRight()!=0)
        frameElem.setAttribute( "brightpt", bRight() );

    if(bTop()!=0)
        frameElem.setAttribute( "btoppt", bTop() );

    if(bBottom()!=0)
        frameElem.setAttribute( "bbottompt", bBottom() );

    if(frameBehavior()!=AutoCreateNewFrame)
        frameElem.setAttribute( "autoCreateNewFrame", static_cast<int>( frameBehavior()) );

    //if(newFrameBehavior()!=Reconnect) // always save this one, since the default value depends on the type of frame, etc.
    frameElem.setAttribute( "newFrameBehavior", static_cast<int>( newFrameBehavior()) );

    //same reason
    frameElem.setAttribute( "copy", static_cast<int>( m_bCopy ) );

    if(sheetSide()!= AnySide)
        frameElem.setAttribute( "sheetSide", static_cast<int>( sheetSide()) );
}

void KWFrame::load( QDomElement &frameElem, bool headerOrFooter, int syntaxVersion )
{
    m_runAround = static_cast<RunAround>( KWDocument::getAttribute( frameElem, "runaround", RA_NO ) );
    m_runAroundGap = ( frameElem.hasAttribute( "runaroundGap" ) )
                          ? frameElem.attribute( "runaroundGap" ).toDouble()
                          : frameElem.attribute( "runaGapPT" ).toDouble();
    m_sheetSide = static_cast<SheetSide>( KWDocument::getAttribute( frameElem, "sheetSide", AnySide ) );
    m_frameBehavior = static_cast<FrameBehavior>( KWDocument::getAttribute( frameElem, "autoCreateNewFrame", AutoCreateNewFrame ) );
    // Old documents had no "NewFrameBehavior" for footers/headers -> default to Copy.
    NewFrameBehavior defaultValue = headerOrFooter ? Copy : Reconnect;
    // for old document we used the American spelling of newFrameBehavior, so this is for backwards compatibility.
    defaultValue = static_cast<NewFrameBehavior>( KWDocument::getAttribute( frameElem, "newFrameBehaviour", defaultValue ) );
    m_newFrameBehavior = static_cast<NewFrameBehavior>( KWDocument::getAttribute( frameElem, "newFrameBehavior", defaultValue ) );

    KoBorder l, r, t, b;
    l.ptWidth = KWDocument::getAttribute( frameElem, "lWidth", 0.0 );
    r.ptWidth = KWDocument::getAttribute( frameElem, "rWidth", 0.0 );
    t.ptWidth = KWDocument::getAttribute( frameElem, "tWidth", 0.0 );
    b.ptWidth = KWDocument::getAttribute( frameElem, "bWidth", 0.0 );
    if ( frameElem.hasAttribute("lRed") )
        l.color.setRgb(
            KWDocument::getAttribute( frameElem, "lRed", 0 ),
            KWDocument::getAttribute( frameElem, "lGreen", 0 ),
            KWDocument::getAttribute( frameElem, "lBlue", 0 ) );
    if ( frameElem.hasAttribute("rRed") )
        r.color.setRgb(
            KWDocument::getAttribute( frameElem, "rRed", 0 ),
            KWDocument::getAttribute( frameElem, "rGreen", 0 ),
            KWDocument::getAttribute( frameElem, "rBlue", 0 ) );
    if ( frameElem.hasAttribute("tRed") )
        t.color.setRgb(
            KWDocument::getAttribute( frameElem, "tRed", 0 ),
            KWDocument::getAttribute( frameElem, "tGreen", 0 ),
            KWDocument::getAttribute( frameElem, "tBlue", 0 ) );
    if ( frameElem.hasAttribute("bRed") )
        b.color.setRgb(
            KWDocument::getAttribute( frameElem, "bRed", 0 ),
            KWDocument::getAttribute( frameElem, "bGreen", 0 ),
            KWDocument::getAttribute( frameElem, "bBlue", 0 ) );
    l.style = static_cast<KoBorder::BorderStyle>( KWDocument::getAttribute( frameElem, "lStyle", KoBorder::SOLID ) );
    r.style = static_cast<KoBorder::BorderStyle>( KWDocument::getAttribute( frameElem, "rStyle", KoBorder::SOLID ) );
    t.style = static_cast<KoBorder::BorderStyle>( KWDocument::getAttribute( frameElem, "tStyle", KoBorder::SOLID ) );
    b.style = static_cast<KoBorder::BorderStyle>( KWDocument::getAttribute( frameElem, "bStyle", KoBorder::SOLID ) );
    QColor c;
    if ( frameElem.hasAttribute("bkRed") )
        c.setRgb(
            KWDocument::getAttribute( frameElem, "bkRed", 0 ),
            KWDocument::getAttribute( frameElem, "bkGreen", 0 ),
            KWDocument::getAttribute( frameElem, "bkBlue", 0 ) );

    if ( syntaxVersion < 2 ) // Activate old "white border == no border" conversion
    {
        if(c==l.color && l.ptWidth==1 && l.style==0 )
            l.ptWidth=0;
        if(c==r.color  && r.ptWidth==1 && r.style==0)
            r.ptWidth=0;
        if(c==t.color && t.ptWidth==1 && t.style==0 )
            t.ptWidth=0;
        if(c==b.color && b.ptWidth==1 && b.style==0 )
            b.ptWidth=0;
    }
    brd_left = l;
    brd_right = r;
    brd_top = t;
    brd_bottom = b;
    m_backgroundColor = QBrush( c );


    if( frameElem.hasAttribute("bkStyle"))
        m_backgroundColor.setStyle (static_cast<Qt::BrushStyle>(KWDocument::getAttribute( frameElem, "bkStyle", Qt::SolidPattern )));

    bleft = frameElem.attribute( "bleftpt" ).toDouble();
    bright = frameElem.attribute( "brightpt" ).toDouble();
    btop = frameElem.attribute( "btoppt" ).toDouble();
    bbottom = frameElem.attribute( "bbottompt" ).toDouble();
    m_bCopy = KWDocument::getAttribute( frameElem, "copy", headerOrFooter /* default to true for h/f */ );
}


bool KWFrame::frameAtPos( QPoint point, bool borderOfFrameOnly) {
    QRect outerRect( outerRect() );
    // Give the user a bit of margin for clicking on it :)
    const int margin = 2;
    outerRect.rLeft() -= margin;
    outerRect.rTop() -= margin;
    outerRect.rRight() += margin;
    outerRect.rBottom() += margin;
    if ( outerRect.contains( point ) ) {
        if(borderOfFrameOnly && frameSet()) {
            QRect innerRect( frameSet()->kWordDocument()->zoomRect( *this ) );
            innerRect.rLeft() += margin;
            innerRect.rTop() += margin;
            innerRect.rRight() -= margin;
            innerRect.rBottom() -= margin;
            return (!innerRect.contains(point) );
        }
        return true;
    }
    return false;
}

/******************************************************************/
/* Class: KWFrameSet                                              */
/******************************************************************/
KWFrameSet::KWFrameSet( KWDocument *doc )
    : m_doc( doc ), frames(), m_framesInPage(), m_firstPage( 0 ), m_emptyList(),
      m_info( FI_BODY ),
      m_current( 0 ), grpMgr( 0L ), m_removeableHeader( false ), m_visible( true ),
      m_anchorTextFs( 0L ), m_currentDrawnCanvas( 0L ), m_dcop( 0L )
{
    // Send our "repaintChanged" signals to the document.
    connect( this, SIGNAL( repaintChanged( KWFrameSet * ) ),
             doc, SLOT( slotRepaintChanged( KWFrameSet * ) ) );
    frames.setAutoDelete( true );
    m_framesInPage.setAutoDelete( true ); // autodelete the lists in the array (not the frames;)
}

KWordFrameSetIface* KWFrameSet::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KWordFrameSetIface( this );

    return m_dcop;
}


KWFrameSet::~KWFrameSet()
{
    delete m_dcop;
}

void KWFrameSet::addFrame( KWFrame *_frame, bool recalc )
{
    if ( frames.findRef( _frame ) != -1 )
        return;

    frames.append( _frame );
    _frame->setFrameSet(this);
    if(recalc)
        updateFrames();
}

void KWFrameSet::delFrame( unsigned int _num )
{
    KWFrame *frm = frames.at( _num );
    Q_ASSERT( frm );
    delFrame(frm,true);
}

void KWFrameSet::delFrame( KWFrame *frm, bool remove )
{
    kdDebug() << "KWFrameSet::delFrame " << frm << " " << remove << endl;
    int _num = frames.findRef( frm );
    Q_ASSERT( _num != -1 );
    if ( _num == -1 )
        return;

    frm->setFrameSet(0L);
    if ( !remove )
    {
        frames.take( _num );
        if (frm->isSelected()) // get rid of the resize handles
            frm->setSelected(false);
    }
    else
        frames.remove( _num );


    updateFrames();
}

void KWFrameSet::deleteAllFrames()
{
    if ( !frames.isEmpty() )
    {
        frames.clear();
        updateFrames();
    }
}

void KWFrameSet::deleteAllCopies()
{
    if ( frames.count() > 1 )
    {
        KWFrame * firstFrame = frames.first()->getCopy();
        frames.clear();
        frames.append( firstFrame );
        updateFrames();
    }
}

void KWFrameSet::createEmptyRegion( const QRect & crect, QRegion & emptyRegion, KWViewMode *viewMode )
{
    int paperHeight = m_doc->paperHeight();
    //kdDebug() << "KWFrameSet::createEmptyRegion " << getName() << endl;
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt )
    {
        QRect outerRect( viewMode->normalToView( frameIt.current()->outerRect() ) );
        //kdDebug() << "KWFrameSet::createEmptyRegion outerRect=" << DEBUGRECT( outerRect ) << " crect=" << DEBUGRECT( crect ) << endl;
        outerRect &= crect; // This is important, to avoid calling subtract with a Y difference > 65536
        if ( !outerRect.isEmpty() )
        {
            emptyRegion = emptyRegion.subtract( outerRect );
            //kdDebug() << "KWFrameSet::createEmptyRegion emptyRegion now: " << endl; DEBUGREGION( emptyRegion );
        }
        if ( crect.bottom() + paperHeight < outerRect.top() )
            return; // Ok, we're far below the crect, abort.
    }
}

void KWFrameSet::drawFrameBorder( QPainter *painter, KWFrame *frame, KWFrame *settingsFrame, const QRect &crect, KWViewMode *viewMode, KWCanvas *canvas )
{
    QRect outerRect( viewMode->normalToView( frame->outerRect() ) );
    //kdDebug(32002) << "KWFrameSet::drawFrameBorder frame: " << frame
    //               << " outerRect: " << DEBUGRECT( outerRect ) << endl;

    if ( !crect.intersects( outerRect ) )
    {
        //kdDebug() << "KWFrameSet::drawFrameBorder no intersection with " << DEBUGRECT(crect) << endl;
        return;
    }

    QRect frameRect( viewMode->normalToView( m_doc->zoomRect(  *frame ) ) );

    painter->save();
    QBrush bgBrush( settingsFrame->backgroundColor() );
    bgBrush.setColor( KWDocument::resolveBgColor( bgBrush.color(), painter ) );
    painter->setBrush( bgBrush );

    // Draw default borders using view settings...
    QPen viewSetting( lightGray ); // TODO use qcolorgroup
    // ...except when printing, or embedded doc, or disabled.
    if ( ( painter->device()->devType() == QInternal::Printer ) ||
         !canvas || !canvas->gui()->getView()->viewFrameBorders() )
    {
        viewSetting.setColor( bgBrush.color() );
    }

    // Draw borders either as the user defined them, or using the view settings.
    // Borders should be drawn _outside_ of the frame area
    // otherwise the frames will erase the border when painting themselves.

    KoBorder::drawBorders( *painter, m_doc, frameRect,
                           settingsFrame->leftBorder(), settingsFrame->rightBorder(),
                           settingsFrame->topBorder(), settingsFrame->bottomBorder(),
                           1, viewSetting );
    painter->restore();
}

void KWFrameSet::setFloating()
{
    // Find main text frame
    QPtrListIterator<KWFrameSet> fit = m_doc->framesetsIterator();
    for ( ; fit.current() ; ++fit )
    {
        KWTextFrameSet * frameSet = dynamic_cast<KWTextFrameSet *>( fit.current() );
        if ( !frameSet || frameSet->frameSetInfo() != FI_BODY )
            continue;

        Qt3::QTextParag* parag = 0L;
        int index = 0;
        KoPoint dPoint( frames.first()->topLeft() );
        kdDebug() << "KWFrameSet::setFloating looking for pos at " << dPoint.x() << " " << dPoint.y() << endl;
        frameSet->findPosition( dPoint, parag, index );
        // Create anchor. TODO: refcount the anchors!
        setAnchored( frameSet, parag->paragId(), index );
        frameSet->layout();
        frames.first()->updateResizeHandles();
        m_doc->frameChanged(  frames.first() );
        return;
    }
}

void KWFrameSet::setAnchored( KWTextFrameSet* textfs, int paragId, int index, bool placeHolderExists /* = false */ )
{
    Q_ASSERT( textfs );
    kdDebug() << "KWFrameSet::setAnchored " << textfs << " " << paragId << " " << index << " " << placeHolderExists << endl;
    if ( isFloating() )
        deleteAnchors();
    m_anchorTextFs = textfs;
    KWTextParag * parag = static_cast<KWTextParag *>( textfs->textDocument()->paragAt( paragId ) );
    Q_ASSERT( parag );
    if ( parag )
        createAnchors( parag, index, placeHolderExists );
}

void KWFrameSet::setAnchored( KWTextFrameSet* textfs )
{
    m_anchorTextFs = textfs;
}

// Find where our anchor is ( if we are anchored ).
// We can't store a pointers to anchors, because over time we might change anchors
// (Especially, undo/redo of insert/delete can reuse an old anchor and forget a newer one etc.)
KWAnchor * KWFrameSet::findAnchor( int frameNum )
{
    Q_ASSERT( m_anchorTextFs );
    QPtrListIterator<Qt3::QTextCustomItem> cit( m_anchorTextFs->textDocument()->allCustomItems() );
    for ( ; cit.current() ; ++cit )
    {
        KWAnchor * anchor = dynamic_cast<KWAnchor *>( cit.current() );
        if ( anchor && !anchor->isDeleted()
             && anchor->frameSet() == this && anchor->frameNum() == frameNum )
                return anchor;
    }
    kdWarning() << "KWFrameSet::findAnchor anchor not found (frameset='" << getName()
                << "' frameNum=" << frameNum << ")" << endl;
    return 0L;
}

void KWFrameSet::setFixed()
{
    kdDebug() << "KWFrameSet::setFixed" << endl;
    if ( isFloating() )
        deleteAnchors();
    m_anchorTextFs = 0L;
}

KWAnchor * KWFrameSet::createAnchor( KoTextDocument *txt, int frameNum )
{
    KWAnchor * anchor = new KWAnchor( txt, this, frameNum );
    return anchor;
}

void KWFrameSet::createAnchors( KWTextParag * parag, int index, bool placeHolderExists /*= false */ /*only used when loading*/ )
{
    kdDebug() << "KWFrameSet::createAnchors" << endl;
    Q_ASSERT( m_anchorTextFs );
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt, ++index )
    {
        //if ( ! frameIt.current()->anchor() )
        {
            // Anchor this frame, after the previous one
            KWAnchor * anchor = createAnchor( m_anchorTextFs->textDocument(), frameFromPtr( frameIt.current() ) );
            if ( !placeHolderExists )
                parag->insert( index, KoTextObject::customItemChar() );
            parag->setCustomItem( index, anchor, 0 );
        }
    }
    parag->setChanged( true );
    emit repaintChanged( m_anchorTextFs );
}

void KWFrameSet::deleteAnchor( KWAnchor * anchor )
{
    // Simple deletion, no undo/redo
    QTextCursor c( m_anchorTextFs->textDocument() );
    c.setParag( anchor->paragraph() );
    c.setIndex( anchor->index() );
    anchor->setDeleted( true ); // this sets m_anchorTextFs to 0L

    static_cast<KWTextParag*>(c.parag())->removeCustomItem(c.index());
    c.remove(); // This deletes the character where the anchor was
    // We don't delete the anchor since it might be in a customitemmap in a text-insert command
    // TODO: refcount the anchors
    c.parag()->setChanged( true );
}

void KWFrameSet::deleteAnchors()
{
    kdDebug() << "KWFrameSet::deleteAnchors" << endl;
    KWTextFrameSet * textfs = m_anchorTextFs;
    Q_ASSERT( textfs );
    if ( !textfs )
        return;
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    int frameNum = 0;
    // At the moment there's only one anchor per frameset
    // With tables the loop below will be wrong anyway...
    //for ( ; frameIt.current(); ++frameIt, ++frameNum )
    {
/*        if ( frameIt.current()->anchor() )
            deleteAnchor( frameIt.current()->anchor() );
        frameIt.current()->setAnchor( 0L );
*/
        KWAnchor * anchor = findAnchor( frameNum );
        deleteAnchor( anchor );
    }
    emit repaintChanged( textfs );
}

void KWFrameSet::moveFloatingFrame( int frameNum, const KoPoint &position )
{
    KWFrame * frame = frames.at( frameNum );
    Q_ASSERT( frame );
    if ( !frame ) return;

    KoPoint pos( position );
    // position includes the border, we need to adjust accordingly
    pos.rx() += frame->leftBorder().ptWidth;
    pos.ry() += frame->topBorder().ptWidth;
    if ( frame->topLeft() != pos )
    {
        kdDebug() << "KWFrameSet::moveFloatingFrame " << pos.x() << "," << pos.y() << endl;
        frame->moveTopLeft( pos );
        kWordDocument()->updateAllFrames();
        if ( frame->isSelected() )
            frame->updateResizeHandles();
    }
}

QSize KWFrameSet::floatingFrameSize( int frameNum )
{
    KWFrame * frame = frames.at( frameNum );
    Q_ASSERT( frame );
    return frame->outerRect().size();
}

KCommand * KWFrameSet::anchoredObjectCreateCommand( int frameNum )
{
    KWFrame * frame = frames.at( frameNum );
    Q_ASSERT( frame );
    return new KWCreateFrameCommand( QString::null, frame );
}

KCommand * KWFrameSet::anchoredObjectDeleteCommand( int frameNum )
{
    KWFrame * frame = frames.at( frameNum );
    Q_ASSERT( frame );
    return new KWDeleteFrameCommand( QString::null, frame );
}

KWFrame * KWFrameSet::frameByBorder( const QPoint & nPoint )
{
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt ) {
        if(frameIt.current()->frameAtPos(nPoint, true))
            return frameIt.current();
    }
    return 0L;
}

KWFrame * KWFrameSet::frameAtPos( double _x, double _y )
{
    KoPoint docPoint( _x, _y );
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt )
        if ( frameIt.current()->contains( docPoint ) )
            return frameIt.current();
    return 0L;
}

KWFrame *KWFrameSet::frame( unsigned int _num )
{
    return frames.at( _num );
}

int KWFrameSet::frameFromPtr( KWFrame *frame )
{
    return frames.findRef( frame );
}

KWFrame * KWFrameSet::settingsFrame(KWFrame* frame)
{
    QPtrListIterator<KWFrame> frameIt( frame->frameSet()->frameIterator() );
    if ( !frame->isCopy() )
        return frame;
    KWFrame* lastRealFrame=0L;
    for ( ; frameIt.current(); ++frameIt )
    {
        KWFrame *curFrame = frameIt.current();
        if( curFrame == frame )
            return lastRealFrame ? lastRealFrame : frame;
        if ( !lastRealFrame || !curFrame->isCopy() )
            lastRealFrame = curFrame;
    }
    return frame; //fallback, should never happen
}

void KWFrameSet::updateFrames()
{
    if ( frames.isEmpty() )
        return; // No frames. This happens when the frameset is deleted (still exists for undo/redo)

    // Not visible ? Don't bother then.
    if ( !isVisible() )
        return;

    //kdDebug() << "KWFrameSet::updateFrames " << this << " " << getName() << endl;

    // For each of our frames, clear old list of frames on top, and grab min/max page nums
    m_firstPage = frames.first()->pageNum(); // we know frames is not empty here
    int lastPage = m_firstPage;
    QPtrListIterator<KWFrame> fIt( frameIterator() );
    for ( ; fIt.current(); ++fIt ) {
        fIt.current()->clearFramesOnTop();
        int pg = fIt.current()->pageNum();
        m_firstPage = QMIN( m_firstPage, pg );
        lastPage = QMAX( lastPage, pg );
    }

    // Prepare the m_framesInPage structure
    int oldSize = m_framesInPage.size();
    m_framesInPage.resize( lastPage - m_firstPage + 1 );
    // Clear the old elements
    int oldElements = QMIN( oldSize, (int)m_framesInPage.size() );
    for ( int i = 0 ; i < oldElements ; ++i )
        m_framesInPage[i]->clear();
    // Initialize the new elements.
    for ( int i = oldElements ; i < (int)m_framesInPage.size() ; ++i )
        m_framesInPage.insert( i, new QPtrList<KWFrame>() );

    // Iterate over frames again, to fill the m_framesInPage array
    fIt.toFirst();
    for ( ; fIt.current(); ++fIt ) {
        int pg = fIt.current()->pageNum();
        Q_ASSERT( pg <= lastPage );
        m_framesInPage[pg - m_firstPage]->append( fIt.current() );
    }

    // Iterate over ALL framesets, to find those which have frames on top of us.
    // We'll use this information in various methods (adjust[LR]Margin, drawContents etc.)
    // So we want it cached.
    QPtrListIterator<KWFrameSet> framesetIt( m_doc->framesetsIterator() );
    // Look for the one that's in the document's list. Table cells aren't.
    KWFrameSet* thisFrameSet = grpMgr ? grpMgr : this;
    bool foundThis = false;
    for (; framesetIt.current(); ++framesetIt )
    {
        KWFrameSet *frameSet = framesetIt.current();
        if ( frameSet == thisFrameSet )
            foundThis = true;

        if ( !frameSet->isVisible() )
            continue;

        // Floating frames are not "on top", they are "inside".
        if ( frameSet->isFloating() )
            continue;

        //kdDebug() << "KWFrameSet::updateFrames considering frameset " << frameSet << endl;

        QPtrListIterator<KWFrame> frameIt( frameSet->frameIterator() );
        for ( ; frameIt.current(); ++frameIt )
        {
            KWFrame *frameMaybeOnTop = frameIt.current();
            // Is this frame over any of our frames ?
            QPtrListIterator<KWFrame> fIt( frameIterator() );
            for ( ; fIt.current(); ++fIt )
            {
                if ( fIt.current() != frameMaybeOnTop ) // Skip identity case ;)
                {
                    // Handle case of z-order equality the same way as the old code worked:
                    // the order in the main frameset list is what counts.
                    if ( ( fIt.current()->zOrder() == frameMaybeOnTop->zOrder() && foundThis )
                         || fIt.current()->zOrder() < frameMaybeOnTop->zOrder() )
                    {
                        KoRect intersect = fIt.current()->intersect( frameMaybeOnTop->outerKoRect() );
                        if( !intersect.isEmpty() )
                        {
                            kdDebug(32002)
                                << "KWFrameSet::updateFrames adding frame "
                                << frameMaybeOnTop << " (zorder: " << frameMaybeOnTop->zOrder() << ")"
                                << " on top of frame " << fIt.current() << " (zorder: " << fIt.current()->zOrder() << ")"
                                << "\n   intersect: " << DEBUGRECT(intersect)
                                << " (zoomed: " << DEBUGRECT( m_doc->zoomRect( intersect ) ) << endl;
                            fIt.current()->addFrameOnTop( frameMaybeOnTop );
                        }
                    }
                }
            }
        }
    }

    if ( isFloating() )
    {
        //kdDebug() << "KWFrameSet::updateFrames " << getName() << " is floating" << endl;
        QPtrListIterator<KWFrame> frameIt = frameIterator();
        int frameNum = 0;
        // At the moment there's only one anchor per frameset
        //for ( ; frameIt.current(); ++frameIt, ++frameNum )
        {
            KWAnchor * anchor = findAnchor( frameNum );
            //KWAnchor * anchor = frameIt.current()->anchor();
            //kdDebug() << "KWFrameSet::updateFrames anchor=" << anchor << endl;
            if ( anchor )
                anchor->resize();
        }
    }
}

const QPtrList<KWFrame> & KWFrameSet::framesInPage( int pageNum ) const
{
    if ( pageNum < m_firstPage || pageNum >= (int)m_framesInPage.size() + m_firstPage )
    {
#ifdef DEBUG_DTI
        kdWarning() << getName() << " framesInPage called for pageNum=" << pageNum << ". "
                    << " Min value: " << m_firstPage
                    << " Max value: " << m_framesInPage.size() + m_firstPage - 1 << endl;
#endif
        return m_emptyList; // QPtrList<KWFrame>() doesn't work, it's a temporary
    }
    return * m_framesInPage[pageNum - m_firstPage];
}

void KWFrameSet::drawContents( QPainter *p, const QRect & crect, QColorGroup &cg,
                               bool onlyChanged, bool resetChanged,
                               KWFrameSetEdit *edit, KWViewMode *viewMode, KWCanvas *canvas )
{
    /* kdDebug(32002) << "KWFrameSet::drawContents " << this << " " << getName()
                   << " onlyChanged=" << onlyChanged << " resetChanged=" << resetChanged
                   << " crect= " << DEBUGRECT(crect)
                   << endl; */
    m_currentDrawnCanvas = canvas;

    QPtrListIterator<KWFrame> frameIt( frameIterator() );
    KWFrame * lastRealFrame = 0L;
    int lastRealFrameTop = 0;
    int totalHeight = 0;
    for ( ; frameIt.current(); ++frameIt )
    {
        KWFrame *frame = frameIt.current();
        if ( !frame->isValid() )
        {
            kdDebug(32002) << "KWFrameSet::drawContents invalid frame " << frame << endl;
            continue;
        }

        QRect r(crect);
        QRect normalFrameRect( m_doc->zoomRect( *frame ) );
        QRect frameRect( viewMode->normalToView( normalFrameRect ) );
        //kdDebug(32002) << "                    frame=" << frame << " crect=" << DEBUGRECT(r) << endl;
        r = r.intersect( frameRect );
        //kdDebug(32002) << "                    framerect=" << DEBUGRECT(*frame) << " intersec=" << DEBUGRECT(r) << " todraw=" << !r.isEmpty() << endl;
        if ( !r.isEmpty() )
        {
            // This translates the coordinates in the document contents
            // ( frame and r are up to here in this system )
            // into the frame's own coordinate system.
            int offsetX = normalFrameRect.left();
            int offsetY = normalFrameRect.top() - ( ( frame->isCopy() && lastRealFrame ) ? lastRealFrameTop : totalHeight );

            QRect fcrect = viewMode->viewToNormal( r );
            //kdDebug() << "KWFrameSet::drawContents crect after view-to-normal:" << DEBUGRECT( fcrect )  << endl;
            // y=-1 means all (in QRT), so let's not go there !
            //QPoint tl( QMAX( 0, fcrect.left() - offsetX ), QMAX( 0, fcrect.top() - offsetY ) );
            //fcrect.moveTopLeft( tl );
            fcrect.moveBy( -offsetX, -offsetY );
            Q_ASSERT( fcrect.x() >= 0 );
            Q_ASSERT( fcrect.y() >= 0 );

            // fcrect is now the portion of the frame to be drawn,
            // in the frame's coordinates and in pixels
            //kdDebug() << "KWFrameSet::drawContents in internal coords:" << DEBUGRECT( fcrect ) << endl;

            // The settings come from this frame
            KWFrame * settingsFrame = ( frame->isCopy() && lastRealFrame ) ? lastRealFrame : frame;

            QRegion reg = frameClipRegion( p, frame, r, viewMode, onlyChanged );
            if ( !reg.isEmpty() )
            {
                p->save();
                p->setClipRegion( reg );
                p->translate( r.x() - fcrect.x(), r.y() - fcrect.y() ); // This assume that viewToNormal() is only a translation
                p->setBrushOrigin( p->brushOrigin() + r.topLeft() - fcrect.topLeft() );

                QBrush bgBrush( settingsFrame->backgroundColor() );
                bgBrush.setColor( KWDocument::resolveBgColor( bgBrush.color(), p ) );
                cg.setBrush( QColorGroup::Base, bgBrush );

                drawFrame( frame, p, fcrect, cg, onlyChanged, resetChanged, edit );

                p->restore();
            }
        }
        if(! getGroupManager()) {
            QRect outerRect( viewMode->normalToView( frame->outerRect() ) );
            r = crect.intersect( outerRect );
            if ( !r.isEmpty() )
            {
                // Now draw the frame border
                // Clip frames on top if onlyChanged, but don't clip to the frame
                QRegion reg = frameClipRegion( p, frame, r, viewMode, onlyChanged, false );
                if ( !reg.isEmpty() )
                {
                    p->save();
                    p->setClipRegion( reg );
                    KWFrame * settingsFrame = ( frame->isCopy() && lastRealFrame ) ? lastRealFrame : frame;
                    drawFrameBorder( p, frame, settingsFrame, r, viewMode, canvas );
                    p->restore();
                }// else kdDebug() << "KWFrameSet::drawContents not drawing border for frame " << frame << endl;
            }
        }

        if ( !lastRealFrame || !frame->isCopy() )
        {
            lastRealFrame = frame;
            lastRealFrameTop = totalHeight;
        }
        totalHeight += normalFrameRect.height();
    }
    m_currentDrawnCanvas = 0L;
}

bool KWFrameSet::contains( double mx, double my )
{
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt )
        if ( frameIt.current()->contains( KoPoint( mx, my ) ) )
            return true;

    return false;
}

bool KWFrameSet::getMouseCursor( const QPoint &nPoint, bool controlPressed, QCursor & cursor )
{
    bool canMove = isMoveable();
    KoPoint docPoint = m_doc->unzoomPoint( nPoint );
    QCursor defaultCursor = ( canMove && !isFloating() ) ? Qt::sizeAllCursor : KCursor::handCursor();
    // See if we're over a frame border
    KWFrame * frame = frameByBorder( nPoint );
    if ( frame )
    {
        cursor = frame->getMouseCursor( docPoint, grpMgr ? true : false, defaultCursor );
        return true;
    }

    frame = frameAtPos( docPoint.x(), docPoint.y() );
    if ( frame == 0L )
        return false;

    if ( controlPressed )
        cursor = defaultCursor;
    else
        cursor = frame->getMouseCursor( docPoint, grpMgr ? true : false, Qt::ibeamCursor );
    return true;
}

void KWFrameSet::saveCommon( QDomElement &parentElem, bool saveFrames )
{
    if ( frames.isEmpty() ) // Deleted frameset -> don't save
        return;

    // Save all the common attributes for framesets.
    parentElem.setAttribute( "frameType", static_cast<int>( type() ) );
    parentElem.setAttribute( "frameInfo", static_cast<int>( m_info ) );
    parentElem.setAttribute( "name", correctQString( m_name ) );
    parentElem.setAttribute( "visible", static_cast<int>( m_visible ) );

    if ( saveFrames )
    {
        QPtrListIterator<KWFrame> frameIt = frameIterator();
        for ( ; frameIt.current(); ++frameIt )
        {
            KWFrame *frame = frameIt.current();
            QDomElement frameElem = parentElem.ownerDocument().createElement( "FRAME" );
            parentElem.appendChild( frameElem );

            frame->save( frameElem );

            if(m_doc->processingType() == KWDocument::WP) {
                // Assume that all header/footer frames in the same frameset are
                // perfect copies. This might not be the case some day though.
                if(frameSetInfo() == FI_FIRST_HEADER ||
                   frameSetInfo() == FI_ODD_HEADER ||
                   frameSetInfo() == FI_EVEN_HEADER ||
                   frameSetInfo() == FI_FIRST_FOOTER ||
                   frameSetInfo() == FI_ODD_FOOTER ||
                   frameSetInfo() == FI_EVEN_FOOTER ||
                   frameSetInfo() == FI_FOOTNOTE) break;
            }
        }
    }
}

//
// This function is intended as a helper for all the derived classes. It reads
// in all the attributes common to all framesets and loads all frames.
//
void KWFrameSet::load( QDomElement &framesetElem, bool loadFrames )
{
    m_info = static_cast<KWFrameSet::Info>( KWDocument::getAttribute( framesetElem, "frameInfo", KWFrameSet::FI_BODY ) );
    m_visible = static_cast<bool>( KWDocument::getAttribute( framesetElem, "visible", true ) );
    if ( framesetElem.hasAttribute( "removeable" ) )
        m_removeableHeader = static_cast<bool>( KWDocument::getAttribute( framesetElem, "removeable", false ) );
    else
        m_removeableHeader = static_cast<bool>( KWDocument::getAttribute( framesetElem, "removable", false ) );

    if ( loadFrames )
    {
        // <FRAME>
        QDomElement frameElem = framesetElem.firstChild().toElement();
        for ( ; !frameElem.isNull() ; frameElem = frameElem.nextSibling().toElement() )
        {
            if ( frameElem.tagName() == "FRAME" )
            {
                KoRect rect;
                rect.setLeft( KWDocument::getAttribute( frameElem, "left", 0.0 ) );
                rect.setTop( KWDocument::getAttribute( frameElem, "top", 0.0 ) );
                rect.setRight( KWDocument::getAttribute( frameElem, "right", 0.0 ) );
                rect.setBottom( KWDocument::getAttribute( frameElem, "bottom", 0.0 ) );
                KWFrame * frame = new KWFrame(this, rect.x(), rect.y(), rect.width(), rect.height() );
                frame->load( frameElem, isHeaderOrFooter(), m_doc->syntaxVersion() );
                addFrame( frame, false );
                m_doc->progressItemLoaded();
            }
        }
    }
}

bool KWFrameSet::hasSelectedFrame()
{
    for ( unsigned int i = 0; i < frames.count(); i++ ) {
        if ( frames.at( i )->isSelected() )
            return true;
    }

    return false;
}

void KWFrameSet::setVisible( bool v )
{
    m_visible = v;
    if ( m_visible )
        // updateFrames was disabled while we were invisible
        updateFrames();
}

bool KWFrameSet::isVisible() const
{
    return ( m_visible &&
             !frames.isEmpty() &&
             (!isAHeader() || m_doc->isHeaderVisible()) &&
             (!isAFooter() || m_doc->isFooterVisible()) &&
             !isAWrongHeader( m_doc->getHeaderType() ) &&
             !isAWrongFooter( m_doc->getFooterType() ) );
}

bool KWFrameSet::isAHeader() const
{
    return ( m_info == FI_FIRST_HEADER || m_info == FI_EVEN_HEADER || m_info == FI_ODD_HEADER );
}

bool KWFrameSet::isAFooter() const
{
    return ( m_info == FI_FIRST_FOOTER || m_info == FI_EVEN_FOOTER || m_info == FI_ODD_FOOTER );
}

bool KWFrameSet::isAWrongHeader( KoHFType t ) const
{
    switch ( m_info ) {
    case FI_FIRST_HEADER: {
        if ( t == HF_FIRST_DIFF ) return false;
        return true;
    } break;
    case FI_EVEN_HEADER: {
        return false;
    } break;
    case FI_ODD_HEADER: {
        if ( t == HF_EO_DIFF ) return false;
        return true;
    } break;
    default: return false;
    }
}

bool KWFrameSet::isAWrongFooter( KoHFType t ) const
{
    switch ( m_info ) {
    case FI_FIRST_FOOTER: {
        if ( t == HF_FIRST_DIFF ) return false;
        return true;
    } break;
    case FI_EVEN_FOOTER: {
        return false;
    } break;
    case FI_ODD_FOOTER: {
        if ( t == HF_EO_DIFF ) return false;
        return true;
    } break;
    default: return false;
    }
}

bool KWFrameSet::isMainFrameset() const
{
    return ( m_doc->processingType() == KWDocument::WP &&
             m_doc->frameSet( 0 ) == this );
}

bool KWFrameSet::isMoveable() const
{
    if ( isHeaderOrFooter() )
        return false;
    return !isMainFrameset() && !isFloating();
}

void KWFrameSet::zoom( bool )
{
}

void KWFrameSet::finalize()
{
    //kdDebug() << "KWFrameSet::finalize ( calls updateFrames + zoom ) " << this << endl;
    updateFrames();
    zoom( false );
}

QRegion KWFrameSet::frameClipRegion( QPainter * painter, KWFrame *frame, const QRect & crect,
                                     KWViewMode * viewMode, bool /*onlyChanged*/, bool clipFrame )
{
    KWDocument * doc = kWordDocument();
    QRect rc = painter->xForm( crect );
    Q_ASSERT( frame );
    if ( clipFrame )
    {
        //kdDebug(32002) << "KWFrameSet::frameClipRegion rc initially " << DEBUGRECT(rc) << endl;
        rc &= painter->xForm( viewMode->normalToView( doc->zoomRect( *frame ) ) ); // intersect
        //kdDebug(32002) << "KWFrameSet::frameClipRegion frame=" << DEBUGRECT(*frame)
        //               << " clip region rect=" << DEBUGRECT(rc)
        //               << " rc.isEmpty()=" << rc.isEmpty() << endl;
    }
    if ( !rc.isEmpty() )
    {
        QRegion reg( rc );
        // This breaks when a frame is under another one, it still appears if !onlyChanged.
        // cvs log says this is about frame borders... hmm.
        /// ### if ( onlyChanged )
        {
            QPtrListIterator<KWFrame> fIt( frame->framesOnTop() );
            for ( ; fIt.current() ; ++fIt )
            {
                QRect r = painter->xForm( viewMode->normalToView( (*fIt)->outerRect() ) );
                //kdDebug(32002) << "frameClipRegion subtract rect "<< DEBUGRECT(r) << endl;
                reg -= r; // subtract
            }
        }

	// clip inline frames against their 'parent frames' (=the frame containing the anchor of the frame.)
	KWFrameSet *parentFrameset= this;
    	KWFrame *parentFrame=frame;
    	while (parentFrameset->isFloating()) {
		parentFrameset=parentFrameset->anchorFrameset();
		parentFrame=parentFrameset->frameAtPos(parentFrame->x(), parentFrame->y());
		QRect r = painter->xForm( viewMode->normalToView( parentFrame->outerRect() ) );
		reg &= r;
    	}


        return reg;
    } else return QRegion();
}

bool KWFrameSet::canRemovePage( int num )
{
    QPtrListIterator<KWFrame> frameIt( frameIterator() );
    for ( ; frameIt.current(); ++frameIt )
    {
        KWFrame * frame = frameIt.current();
        if ( frame->pageNum() == num )
        {
            // Ok, so we have a frame on that page -> we can't remove it unless it's a copied frame
            if ( ! ( frame->isCopy() && frameIt.current() != frames.first() ) )
                return false;
        }
    }
    return true;
}

void KWFrameSet::showPopup( KWFrame *, KWFrameSetEdit *, KWView *view, const QPoint &point )
{
    QPopupMenu * popup = view->popupMenu("frame_popup");
    Q_ASSERT(popup);
    if (popup)
        popup->popup( point );
}

void KWFrameSet::setFrameBehavior( KWFrame::FrameBehavior fb ) {
    for(KWFrame *f=frames.first();f;f=frames.next())
        f->setFrameBehavior(fb);
}

void KWFrameSet::setNewFrameBehavior( KWFrame::NewFrameBehavior nfb ) {
    for(KWFrame *f=frames.first();f;f=frames.next())
        f->setNewFrameBehavior(nfb);
}

void KWFrameSet::resizeFrameSetCoords( KWFrame* frame, double newLeft, double newTop, double newRight, double newBottom, bool finalSize )
{
    frame->setLeft( newLeft );
    frame->setTop( newTop );
    resizeFrame( frame, newRight - newLeft, newBottom - newTop, finalSize );
}

void KWFrameSet::resizeFrame( KWFrame* frame, double newWidth, double newHeight, bool )
{
    frame->setWidth( newWidth );
    frame->setHeight( newHeight );
}


#ifndef NDEBUG
void KWFrameSet::printDebug()
{
    static const char * typeFrameset[] = { "base", "txt", "pic", "part", "formula", "clipart",
                                           "6", "7", "8", "9", "table",
                                           "ERROR" };
    static const char * infoFrameset[] = { "body", "first header", "odd headers", "even headers",
                                           "first footer", "odd footers", "even footers", "footnote", "ERROR" };
    static const char * frameBh[] = { "AutoExtendFrame", "AutoCreateNewFrame", "Ignore", "ERROR" };
    static const char * newFrameBh[] = { "Reconnect", "NoFollowup", "Copy" };
    static const char * runaround[] = { "No Runaround", "Bounding Rect", "Horizontal Space", "ERROR" };

    kdDebug() << " |  Visible: " << isVisible() << endl;
    kdDebug() << " |  Type: " << typeFrameset[ type() ] << endl;
    kdDebug() << " |  Info: " << infoFrameset[ frameSetInfo() ] << endl;
    kdDebug() << " |  Floating: " << isFloating() << endl;

    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( unsigned int j = 0; frameIt.current(); ++frameIt, ++j ) {
        KWFrame * frame = frameIt.current();
        QCString copy = frame->isCopy() ? "[copy]" : "";
        kdDebug() << " +-- Frame " << j << " of "<< getNumFrames() << "    (" << frame << ")  " << copy << endl;
        printDebug( frame );
        kdDebug() << "     Rectangle : " << frame->x() << "," << frame->y() << " " << frame->width() << "x" << frame->height() << endl;
        kdDebug() << "     RunAround: "<< runaround[ frame->runAround() ] << endl;
        kdDebug() << "     FrameBehavior: "<< frameBh[ frame->frameBehavior() ] << endl;
        kdDebug() << "     NewFrameBehavior: "<< newFrameBh[ frame->newFrameBehavior() ] << endl;
        QColor col = frame->backgroundColor().color();
        kdDebug() << "     BackgroundColor: "<< ( col.isValid() ? col.name().latin1() : "(default)" ) << endl;
        kdDebug() << "     SheetSide "<< frame->sheetSide() << endl;
        kdDebug() << "     Z Order: " << frame->zOrder() << endl;
        kdDebug() << "     Number of frames on top: " << frame->framesOnTop().count() << endl;
        kdDebug() << "     minFrameHeight "<< frame->minFrameHeight() << endl;
        if(frame->isSelected())
            kdDebug() << " *   Page "<< frame->pageNum() << endl;
        else
            kdDebug() << "     Page "<< frame->pageNum() << endl;
    }
}

void KWFrameSet::printDebug( KWFrame * )
{
}

#endif

KWFrameSetEdit::KWFrameSetEdit( KWFrameSet * fs, KWCanvas * canvas )
     : m_fs(fs), m_canvas(canvas), m_currentFrame( fs->frame(0) )
{
}

void KWFrameSetEdit::drawContents( QPainter *p, const QRect &crect,
                                   QColorGroup &cg, bool onlyChanged, bool resetChanged,
                                   KWViewMode *viewMode, KWCanvas *canvas )
{
    //kdDebug() << "KWFrameSetEdit::drawContents " << frameSet()->getName() << endl;
    frameSet()->drawContents( p, crect, cg, onlyChanged, resetChanged, this, viewMode, canvas );
}

/******************************************************************/
/* Class: KWPictureFrameSet                                       */
/******************************************************************/
KWPictureFrameSet::KWPictureFrameSet( KWDocument *_doc, const QString & name )
    : KWFrameSet( _doc )
{
    if ( name.isEmpty() )
        m_name = _doc->generateFramesetName( i18n( "Picture %1" ) );
    else
        m_name = name;
    m_keepAspectRatio = true;
}

KWPictureFrameSet::~KWPictureFrameSet() {
}

KWordFrameSetIface* KWPictureFrameSet::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KWordPictureFrameSetIface( this );

    return m_dcop;
}


void KWPictureFrameSet::loadImage( const QString & fileName, const QSize &_imgSize )
{
    KoPictureCollection *collection = m_doc->imageCollection();

    m_image = collection->loadPicture( fileName );

    m_image.setSize( _imgSize );
}

void KWPictureFrameSet::setSize( const QSize & _imgSize )
{
    m_image.setSize( _imgSize );
}

void KWPictureFrameSet::resizeFrame( KWFrame* frame, double newWidth, double newHeight, bool finalSize )
{
    KWFrameSet::resizeFrame( frame, newWidth, newHeight, finalSize );
    QSize newSize = kWordDocument()->zoomSize( frame->size() );
    m_image.setSize( newSize );
    // TODO: !finalSize /* finalSize==false -> fast mode=true */ );
}

QDomElement KWPictureFrameSet::save( QDomElement & parentElem, bool saveFrames )
{
    if ( frames.isEmpty() ) // Deleted frameset -> don't save
        return QDomElement();
    QDomElement framesetElem = parentElem.ownerDocument().createElement( "FRAMESET" );
    parentElem.appendChild( framesetElem );

    KWFrameSet::saveCommon( framesetElem, saveFrames );

    QDomElement imageElem = parentElem.ownerDocument().createElement( "IMAGE" );
    framesetElem.appendChild( imageElem );
    imageElem.setAttribute( "keepAspectRatio", m_keepAspectRatio ? "true" : "false" );
    QDomElement elem = parentElem.ownerDocument().createElement( "KEY" );
    imageElem.appendChild( elem );
    m_image.getKey().saveAttributes( elem );
    return framesetElem;
}

void KWPictureFrameSet::load( QDomElement &attributes, bool loadFrames )
{
    KWFrameSet::load( attributes, loadFrames );

    // <IMAGE>
    QDomElement image = attributes.namedItem( "IMAGE" ).toElement();
    if ( !image.isNull() ) {
        m_keepAspectRatio = image.attribute( "keepAspectRatio", "true" ) == "true";
        // <KEY>
        QDomElement keyElement = image.namedItem( "KEY" ).toElement();
        if ( !keyElement.isNull() )
        {
            KoPictureKey key;
            key.loadAttributes( keyElement, QDate(), QTime() );
            m_image.clear();
            m_image.setKey( key );
            m_doc->addImageRequest( this );
        }
        else
        {
            // <FILENAME> (old format, up to KWord-1.1-beta2)
            QDomElement filenameElement = image.namedItem( "FILENAME" ).toElement();
            if ( !filenameElement.isNull() )
            {
                QString filename = filenameElement.attribute( "value" );
                m_image.clear();
                m_image.setKey( KoPictureKey( filename, QDateTime::currentDateTime() ) );
                m_doc->addImageRequest( this );
            }
            else
            {
                kdError(32001) << "Missing KEY tag in IMAGE" << endl;
            }
        }
    } else {
        kdError(32001) << "Missing IMAGE tag in FRAMESET" << endl;
    }
}

void KWPictureFrameSet::drawFrame( KWFrame *frame, QPainter *painter, const QRect &crect,
                                   QColorGroup &, bool, bool, KWFrameSetEdit * )
{
    //kdDebug() << "KWPictureFrameSet::drawFrame crect=" << DEBUGRECT(crect) << endl;
    m_image.draw( *painter, 0, 0, kWordDocument()->zoomItX( frame->width() ), kWordDocument()->zoomItY( frame->height() ),
                  crect.x(), crect.y(), crect.width(), crect.height() );
}

KWFrame *KWPictureFrameSet::frameByBorder( const QPoint & nPoint )
{
    // For pictures/cliparts there is nothing to do when clicking
    // inside the frame, so the whole frame is a 'border' (clicking in it selects the frame)
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt )
    {
        QRect outerRect( frameIt.current()->outerRect() );
        if ( outerRect.contains( nPoint ) )
            return frameIt.current();
    }
    return 0L;
}

#ifndef NDEBUG
void KWPictureFrameSet::printDebug( KWFrame *frame )
{
    KWFrameSet::printDebug( frame );
    if ( !isDeleted() )
    {
#if 1
        kdDebug() << "Image: key=" << m_image.getKey().toString() << endl;
#else
        kdDebug() << "KoImage: key=" << m_image.key().toString()
                  << " hasRawData=" << !m_image.rawData().isNull()
                  << " image size=" << m_image.size().width() << "x" << m_image.size().height() << endl;
#endif
    }
}
#endif

/******************************************************************/
/* Class: KWClipartFrameSet                                       */
/******************************************************************/
KWClipartFrameSet::KWClipartFrameSet( KWDocument *_doc, const QString & name )
    : KWFrameSet( _doc )
{
    if ( name.isEmpty() )
        m_name = _doc->generateFramesetName( i18n( "Clipart %1" ) );
    else
        m_name = name;
}

void KWClipartFrameSet::loadClipart( const QString & fileName )
{
    kdDebug() << "KWClipartFrameSet::loadClipart " << fileName << endl;
    KoPictureCollection *collection = m_doc->clipartCollection();
    m_clipart = collection->loadPicture( fileName );
}

QDomElement KWClipartFrameSet::save( QDomElement & parentElem, bool saveFrames )
{
    if ( frames.isEmpty() ) // Deleted frameset -> don't save
        return QDomElement();
    QDomElement framesetElem = parentElem.ownerDocument().createElement( "FRAMESET" );
    parentElem.appendChild( framesetElem );

    KWFrameSet::saveCommon( framesetElem, saveFrames );

    QDomElement imageElem = parentElem.ownerDocument().createElement( "CLIPART" );
    framesetElem.appendChild( imageElem );
    QDomElement elem = parentElem.ownerDocument().createElement( "KEY" );
    imageElem.appendChild( elem );
    m_clipart.getKey().saveAttributes( elem );
    return framesetElem;
}

void KWClipartFrameSet::load( QDomElement &attributes, bool loadFrames )
{
    KWFrameSet::load( attributes, loadFrames );

    // <CLIPART>
    QDomElement image = attributes.namedItem( "CLIPART" ).toElement();
    if ( !image.isNull() ) {
        // <KEY>
        QDomElement keyElement = image.namedItem( "KEY" ).toElement();
        if ( !keyElement.isNull() )
        {
            KoPictureKey key;
            key.loadAttributes( keyElement, QDate(), QTime() );
            m_clipart.clear();
            m_clipart.setKey( key );
            m_doc->addClipartRequest( this );
        }
        else
            kdError(32001) << "Missing KEY tag in CLIPART" << endl;
    } else
        kdError(32001) << "Missing CLIPART tag in FRAMESET" << endl;
}

void KWClipartFrameSet::drawFrame( KWFrame *frame, QPainter *painter, const QRect &crect,
                                   QColorGroup &, bool, bool, KWFrameSetEdit * )
{
    if ( m_clipart.isNull() )
    {
        kdWarning() << "Clipart " << &m_clipart << " is Null! (KWClipartFrameSet::drawFrame)" << endl;
    }
    else
    {
        kdDebug() << "Trying to draw Clipart " << &m_clipart << endl;
    }
    m_clipart.draw( *painter, 0, 0, kWordDocument()->zoomItX( frame->width() ), kWordDocument()->zoomItY( frame->height() ),
                    crect.x(), crect.y(), crect.width(), crect.height() );
}

KWFrame *KWClipartFrameSet::frameByBorder( const QPoint & nPoint )
{
    // For pictures/cliparts there is nothing to do when clicking
    // inside the frame, so the whole frame is a 'border' (clicking in it selects the frame)
    QPtrListIterator<KWFrame> frameIt = frameIterator();
    for ( ; frameIt.current(); ++frameIt )
    {
        QRect outerRect( frameIt.current()->outerRect() );
        if ( outerRect.contains( nPoint ) )
            return frameIt.current();
    }
    return 0L;
}

/******************************************************************/
/* Class: KWPartFrameSet                                          */
/******************************************************************/
KWPartFrameSet::KWPartFrameSet( KWDocument *_doc, KWChild *_child, const QString & name )
    : KWFrameSet( _doc )
{
    m_child = _child;
    kdDebug() << "KWPartFrameSet::KWPartFrameSet" << endl;
    if ( name.isEmpty() )
        m_name = _doc->generateFramesetName( i18n( "Object %1" ) );
    else
        m_name = name;
}

KWPartFrameSet::~KWPartFrameSet()
{
}

KWordFrameSetIface* KWPartFrameSet::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KWordPartFrameSetIface( this );

    return m_dcop;
}


void KWPartFrameSet::drawFrame( KWFrame* frame, QPainter * painter, const QRect & /*crect TODO*/,
                                QColorGroup &, bool onlyChanged, bool, KWFrameSetEdit * )
{
    if (!onlyChanged)
    {
        if ( !m_child || !m_child->document() )
        {
            kdDebug() << "KWPartFrameSet::drawFrame " << this << " aborting. child=" << m_child << " child->document()=" << m_child->document() << endl;
            return;
        }

        // We have to define better what the rect that we pass, means. Does it include zooming ? (yes I think)
        // Does it define the area to be repainted only ? (here it doesn't, really, but it should)
        QRect rframe( 0, 0, kWordDocument()->zoomItX( frame->width() ),
                      kWordDocument()->zoomItY( frame->height() ) );
        //kdDebug() << "rframe=" << DEBUGRECT( rframe ) << endl;

        m_child->document()->paintEverything( *painter, rframe, true, 0L,
                                            kWordDocument()->zoomedResolutionX(), kWordDocument()->zoomedResolutionY() );

    } //else kdDebug() << "KWPartFrameSet::drawFrame " << this << " onlychanged=true!" << endl;
}

void KWPartFrameSet::updateFrames()
{
    if( frames.isEmpty() ) // Deleted frameset -> don't refresh
        return;
    KWFrameSet::updateFrames();
}

void KWPartFrameSet::updateChildGeometry()
{
    if( frames.isEmpty() ) // Deleted frameset
        return;
    // Set the child geometry from the frame geometry, with no viewmode applied
    // This is necessary e.g. before saving, but shouldn't be done while the part
    // is being activated.
    m_child->setGeometry( frames.first()->toQRect() );
}

QDomElement KWPartFrameSet::save( QDomElement &parentElem, bool saveFrames )
{
    if ( frames.isEmpty() ) // Deleted frameset -> don't save
        return QDomElement();
    KWFrameSet::saveCommon( parentElem, saveFrames );
    // Ok, this one is a bit hackish. KWDocument calls us for saving our stuff into
    // the SETTINGS element, which it creates for us. So our save() doesn't really have
    // the same behaviour as a normal KWFrameSet::save()....
    return QDomElement();
}

void KWPartFrameSet::load( QDomElement &attributes, bool loadFrames )
{
    KWFrameSet::load( attributes, loadFrames );
}


KWFrameSetEdit * KWPartFrameSet::createFrameSetEdit( KWCanvas * canvas )
{
    return new KWPartFrameSetEdit( this, canvas );
}

KWPartFrameSetEdit::KWPartFrameSetEdit( KWPartFrameSet * fs, KWCanvas * canvas )
    : KWFrameSetEdit( fs, canvas )
{
    m_cmdMoveChild=0L;
    m_dcop=0L;
    QObject::connect( partFrameSet()->getChild(), SIGNAL( changed( KoChild * ) ),
                      this, SLOT( slotChildChanged() ) );
    QObject::connect( m_canvas->gui()->getView() ,SIGNAL(activated( bool ))
                      ,this,SLOT(slotChildActivated(bool) ) );
}

KWPartFrameSetEdit::~KWPartFrameSetEdit()
{
    kdDebug() << "KWPartFrameSetEdit::~KWPartFrameSetEdit" << endl;
    delete m_dcop;
}

DCOPObject* KWPartFrameSetEdit::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KWordPartFrameSetEditIface( this );
    return m_dcop;
}


void KWPartFrameSetEdit::slotChildActivated(bool b)
{
    //we store command when we desactivate child.
    if( b)
        return;
    if(m_cmdMoveChild && m_cmdMoveChild->frameMoved())
        partFrameSet()->kWordDocument()->addCommand(m_cmdMoveChild);
    else
        delete m_cmdMoveChild;
    m_cmdMoveChild=0L;
}

void KWPartFrameSetEdit::slotChildChanged()
{
    // This is called when the KoDocumentChild is resized (using the KoFrame)
    // We need to react on it in KWPartFrameSetEdit because the view-mode has to be taken into account
    QPtrListIterator<KWFrame>listFrame=partFrameSet()->frameIterator();
    KWFrame *frame = listFrame.current();
    if ( frame  )
    {
        // We need to apply the viewmode conversion correctly (the child is in unzoomed view coords!)
        KoRect childGeom = KoRect::fromQRect( partFrameSet()->getChild()->geometry() );
        // r is the rect in normal coordinates
        QRect r(m_canvas->viewMode()->viewToNormal( frameSet()->kWordDocument()->zoomRect( childGeom ) ) );
        frame->setLeft( r.left() / frameSet()->kWordDocument()->zoomedResolutionX() );
        frame->setTop( r.top() / frameSet()->kWordDocument()->zoomedResolutionY() );
        frame->setWidth( r.width() / frameSet()->kWordDocument()->zoomedResolutionX() );
        frame->setHeight( r.height() / frameSet()->kWordDocument()->zoomedResolutionY() );
        // ## TODO an undo/redo command (move frame)

        kdDebug() << "KWPartFrameSet::slotChildChanged child's geometry " << DEBUGRECT(partFrameSet()->getChild()->geometry() )
                  << " frame set to " << DEBUGRECT( *frame ) << endl;
        partFrameSet()->kWordDocument()->frameChanged( frame );
        //there is just a frame
        if(m_cmdMoveChild)
            m_cmdMoveChild->listFrameMoved().sizeOfEnd = frame->normalize();
    }
}

void KWPartFrameSetEdit::mousePressEvent( QMouseEvent *e, const QPoint &, const KoPoint & )
{
    if ( e->button() != Qt::LeftButton )
        return;

    // activate child part
    partFrameSet()->updateFrames();
    QPtrListIterator<KWFrame>listFrame = partFrameSet()->frameIterator();
    KWFrame *frame = listFrame.current();
    // Set the child geometry from the frame geometry, applying the viewmode
    // (the child is in unzoomed view coords!)
    QRect r( m_canvas->viewMode()->normalToView( frameSet()->kWordDocument()->zoomRect( *frame ) ) );
    partFrameSet()->getChild()->setGeometry( frameSet()->kWordDocument()->unzoomRect( r ).toQRect() );
    kdDebug() << "KWPartFrameSetEdit: activating. child set to " << DEBUGRECT( partFrameSet()->getChild()->geometry() ) << endl;

    KoDocument* part = partFrameSet()->getChild()->document();
    if ( !part )
        return;
    KWView * view = m_canvas->gui()->getView();
    kdDebug() << "Child activated. part=" << part << " child=" << partFrameSet()->getChild() << endl;
    view->partManager()->addPart( part, false );
    view->partManager()->setActivePart( part, view );

    //create undo/redo move command
    FrameIndex index( frame );
    FrameResizeStruct tmpMove;
    tmpMove.sizeOfBegin = frame->normalize();
    tmpMove.sizeOfEnd = KoRect();

    if(!m_cmdMoveChild)
        m_cmdMoveChild=new KWFramePartMoveCommand( i18n("Move Frame"), index, tmpMove );
}

void KWPartFrameSetEdit::mouseDoubleClickEvent( QMouseEvent *, const QPoint &, const KoPoint & )
{
    /// ## Pretty useless since single-click does it now...
    //activate( m_canvas->gui()->getView() );
}



class FormulaView : public KFormula::View {
public:
    FormulaView( KWFormulaFrameSetEdit* edit, KFormula::Container* c )
        : KFormula::View( c ), m_edit( edit ) {}

    /** Gets called if the cursor ties to leave the formula at its begin. */
    virtual void exitLeft() { m_edit->exitLeft(); }

    /** Gets called if the cursor ties to leave the formula at its end. */
    virtual void exitRight() { m_edit->exitRight(); }

private:
    KWFormulaFrameSetEdit* m_edit;
};


/******************************************************************/
/* Class: KWFormulaFrameSet                                       */
/******************************************************************/
KWFormulaFrameSet::KWFormulaFrameSet( KWDocument *_doc, const QString & name )
    : KWFrameSet( _doc ), m_changed( false )
{
    kdDebug(32001) << "KWFormulaFrameSet::KWFormulaFrameSet" << endl;
    formula = _doc->getFormulaDocument()->createFormula();
    // With the new drawing scheme (drawFrame being called with translated painter)
    // there is no need to move the KFormulaContainer anymore, it remains at (0,0).
    formula->moveTo( 0, 0 );

    connect( formula, SIGNAL( formulaChanged( double, double ) ),
             this, SLOT( slotFormulaChanged( double, double ) ) );
    if ( name.isEmpty() )
        m_name = _doc->generateFramesetName( i18n( "Formula %1" ) );
    else
        m_name = name;

    /*
    if ( isFloating() ) {
        // we need to look for the anchor every time, don't cache this value.
        // undo/redo creates/deletes anchors
        KWAnchor * anchor = findAnchor( 0 );
        if ( anchor ) {
            QTextFormat * format = anchor->format();
            formula->setFontSize( format->font().pointSize() );
        }
    }
    */
}

KWordFrameSetIface* KWFormulaFrameSet::dcopObject()
{
    if ( !m_dcop )
	m_dcop = new KWordFormulaFrameSetIface( this );

    return m_dcop;
}

KWFormulaFrameSet::~KWFormulaFrameSet()
{
    kdDebug(32001) << "KWFormulaFrameSet::~KWFormulaFrameSet" << endl;
    delete formula;
}

void KWFormulaFrameSet::addFrame( KWFrame *_frame, bool recalc )
{
    if ( formula ) {
        _frame->setWidth( formula->width() );
        _frame->setHeight( formula->height() );
    }
    KWFrameSet::addFrame( _frame, recalc );
}

KWFrameSetEdit* KWFormulaFrameSet::createFrameSetEdit(KWCanvas* canvas)
{
    return new KWFormulaFrameSetEdit(this, canvas);
}

void KWFormulaFrameSet::drawFrame( KWFrame* /*frame*/, QPainter* painter, const QRect& crect,
                                   QColorGroup& cg, bool onlyChanged, bool resetChanged,
                                   KWFrameSetEdit *edit )
{
    //kdDebug() << "KWFormulaFrameSet::drawFrame m_changed=" << m_changed << " onlyChanged=" << onlyChanged << endl;
    if ( m_changed || !onlyChanged )
    {
        if ( resetChanged )
            m_changed = false;

        if ( edit )
        {
            KWFormulaFrameSetEdit * formulaEdit = static_cast<KWFormulaFrameSetEdit *>(edit);
            if ( formulaEdit->getFormulaView() ) {
                formulaEdit->getFormulaView()->draw( *painter, crect, cg );
            }
            else {
                formula->draw( *painter, crect, cg );
            }
        }
        else
        {
            //kdDebug() << "KWFormulaFrameSet::drawFrame drawing (without edit) crect=" << DEBUGRECT( crect ) << endl;
            formula->draw( *painter, crect, cg );
        }
    }
}

void KWFormulaFrameSet::slotFormulaChanged( double width, double height )
{
    if ( frames.isEmpty() )
        return;

    double oldWidth = frames.first()->width();
    double oldHeight = frames.first()->height();

    frames.first()->setWidth( width );
    frames.first()->setHeight( height );

    updateFrames();
    kWordDocument()->layout();
    if ( ( oldWidth != width ) || ( oldHeight != height ) ) {
        kWordDocument()->repaintAllViews( false );
        kWordDocument()->updateRulerFrameStartEnd();
    }

    m_changed = true;
}

void KWFormulaFrameSet::updateFrames()
{
    KWFrameSet::updateFrames();
}

QDomElement KWFormulaFrameSet::save(QDomElement& parentElem, bool saveFrames)
{
    if ( frames.isEmpty() ) // Deleted frameset -> don't save
        return QDomElement();
    QDomElement framesetElem = parentElem.ownerDocument().createElement("FRAMESET");
    parentElem.appendChild(framesetElem);

    KWFrameSet::saveCommon(framesetElem, saveFrames);

    QDomElement formulaElem = parentElem.ownerDocument().createElement("FORMULA");
    framesetElem.appendChild(formulaElem);
    formula->save(formulaElem);
    return framesetElem;
}

void KWFormulaFrameSet::load(QDomElement& attributes, bool loadFrames)
{
    KWFrameSet::load(attributes, loadFrames);
    QDomElement formulaElem = attributes.namedItem("FORMULA").toElement();
    paste( formulaElem );
}

void KWFormulaFrameSet::paste( QDomNode& formulaElem )
{
    if (!formulaElem.isNull()) {
        if (formula == 0) {
            formula = m_doc->getFormulaDocument()->createFormula();
            connect(formula, SIGNAL(formulaChanged(double, double)),
                    this, SLOT(slotFormulaChanged(double, double)));
        }
        if (!formula->load(formulaElem)) {
            kdError(32001) << "Error loading formula" << endl;
        }
    }
    else {
        kdError(32001) << "Missing FORMULA tag in FRAMESET" << endl;
    }
}

void KWFormulaFrameSet::zoom( bool forPrint )
{
    if ( !frames.isEmpty() ) {
        KWFrameSet::zoom( forPrint );
    }
}

int KWFormulaFrameSet::floatingFrameBaseline( int /*frameNum*/ )
{
    if ( !frames.isEmpty() )
    {
        return formula->baseline();
    }
    return -1;
}

void KWFormulaFrameSet::showPopup( KWFrame *, KWFrameSetEdit *, KWView *view, const QPoint &point )
{
    QPopupMenu * popup = view->popupMenu("Formula");
    Q_ASSERT(popup);
    if (popup)
        popup->popup( point );
}


KWFormulaFrameSetEdit::KWFormulaFrameSetEdit(KWFormulaFrameSet* fs, KWCanvas* canvas)
        : KWFrameSetEdit(fs, canvas)
{
    //kdDebug(32001) << "KWFormulaFrameSetEdit::KWFormulaFrameSetEdit" << endl;
    formulaView = new FormulaView( this, fs->getFormula() );
    //formulaView->setSmallCursor(true);

    connect( formulaView, SIGNAL( cursorChanged( bool, bool ) ),
             this, SLOT( cursorChanged( bool, bool ) ) );

    m_canvas->gui()->getView()->showFormulaToolbar(true);
    focusInEvent();
    dcop=0;
}

DCOPObject* KWFormulaFrameSetEdit::dcopObject()
{
    if ( !dcop )
	dcop = new KWordFormulaFrameSetEditIface( this );
    return dcop;
}

KWFormulaFrameSetEdit::~KWFormulaFrameSetEdit()
{
    //kdDebug(32001) << "KWFormulaFrameSetEdit::~KWFormulaFrameSetEdit" << endl;
    focusOutEvent();
    // this causes a core dump on quit
    m_canvas->gui()->getView()->showFormulaToolbar(false);
    delete formulaView;
    formulaView = 0;
    formulaFrameSet()->setChanged();
    m_canvas->repaintChanged( formulaFrameSet(), true );
    delete dcop;
}

const KFormula::View* KWFormulaFrameSetEdit::getFormulaView() const { return formulaView; }
KFormula::View* KWFormulaFrameSetEdit::getFormulaView() { return formulaView; }

void KWFormulaFrameSetEdit::keyPressEvent( QKeyEvent* event )
{
    //kdDebug(32001) << "KWFormulaFrameSetEdit::keyPressEvent" << endl;
    formulaView->keyPressEvent( event );;
}

void KWFormulaFrameSetEdit::mousePressEvent( QMouseEvent* event,
                                             const QPoint&,
                                             const KoPoint& pos )
{
    // [Note that this method is called upon RMB and MMB as well, now]
    KoPoint tl = m_currentFrame->topLeft();
    formulaView->mousePressEvent( event, pos-tl );
}

void KWFormulaFrameSetEdit::mouseMoveEvent( QMouseEvent* event,
                                            const QPoint&,
                                            const KoPoint& pos )
{
    KoPoint tl = m_currentFrame->topLeft();
    formulaView->mouseMoveEvent( event, pos-tl );
}

void KWFormulaFrameSetEdit::mouseReleaseEvent( QMouseEvent* event,
                                               const QPoint&,
                                               const KoPoint& pos )
{
    KoPoint tl = m_currentFrame->topLeft();
    formulaView->mouseReleaseEvent( event, pos-tl );
}

void KWFormulaFrameSetEdit::focusInEvent()
{
    //kdDebug(32001) << "KWFormulaFrameSetEdit::focusInEvent" << endl;
    formulaView->focusInEvent(0);
}

void KWFormulaFrameSetEdit::focusOutEvent()
{
    //kdDebug(32001) << "KWFormulaFrameSetEdit::focusOutEvent" << endl;
    formulaView->focusOutEvent(0);
}

void KWFormulaFrameSetEdit::copy()
{
    formulaView->getDocument()->copy();
}

void KWFormulaFrameSetEdit::cut()
{
    formulaView->getDocument()->cut();
}

void KWFormulaFrameSetEdit::paste()
{
    formulaView->getDocument()->paste();
}

void KWFormulaFrameSetEdit::selectAll()
{
    formulaView->slotSelectAll();
}

void KWFormulaFrameSetEdit::moveHome()
{
    formulaView->moveHome( KFormula::WordMovement );
}
void KWFormulaFrameSetEdit::moveEnd()
{
    formulaView->moveEnd( KFormula::WordMovement );
}

void KWFormulaFrameSetEdit::exitLeft()
{
    int index = formulaFrameSet()->findAnchor(0)->index();
    KoTextParag *parag = static_cast<KoTextParag*>( formulaFrameSet()->findAnchor( 0 )->paragraph() );
    m_canvas->editTextFrameSet( formulaFrameSet()->anchorFrameset(), parag, index );
}

void KWFormulaFrameSetEdit::exitRight()
{
    int index = formulaFrameSet()->findAnchor(0)->index();
    KoTextParag *parag = static_cast<KoTextParag*>( formulaFrameSet()->findAnchor( 0 )->paragraph() );
    m_canvas->editTextFrameSet( formulaFrameSet()->anchorFrameset(), parag, index+1 );
}

void KWFormulaFrameSetEdit::cursorChanged( bool visible, bool /*selecting*/ )
{
    if ( visible ) {
        if ( m_currentFrame )
        {
            // Add the cursor position to the (zoomed) frame position
            QPoint nPoint = frameSet()->kWordDocument()->zoomPoint( m_currentFrame->topLeft() );
            nPoint += formulaView->getCursorPoint();
            // Apply viewmode conversion
            QPoint p = m_canvas->viewMode()->normalToView( nPoint );
            m_canvas->ensureVisible( p.x(), p.y() );
        }
    }
    formulaFrameSet()->setChanged();
    m_canvas->repaintChanged( formulaFrameSet(), true );
}

#include "kwframe.moc"
