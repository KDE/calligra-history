/* -*- Mode: C++ -*-

  $Id$

  KDFrame - a multi-platform framing engine

  Copyright (C) 2001 by Klar�lvdalens Datakonsult AB
*/

#include <qpainter.h>
#include <qbrush.h>

#include <KDFrame.h>
#include <KDFrameProfileSection.h>
#include <KDXMLTools.h>
#include "KDFrame.moc"

// internal note:
//
// Yes, the following statement appears in KDChartParams.h as well.
//
// The reason is that KDFrame is a class completely separated
// from the rest of the KDChart project.
//
#if defined( SUN7 )
#include <math.h>
#else
#include <cmath>
#endif
#ifdef __WINDOWS__
#define M_PI 3.14159265358979323846
#endif

KDFrame::~KDFrame()
{
    // Intentionally left blank for now.
}

KDFrame::KDFrameCorner::~KDFrameCorner()
{
    // Intentionally left blank for now.
}





void KDFrame::paintBackground( QPainter& painter, const QRect& innerRect ) const
{
    /* first draw the brush (may contain a pixmap)*/
    if( Qt::NoBrush != _background.style() ) {
        QPen   oldPen(   painter.pen() );
        QPoint oldOrig(  painter.brushOrigin() );
        QBrush oldBrush( painter.brush() );
        painter.setPen( Qt::NoPen );
        painter.setBrushOrigin( innerRect.x(),
                                innerRect.y() );
        painter.setBrush( _background );
        painter.drawRect( innerRect.x(),
                          innerRect.y(),
                          innerRect.width(),
                          innerRect.height() );
        painter.setPen(         oldPen );
        painter.setBrushOrigin( oldOrig );
        painter.setBrush(       oldBrush );
    }
    /* next draw the backPixmap over the brush */
    if( ! _backPixmap.isNull() ) {
        QPoint ol = innerRect.topLeft();
        if( PixCentered == _backPixmapMode )
        {
            ol.setX( innerRect.center().x() - _backPixmap.width() / 2 );
            ol.setY( innerRect.center().y() - _backPixmap.height()/ 2 );
            bitBlt( painter.device(), ol, &_backPixmap );
        } else {
            QWMatrix m;
            double zW = (double)innerRect.width()  / (double)_backPixmap.width();
            double zH = (double)innerRect.height() / (double)_backPixmap.height();
            switch ( _backPixmapMode ) {
            case PixCentered:
                break;
            case PixScaled: {
                    double z;
                    z = QMIN( zW, zH );
                    m.scale( z, z );
                }
                break;
            case PixStretched:
                m.scale( zW, zH );
                break;
            }
            QPixmap pm = _backPixmap.xForm( m );
            ol.setX( innerRect.center().x() - pm.width() / 2 );
            ol.setY( innerRect.center().y() - pm.height()/ 2 );
            bitBlt( painter.device(), ol, &pm );
        }
    }
}


void KDFrame::paintEdges( QPainter& painter, const QRect& innerRect ) const
{

    /*
      Note: The following code OF COURSE is only dummy-code and will be replaced.
    */
    if( Qt::NoPen != _pen.style() ) {
        QPen oldPen( painter.pen() );
        QBrush oldBrush( Qt::NoBrush );
        painter.setPen( _pen );
        painter.setBrush( Qt::NoBrush );
        painter.drawRect( QMAX( innerRect.x()-1, 0),
                          QMAX( innerRect.y()-1, 0),
                          innerRect.width()  +2,
                          innerRect.height() +2 );
        painter.setBrush( oldBrush );
        painter.setPen( oldPen );
    }
}


void KDFrame::paintCorners( QPainter& painter, const QRect& innerRect ) const
{
    ;
}


void KDFrame::paint( QPainter* painter,
                     KDFramePaintSteps steps,
                     QRect innerRect ) const
{
    if( painter ) {
        const QRect& rect = innerRect.isValid() ? innerRect : _innerRect;
        switch( steps ) {
        case PaintBackground:
            paintBackground( *painter, rect );
            break;
        case PaintEdges:
            paintEdges(      *painter, rect );
            break;
        case PaintCorners:
            paintCorners(    *painter, rect );
            break;
        case PaintBorder:
            paintEdges(      *painter, rect );
            paintCorners(    *painter, rect );
            break;
        case PaintAll:
            paintBackground( *painter, rect );
            paintEdges(      *painter, rect );
            paintCorners(    *painter, rect );
            break;
        }
    }
}


void KDFrame::clearProfile( ProfileName name )
{
    switch( name ) {
    case ProfileTop:   _topProfile.clear();
        break;
    case ProfileRight: _rightProfile.clear();
        break;
    case ProfileBottom:_bottomProfile.clear();
        break;
    case ProfileLeft:  _leftProfile.clear();
        break;
    }
}

void KDFrame::addProfileSection( ProfileName      name,
                                 int              wid,
                                 QPen             pen,
                                 KDFrameProfileSection::Direction dir,
                                 KDFrameProfileSection::Curvature curv )
{
    switch( name ) {
    case ProfileTop:   _topProfile.append(   new KDFrameProfileSection( dir, curv, wid, pen ) );
        break;
    case ProfileRight: _rightProfile.append( new KDFrameProfileSection( dir, curv, wid, pen ) );
        break;
    case ProfileBottom:_bottomProfile.append(new KDFrameProfileSection( dir, curv, wid, pen ) );
        break;
    case ProfileLeft:  _leftProfile.append(  new KDFrameProfileSection( dir, curv, wid, pen ) );
        break;
    }
}

void KDFrame::setProfile( ProfileName name, const KDFrameProfile& profile )
{
    switch( name ) {
    case ProfileTop:   _topProfile    = profile;
        break;
    case ProfileRight: _rightProfile  = profile;
        break;
    case ProfileBottom:_bottomProfile = profile;
        break;
    case ProfileLeft:  _leftProfile   = profile;
        break;
    }
}

const KDFrameProfile& KDFrame::profile( ProfileName name ) const
{
    switch( name ) {
    case ProfileTop:   return _topProfile;
        break;
    case ProfileRight: return _rightProfile;
        break;
    case ProfileBottom:return _bottomProfile;
        break;
    default:           return _leftProfile;
    }
}


void KDFrame::setSimpleFrame( SimpleFrame frame,
                              int         lineWidth,
                              int         midLineWidth,
                              QPen        pen,
                              QBrush      background,
                              const QPixmap* backPixmap,
                              BackPixmapMode backPixmapMode )
{
    _topProfile.clear();
    _rightProfile.clear();
    _bottomProfile.clear();
    _leftProfile.clear();
    _pen        = pen;
    _background = background;
    _backPixmap = backPixmap ? *backPixmap : QPixmap();
    _backPixmapMode = backPixmapMode;
    if( FrameFlat == frame ) {
        _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                       KDFrameProfileSection::CvtPlain,
                                                       lineWidth, pen ) );
        _rightProfile  = _topProfile;
        _bottomProfile = _topProfile;
        _leftProfile   = _topProfile;
    }
    else {
        switch( frame ) {
        case FrameElegance: {
            int line1 = lineWidth / 8;
            int line2 = lineWidth / 16;
            int line3 = line2;
            int gap2  = line2 * 4;
            int gap1  = lineWidth - line1 - line2 - line3 - gap2;
            QPen noP( Qt::NoPen );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           line1, pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           gap1,  noP ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           line2, pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           gap2,  noP ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           line3, pen ) );
        }
            break;
        case FrameBoxRaized:
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirRaising,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth,    pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           midLineWidth, pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirSinking,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth,    pen ) );
            break;
        case FrameBoxSunken:
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirSinking,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth,    pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           midLineWidth, pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirRaising,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth,    pen ) );
            break;
        case FramePanelRaized:
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirRaising,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth, pen ) );
            break;
        case FramePanelSunken:
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirSinking,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth, pen ) );
            break;
        case FrameSemicircular:
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirRaising,
                                                           KDFrameProfileSection::CvtConvex,
                                                           lineWidth/2, pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirPlain,
                                                           KDFrameProfileSection::CvtPlain,
                                                           lineWidth-2*(lineWidth/2), pen ) );
            _topProfile.append( new KDFrameProfileSection( KDFrameProfileSection::DirSinking,
                                                           KDFrameProfileSection::CvtConcave,
                                                           lineWidth/2, pen ) );
            break;
        }
    }
    _rightProfile  = _topProfile;
    _bottomProfile = _topProfile;
    _leftProfile   = _topProfile;
    setCorners( CornerNormal, 0 );
}


void KDFrame::createFrameNode( QDomDocument& document, QDomNode& parent,
			       const QString& elementName,
			       const KDFrame& frame )
{
    QDomElement frameElement = document.createElement( elementName );
    parent.appendChild( frameElement );
    KDXML::createIntNode( document, frameElement, "ShadowWidth",
                          frame._shadowWidth );
    KDXML::createStringNode( document, frameElement, "CornerName",
                             cornerNameToString( frame._sunPos ) );

    KDXML::createBrushNode( document, frameElement, "Background",
                            frame._background );
    KDXML::createPixmapNode( document, frameElement, "BackPixmap",
                             frame._backPixmap );
    KDXML::createStringNode( document, frameElement, "BackPixmapMode",
                             backPixmapModeToString( frame._backPixmapMode ) );

    KDXML::createRectNode( document, frameElement, "InnerRect",
                           frame._innerRect );
    createFrameProfileNode( document, frameElement, "TopProfile",
			    frame._topProfile );
    createFrameProfileNode( document, frameElement, "RightProfile",
			    frame._rightProfile );
    createFrameProfileNode( document, frameElement, "BottomProfile",
			    frame._bottomProfile );
    createFrameProfileNode( document, frameElement, "LeftProfile",
			    frame._leftProfile );
    KDFrameCorner::createFrameCornerNode( document, frameElement, "CornerTL",
                                          frame._cornerTL );
    KDFrameCorner::createFrameCornerNode( document, frameElement, "CornerTR",
                                          frame._cornerTR );
    KDFrameCorner::createFrameCornerNode( document, frameElement, "CornerBL",
                                          frame._cornerBL );
    KDFrameCorner::createFrameCornerNode( document, frameElement, "CornerBR",
                                          frame._cornerBR );
}

void KDFrame::createFrameProfileNode( QDomDocument& document, QDomNode& parent,
				      const QString& elementName,
				      KDFrameProfile profile )
{
    QDomElement profileElement = document.createElement( elementName );
    parent.appendChild( profileElement );
    for( const KDFrameProfileSection* section = profile.first(); section != 0;
	 section = profile.next() )
	KDFrameProfileSection::createFrameProfileSectionNode( document,
                                                              profileElement,
                                                              "ProfileSection",
                                                              section );
}


void KDFrame::KDFrameCorner::createFrameCornerNode( QDomDocument& document,
                                                    QDomNode& parent,
                                                    const QString& elementName,
                                                    const KDFrameCorner& corner )
{
    QDomElement cornerElement = document.createElement( elementName );
    parent.appendChild( cornerElement );
    KDXML::createStringNode( document, cornerElement, "Style",
                             KDFrame::cornerStyleToString( corner._style ) );
    KDXML::createIntNode( document, cornerElement, "Width",
                          corner._width );
    createFrameProfileNode( document, cornerElement, "Profile",
			    corner._profile );
}

bool KDFrame::readFrameNode( const QDomElement& element, KDFrame& frame )
{
    bool ok = true;
    int tempShadowWidth;
    CornerName tempCornerName;
    QBrush tempBackground;
    QPixmap tempBackPixmap;
    BackPixmapMode tempBackPixmapMode;
    QRect tempInnerRect;
    KDFrameProfile tempTopProfile, tempRightProfile,
	tempBottomProfile, tempLeftProfile;
    KDFrameCorner tempCornerTL, tempCornerTR, tempCornerBL, tempCornerBR;
    QDomNode node = element.firstChild();
    while( !node.isNull() ) {
        QDomElement element = node.toElement();
        if( !element.isNull() ) { // was really an element
            QString tagName = element.tagName();
            if( tagName == "ShadowWidth" ) {
                ok = ok & KDXML::readIntNode( element, tempShadowWidth );
            } else if( tagName == "CornerName" ) {
		QString value;
                ok = ok & KDXML::readStringNode( element, value );
		tempCornerName = stringToCornerName( value );
            } else if( tagName == "Background" ) {
                ok = ok & KDXML::readBrushNode( element, tempBackground );
            } else if( tagName == "BackPixmap" ) {
                ok = ok & KDXML::readPixmapNode( element, tempBackPixmap );
            } else if( tagName == "BackPixmapMode" ) {
                QString value;
                ok = ok & KDXML::readStringNode( element, value );
                tempBackPixmapMode = stringToBackPixmapMode( value );
            } else if( tagName == "InnerRect" ) {
                ok = ok & KDXML::readRectNode( element, tempInnerRect );
            } else if( tagName == "TopProfile" ) {
                ok = ok & readFrameProfileNode( element, tempTopProfile );
            } else if( tagName == "RightProfile" ) {
                ok = ok & readFrameProfileNode( element, tempRightProfile );
            } else if( tagName == "BottomProfile" ) {
                ok = ok & readFrameProfileNode( element, tempBottomProfile );
            } else if( tagName == "LeftProfile" ) {
                ok = ok & readFrameProfileNode( element, tempLeftProfile );
            } else if( tagName == "CornerTL" ) {
                ok = ok & KDFrameCorner::readFrameCornerNode( element,
                                                              tempCornerTL );
            } else if( tagName == "CornerTR" ) {
                ok = ok & KDFrameCorner::readFrameCornerNode( element,
                                                              tempCornerTR );
            } else if( tagName == "CornerBL" ) {
                ok = ok & KDFrameCorner::readFrameCornerNode( element,
                                                              tempCornerBL );
            } else if( tagName == "CornerBR" ) {
                ok = ok & KDFrameCorner::readFrameCornerNode( element,
                                                              tempCornerBR );
            } else {
                qDebug( "Unknown tag in frame" );
            }
        }
        node = node.nextSibling();
    }

    if( ok ) {
	frame._shadowWidth = tempShadowWidth;
	frame._sunPos = tempCornerName;
        frame._background = tempBackground;
        frame._backPixmap = tempBackPixmap;
        frame._backPixmapMode = tempBackPixmapMode;
	frame._innerRect = tempInnerRect;
	frame._topProfile = tempTopProfile;
	frame._rightProfile = tempRightProfile;
	frame._bottomProfile = tempBottomProfile;
	frame._leftProfile = tempLeftProfile;
	frame._cornerTL = tempCornerTL;
	frame._cornerTR = tempCornerTR;
	frame._cornerBL = tempCornerBL;
	frame._cornerBR = tempCornerBR;
    }

    return ok;
}


bool KDFrame::readFrameProfileNode( const QDomElement& element,
				    KDFrameProfile& profile )
{
    QDomNode node = element.firstChild();
    while( !node.isNull() ) {
        QDomElement element = node.toElement();
        if( !element.isNull() ) { // was really an element
            QString tagName = element.tagName();
	    if( tagName == "ProfileSection" ) {
		KDFrameProfileSection* section = new KDFrameProfileSection();
                KDFrameProfileSection::readFrameProfileSectionNode( element,
                                                                    section );
		profile.append( section );
            } else {
                qDebug( "Unknown tag in double map" );
                return false;
            }
        }
        node = node.nextSibling();
    }

    return true;
}


bool KDFrame::KDFrameCorner::readFrameCornerNode( const QDomElement& element,
                                                  KDFrameCorner& corner )
{
    bool ok = true;
    CornerStyle tempStyle;
    int tempWidth;
    KDFrameProfile tempProfile;
    QDomNode node = element.firstChild();
    while( !node.isNull() ) {
        QDomElement element = node.toElement();
        if( !element.isNull() ) { // was really an element
            QString tagName = element.tagName();
            if( tagName == "Style" ) {
		QString value;
                ok = ok & KDXML::readStringNode( element, value );
		tempStyle = stringToCornerStyle( value );
            } else if( tagName == "Width" ) {
                ok = ok & KDXML::readIntNode( element, tempWidth );
            } else if( tagName == "Profile" ) {
		KDFrameProfile profile;
                ok = ok & readFrameProfileNode( element, profile );
            } else {
                qDebug( "Unknown tag in frame" );
            }
        }
        node = node.nextSibling();
    }

    if( ok ) {
	corner._style = tempStyle;
	corner._width = tempWidth;
	corner._profile = tempProfile;
    }

    return ok;
}
