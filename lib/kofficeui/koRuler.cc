/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// Description: Ruler (header)

/******************************************************************/

#include "koRuler.h"
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>


class KoRulerPrivate {
public:
    KoRulerPrivate() {
    }
    ~KoRulerPrivate() {}

    QWidget *canvas;
    int flags;
    int oldMx, oldMy;
    bool mousePressed;
    KoRuler::Action action;
    bool whileMovingBorderLeft, whileMovingBorderRight; 	
    bool whileMovingBorderTop, whileMovingBorderBottom;
    QPixmap pmFirst, pmLeft;
    KoTabChooser *tabChooser;
    QList<KoTabulator> tabList;
    bool removeTab;     // Do we have to remove a tab in the DC Event?
    int currTab;
    QPopupMenu *rb_menu;
    int mMM, mPT, mINCH;
};


/******************************************************************/
/* Class: KoRuler						  */
/******************************************************************/

/*================================================================*/
KoRuler::KoRuler( QWidget *_parent, QWidget *_canvas, Orientation _orientation,
		 KoPageLayout _layout, int _flags, KoTabChooser *_tabChooser )
    : QFrame( _parent ), buffer( width(), height() ), m_zoom(1.0), m_1_zoom(1.0),
      unit( "mm" )
{
    setWFlags( WResizeNoErase );
    setFrameStyle( Box | Raised );

    d=new KoRulerPrivate();

    d->tabChooser = _tabChooser;

    d->canvas = _canvas;
    orientation = _orientation;
    layout = _layout;
    d->flags = _flags;

    diffx = 0;
    diffy = 0;

    setMouseTracking( true );
    d->mousePressed = false;
    d->action = A_NONE;

    d->oldMx = 0;
    d->oldMy = 0;

    showMPos = false;
    mposX = 0;
    mposY = 0;
    hasToDelete = false;
    d->whileMovingBorderLeft = d->whileMovingBorderRight = d->whileMovingBorderTop = d->whileMovingBorderBottom = false;

    d->pmFirst = UserIcon( "koRulerFirst" );
    d->pmLeft = UserIcon( "koRulerLeft" );
    d->currTab = -1;

    d->tabList.setAutoDelete( false );
    d->removeTab=false;
    frameStart = -1;

    allowUnits = true;
    setupMenu();
}

/*================================================================*/
KoRuler::~KoRuler()
{
    delete d;
}

/*================================================================*/
void KoRuler::setMousePos( int mx, int my )
{
    if ( !showMPos || ( mx == mposX && my == mposY ) ) return;

    QPainter p;
    p.begin( this );
    p.setRasterOp( NotROP );
    p.setPen( QPen( black, 1, SolidLine ) );

    if ( orientation == Qt::Horizontal ) {
	if ( hasToDelete )
	    p.drawLine( mposX, 1, mposX, height() - 1 );
	p.drawLine( mx, 1, mx, height() - 1 );
	hasToDelete = true;
    }

    if ( orientation == Qt::Vertical ) {
	if ( hasToDelete )
	    p.drawLine( 1, mposY, width() - 1, mposY );
	p.drawLine( 1, my, width() - 1, my );
	hasToDelete = true;
    }
    p.end();

    mposX = mx;
    mposY = my;
}

/*================================================================*/
void KoRuler::drawHorizontal( QPainter *_painter )
{
    QPainter p;
    p.begin( &buffer );

    p.fillRect( 0, 0, width(), height(), QBrush( colorGroup().brush( QColorGroup::Background ) ) );

    double dist;
    int j = 0;
    double pw = static_cast<double>( layout.ptWidth );
    pw=zoomIt(pw);
    QString str;
    QFont font = QFont( "helvetica", 8 ); // Ugh... hardcoded (Werner)
    QFontMetrics fm( font );

    p.setPen( QPen( black, 1, SolidLine ) );
    p.setBrush( white );

    QRect r;
    if ( !d->whileMovingBorderLeft )
	r.setLeft( -diffx + zoomIt(layout.ptLeft) );
    else
	r.setLeft( d->oldMx );
    r.setTop( 0 );
    if ( !d->whileMovingBorderRight )
	r.setRight( -diffx + pw - zoomIt(layout.ptRight) );
    else
	r.setRight( d->oldMx );
    r.setBottom( height() );

    p.drawRect( r );

    p.setPen( QPen( black, 1, SolidLine ) );
    p.setFont( font );

    if ( unit == "inch" )
	dist = _INCH_TO_POINT * m_zoom;
    else if ( unit == "pt" )
	dist = 100.0 * m_zoom;
    else
	dist = 10.0 * _MM_TO_POINT * m_zoom;

    for ( double i = 0.0;i <= pw;i += dist ) {
	str=QString::number(j++);
	if ( unit == "pt" && j!=1)
	    str+="00";
	p.drawText( double2Int(i) - diffx - fm.width( str ) * 0.5, ( height() - fm.height() ) * 0.5,
		    fm.width( str ), height(), AlignLeft | AlignTop, str );
    }

    for ( double i = dist * 0.5;i <= pw;i += dist ) {
	int ii=double2Int(i);
	p.drawLine( ii - diffx, 5, ii - diffx, height() - 5 );
    }

    for ( double i = dist * 0.25;i <= pw;i += dist * 0.5 ) {
	int ii=double2Int(i);
	p.drawLine( ii - diffx, 7, ii - diffx, height() - 7 );
    }

    p.setPen( QPen( black ) );
    //p.drawLine( pw - diffx - 1, 1, pw - diffx - 1, height() - 1 );
    int constant=zoomIt(1);
    p.drawLine( pw - diffx + constant, 1, pw - diffx + constant, height() - 1 );
    p.setPen( QPen( white ) );
    p.drawLine( pw - diffx, 1, pw - diffx, height() - 1 );

    p.setPen( QPen( black ) );
    //p.drawLine( -diffx - 2, 1, -diffx - 2, height() - 1 );
    p.drawLine( -diffx, 1, -diffx, height() - 1 );
    p.setPen( QPen( white ) );
    p.drawLine( -diffx - constant, 1, -diffx - constant, height() - 1 );

    if ( d->flags & F_INDENTS ) {
	p.drawPixmap( zoomIt(i_first) - d->pmFirst.size().width() * 0.5 + r.left(), 2, d->pmFirst );
	p.drawPixmap( zoomIt(i_left) - d->pmLeft.size().width() * 0.5 + r.left(),
		     height() - d->pmLeft.size().height() - 2, d->pmLeft );
    }

    if ( d->action == A_NONE && showMPos ) {
	p.setPen( QPen( black, 1, SolidLine ) );
	p.drawLine( mposX, 1, mposX, height() - 1 );
    }
    hasToDelete = false;

    if ( d->tabChooser && ( d->flags & F_TABS ) && !d->tabList.isEmpty() )
	drawTabs( p );

    p.end();
    _painter->drawPixmap( 0, 0, buffer );
}

/*================================================================*/
void KoRuler::drawTabs( QPainter &_painter )
{
    KoTabulator *_tab = 0L;
    int ptPos = 0;

    _painter.setPen( QPen( black, 2, SolidLine ) );

    for ( unsigned int i = 0;i < d->tabList.count();i++ ) {
	_tab = d->tabList.at( i );
	ptPos = zoomIt(_tab->ptPos) - diffx + ( frameStart == -1 ? static_cast<int>( zoomIt(layout.ptLeft) ) :
						zoomIt(frameStart) );
	switch ( _tab->type ) {
	case T_LEFT: {
	    ptPos -= 4;
	    _painter.drawLine( ptPos + 4, height() - 4, ptPos + 20 - 4, height() - 4 );
	    _painter.drawLine( ptPos + 5, 4, ptPos + 5, height() - 4 );
	} break;
	case T_CENTER: {
	    ptPos -= 10;
	    _painter.drawLine( ptPos + 4, height() - 4, ptPos + 20 - 4, height() - 4 );
	    _painter.drawLine( ptPos + 20 * 0.5, 4, ptPos + 20 * 0.5, height() - 4 );
	} break;
	case T_RIGHT: {
	    ptPos -= 16;
	    _painter.drawLine( ptPos + 4, height() - 4, ptPos + 20 - 4, height() - 4 );
	    _painter.drawLine( ptPos + 20 - 5, 4, ptPos + 20 - 5, height() - 4 );
	} break;
	case T_DEC_PNT: {
	    ptPos -= 10;
	    _painter.drawLine( ptPos + 4, height() - 4, ptPos + 20 - 4, height() - 4 );
	    _painter.drawLine( ptPos + 20 * 0.5, 4, ptPos + 20 * 0.5, height() - 4 );
	    _painter.fillRect( ptPos + 20 * 0.5 + 2, height() - 9, 3, 3, black );
	} break;
	default: break;
	}
    }
}

/*================================================================*/
void KoRuler::drawVertical( QPainter *_painter )
{
    QPainter p;
    p.begin( &buffer );

    p.fillRect( 0, 0, width(), height(), QBrush( colorGroup().brush( QColorGroup::Background ) ) );

    double dist;
    int j = 0;
    double ph = static_cast<double>( layout.ptHeight );
    ph=zoomIt(ph);
    QString str;
    QFont font = QFont( "helvetica", 8 );  // Hardcode? (Werner)
    QFontMetrics fm( font );

    p.setPen( QPen( black, 1, SolidLine ) );
    p.setBrush( white );

    QRect r;

    if ( !d->whileMovingBorderTop )
	r.setTop( -diffy + zoomIt(layout.ptTop) );
    else
	r.setTop( d->oldMy );
    r.setLeft( 0 );
    if ( !d->whileMovingBorderBottom )
	r.setBottom( -diffy + ph - zoomIt(layout.ptBottom) );
    else
	r.setBottom( d->oldMy );
    r.setRight( width() );

    p.drawRect( r );

    p.setPen( QPen( black, 1, SolidLine ) );
    p.setFont( font );

    if ( unit == "inch" )
	dist = _INCH_TO_POINT * m_zoom;
    else if ( unit == "pt" )
	dist = 100.0 * m_zoom;
    else
	dist = 10.0 * _MM_TO_POINT * m_zoom;

    for ( double i = 0.0;i <= ph;i += dist ) {
	str=QString::number(j++);
	if ( unit == "pt" && j!=1 )
	    str+="00";
	p.drawText( ( width() - fm.width( str ) ) * 0.5, double2Int(i) - diffy - fm.height() * 0.5,
		    width(), fm.height(), AlignLeft | AlignTop, str );
    }

    for ( double i = dist * 0.5;i <= ph;i += dist ) {
	int ii=double2Int(i);
	p.drawLine( 5, ii - diffy, width() - 5, ii - diffy );
    }

    for ( double i = dist * 0.25;i <= ph;i += dist *0.5 ) {
	int ii=double2Int(i);
	p.drawLine( 7, ii - diffy, width() - 7, ii - diffy );
    }

    p.setPen( QPen( black ) );
    //p.drawLine( 1, ph - diffy - 1, width() - 1, ph - diffy - 1 );
    p.drawLine( 1, ph - diffy + 1, width() - 1, ph - diffy + 1 );
    p.setPen( QPen( white ) );
    p.drawLine( 1, ph - diffy, width() - 1, ph - diffy );

    p.setPen( QPen( black ) );
    //p.drawLine( 1, -diffy - 2, width() - 1, -diffy - 2 );
    p.drawLine( 1, -diffy, width() - 1, -diffy );
    p.setPen( QPen( white ) );
    p.drawLine( 1, -diffy - 1, width() - 1, -diffy - 1 );

    if ( d->action == A_NONE && showMPos ) {
	p.setPen( QPen( black, 1, SolidLine ) );
	p.drawLine( 1, mposY, width() - 1, mposY );
    }
    hasToDelete = false;

    p.end();
    _painter->drawPixmap( 0, 0, buffer );
}

/*================================================================*/
void KoRuler::mousePressEvent( QMouseEvent *e )
{
    d->oldMx = e->x();
    d->oldMy = e->y();
    d->mousePressed = true;
    d->removeTab=false;

    if ( e->button() == RightButton ) {
	QPoint pnt( QCursor::pos() );
	d->rb_menu->popup( pnt );
	d->action = A_NONE;
	d->mousePressed = false;
	return;
    }

    if ( d->action == A_BR_RIGHT || d->action == A_BR_LEFT ) {
	if ( d->action == A_BR_RIGHT )
	    d->whileMovingBorderRight = true;
	else
	    d->whileMovingBorderLeft = true;

	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
	    p.end();
	}

	repaint( false );
    } else if ( d->action == A_BR_TOP || d->action == A_BR_BOTTOM ) {
	if ( d->action == A_BR_TOP )
	    d->whileMovingBorderTop = true;
	else
	    d->whileMovingBorderBottom = true;

	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( 0, d->oldMy, d->canvas->width(), d->oldMy );
	    p.end();
	}

	repaint( false );
    } else if ( d->action == A_FIRST_INDENT || d->action == A_LEFT_INDENT ) {
	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
	    p.end();
	}
    } else if ( d->action == A_TAB ) {
	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    int pt=zoomIt(d->tabList.at(d->currTab)->ptPos);
	    pt+= (frameStart == -1 ? zoomIt(layout.ptLeft) : zoomIt(frameStart));
	    p.drawLine( pt, 0, pt, d->canvas->height() );
	    p.end();
	}
    } else if ( d->tabChooser && ( d->flags & F_TABS ) && d->tabChooser->getCurrTabType() != 0 ) {
	int pw = static_cast<int>(zoomIt(layout.ptWidth));
	int left = static_cast<int>(zoomIt(layout.ptLeft));
	left -= diffx;
        int right = static_cast<int>(zoomIt(layout.ptRight));
	right = pw - right - diffx;
	
	if( e->x()-left < 0 || right-e->x() < 0 )
	    return;
	KoTabulator *_tab = new KoTabulator;
	switch ( d->tabChooser->getCurrTabType() ) {
	case KoTabChooser::TAB_LEFT:
	    _tab->type = T_LEFT;
	    break;
	case KoTabChooser::TAB_CENTER:
	    _tab->type = T_CENTER;
	    break;
	case KoTabChooser::TAB_RIGHT:
	    _tab->type = T_RIGHT;
	    break;
	case KoTabChooser::TAB_DEC_PNT:
	    _tab->type = T_DEC_PNT;
	    break;
	default: break;
	}
	_tab->ptPos = unZoomIt(e->x() + diffx - ( frameStart == -1 ? static_cast<int>( layout.ptLeft ) : frameStart ));
	_tab->mmPos = cPOINT_TO_MM( _tab->ptPos );
	_tab->inchPos = cPOINT_TO_INCH( _tab->ptPos );

	d->tabList.append( _tab );
	d->removeTab=true;
	emit tabListChanged( &d->tabList );
	repaint( false );
    }
}

/*================================================================*/
void KoRuler::mouseReleaseEvent( QMouseEvent *e )
{
    d->mousePressed = false;

    // Hacky, but necessary to prevent multiple tabs with the same coordinates (Werner)
    if(d->removeTab)
	mouseMoveEvent(e);

    if ( d->action == A_BR_RIGHT || d->action == A_BR_LEFT ) {
	d->whileMovingBorderRight = false;
	d->whileMovingBorderLeft = false;

	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
	    p.end();
	}

	repaint( false );
	emit newPageLayout( layout );
    } else if ( d->action == A_BR_TOP || d->action == A_BR_BOTTOM ) {
	d->whileMovingBorderTop = false;
	d->whileMovingBorderBottom = false;

	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( 0, d->oldMy, d->canvas->width(), d->oldMy );
	    p.end();
	}

	repaint( false );
	emit newPageLayout( layout );
    } else if ( d->action == A_FIRST_INDENT ) {
	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
	    p.end();
	}

	repaint( false );
	emit newFirstIndent( i_first );
    } else if ( d->action == A_LEFT_INDENT ) {
	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
	    p.end();
	}

	repaint( false );
	int _tmp = i_first;
	emit newLeftIndent( i_left );
	i_first = _tmp;
	emit newFirstIndent( i_first );
    } else if ( d->action == A_TAB ) {
	if ( d->canvas ) {
	    QPainter p;
	    p.begin( d->canvas );
	    p.setRasterOp( NotROP );
	    p.setPen( QPen( black, 1, SolidLine ) );
	    int pt=zoomIt(d->tabList.at(d->currTab)->ptPos);
	    pt+= (frameStart == -1 ? zoomIt(layout.ptLeft) : zoomIt(frameStart));
	    p.drawLine( pt, 0, pt, d->canvas->height() );
	    p.end();
	}
	if ( /*tabList.at( currTab )->ptPos + ( frameStart == -1 ? static_cast<int>( layout.ptLeft ) :
	       frameStart ) < layout.ptLeft ||
	      tabList.at( currTab )->ptPos + ( frameStart == -1 ? static_cast<int>( layout.ptLeft ) :
	      frameStart ) > layout.ptWidth -
	      ( layout.ptRight + layout.ptLeft ) || */e->y() < -50 || e->y() > height() + 50 )
	    d->tabList.remove( d->currTab );

	emit tabListChanged( &d->tabList );
	repaint( false );
    }
}

/*================================================================*/
void KoRuler::mouseMoveEvent( QMouseEvent *e )
{
    hasToDelete = false;

    int pw = static_cast<int>(zoomIt(layout.ptWidth));
    int ph = static_cast<int>(zoomIt(layout.ptHeight));
    int left = static_cast<int>(zoomIt(layout.ptLeft));
    left -= diffx;
    int top = static_cast<int>(zoomIt(layout.ptTop));
    top -= diffy;
    int right = static_cast<int>(zoomIt(layout.ptRight));
    right = pw - right - diffx;
    int bottom = static_cast<int>(zoomIt(layout.ptBottom));
    bottom = ph - bottom - diffy;
    int ip_left = zoomIt(i_left);
    int ip_first = zoomIt(i_first);

    int mx = e->x();
    mx = mx+diffx < 0 ? 0 : mx;
    int my = e->y();
    my = my+diffy < 0 ? 0 : my;

    switch ( orientation ) {
	case Qt::Horizontal: {
	    if ( !d->mousePressed ) {
		setCursor( ArrowCursor );
		d->action = A_NONE;
		if ( mx > left - 5 && mx < left + 5 ) {
		    setCursor( sizeHorCursor );
		    d->action = A_BR_LEFT;
		} else if ( mx > right - 5 && mx < right + 5 ) {
		    setCursor( sizeHorCursor );
		    d->action = A_BR_RIGHT;
		}

		if ( d->flags & F_INDENTS ) {
		    if ( mx > left + ip_first - 5 && mx < left + ip_first + 5 &&
			 my >= 2 && my <= d->pmFirst.size().height() + 2 ) {
			setCursor( ArrowCursor );
			d->action = A_FIRST_INDENT;
		    } else if ( mx > left + ip_left - 5 && mx < left + ip_left + 5 &&
				my >=	height() - d->pmLeft.size().height() - 2 && my <= height() - 2 ) {
			setCursor( ArrowCursor );
			d->action = A_LEFT_INDENT;
		    }
		}
	
		if ( d->flags & F_TABS ) {
		    int pos;
		    d->currTab = -1;
		    for ( unsigned int i = 0; i < d->tabList.count(); i++ ) {
			pos = zoomIt(d->tabList.at( i )->ptPos) - diffx + ( frameStart == -1 ?
									     static_cast<int>(zoomIt(layout.ptLeft)) :
									     zoomIt(frameStart));
			if ( mx > pos - 5 && mx < pos + 5 ) {
			    setCursor( sizeHorCursor );
			    d->action = A_TAB;
			    d->currTab = i;
			    break;
			}
		    }
		}
	    } else {
		// Note: All limits should be 0, but KWord crashes currently, when the
		// page is too small! (Werner)
		switch ( d->action ) {
		    case A_BR_LEFT: {
			if ( d->canvas && mx+diffx < right-10 ) {
			    QPainter p;
			    p.begin( d->canvas );
			    p.setRasterOp( NotROP );
			    p.setPen( QPen( black, 1, SolidLine ) );
			    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
			    p.drawLine( mx, 0, mx, d->canvas->height() );
			    layout.left = layout.mmLeft = cPOINT_TO_MM(unZoomIt(mx + diffx));
			    layout.inchLeft = cMM_TO_INCH(layout.mmLeft);
			    layout.ptLeft = unZoomIt(mx + diffx);
			    if( ip_first > right-left-15 ) {
				ip_first=right-left-15;
				ip_first=ip_first<0 ? 0 : ip_first;
				i_first=unZoomIt(ip_first);
				emit newFirstIndent( i_first );
			    }
			    if( ip_left > right-left-15 ) {
				ip_left=right-left-15;
				ip_left=ip_left<0 ? 0 : ip_left;
				i_left=unZoomIt(ip_left);
				emit newLeftIndent( i_left );
			    }
			    d->oldMx = mx;
			    d->oldMy = my;
			    p.end();
			    repaint( false );
			}
		    } break;
		    case A_BR_RIGHT: {
			if ( d->canvas && mx+diffx > left+10 && mx+diffx <= pw) {
			    QPainter p;
			    p.begin( d->canvas );
			    p.setRasterOp( NotROP );
			    p.setPen( QPen( black, 1, SolidLine ) );
			    p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
			    p.drawLine( mx, 0, mx, d->canvas->height() );
			    p.end();
			    layout.right = layout.mmRight = cPOINT_TO_MM(unZoomIt(static_cast<int>( pw - ( mx + diffx )) ) );
			    layout.ptRight = unZoomIt(pw - ( mx + diffx ));
			    layout.inchRight = cPOINT_TO_INCH( layout.ptRight );
			    if( ip_first > right-left-15 ) {
				ip_first=right-left-15;
				ip_first=ip_first<0 ? 0 : ip_first;
				i_first=unZoomIt(ip_first);
				emit newFirstIndent( i_first );
			    }
			    if( ip_left > right-left-15 ) {
				ip_left=right-left-15;
				ip_left=ip_left<0 ? 0 : ip_left;
				i_left=unZoomIt(ip_left);
				emit newLeftIndent( i_left );
			    }
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    case A_FIRST_INDENT: {
			if ( d->canvas ) {
			    if ( mx - left >= 0 && right - mx >= 10 ) {
				QPainter p;
				p.begin( d->canvas );
				p.setRasterOp( NotROP );
				p.setPen( QPen( black, 1, SolidLine ) );
				p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
				p.drawLine( mx, 0, mx, d->canvas->height() );
				p.end();
			    } else
				return;
			    i_first = unZoomIt(mx - left);
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    case A_LEFT_INDENT: {
			if ( d->canvas ) {
			    if ( mx - left >= 0 && right - mx >= 10 ) {
				QPainter p;
				p.begin( d->canvas );
				p.setRasterOp( NotROP );
				p.setPen( QPen( black, 1, SolidLine ) );
				p.drawLine( d->oldMx, 0, d->oldMx, d->canvas->height() );
				p.drawLine( mx, 0, mx, d->canvas->height() );
				p.end();
			    } else
				return;
			    int oldDiff = ip_first-ip_left;
			    ip_left = mx - left;
			    ip_first = ip_left + oldDiff;
			    if( ip_first < 0)
				ip_first=0;
			    else if( ip_first > right-left-10 )
				ip_first=right-left-10;
			    i_left=unZoomIt(ip_left);
			    i_first=unZoomIt(ip_first);
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    case A_TAB: {
			if ( d->canvas && mx-left >= 0 && right-mx >= 0) {
			    QPainter p;
			    p.begin( d->canvas );
			    p.setRasterOp( NotROP );
			    p.setPen( QPen( black, 1, SolidLine ) );
			    int pt=zoomIt(d->tabList.at( d->currTab )->ptPos);
			    int fr=(frameStart == -1 ? static_cast<int>(zoomIt(layout.ptLeft)) : zoomIt(frameStart) );
			    p.drawLine( pt + fr, 0, pt + fr, d->canvas->height() );
			    d->tabList.at( d->currTab )->ptPos += unZoomIt( mx - d->oldMx );
			    d->tabList.at( d->currTab )->mmPos = cPOINT_TO_MM( d->tabList.at( d->currTab )->ptPos );
			    d->tabList.at( d->currTab )->inchPos = cPOINT_TO_INCH( d->tabList.at( d->currTab )->ptPos );
			    pt=zoomIt(d->tabList.at( d->currTab )->ptPos);
			    fr=(frameStart == -1 ? static_cast<int>(zoomIt(layout.ptLeft)) : zoomIt(frameStart) );
			    p.drawLine( pt + fr, 0, pt + fr, d->canvas->height() );
			    p.end();
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    default: break;
		}
	    }
	} break;
	case Qt::Vertical: {
	    if ( !d->mousePressed ) {
		setCursor( ArrowCursor );
		d->action = A_NONE;
		if ( my > top - 5 && my < top + 5 ) {
		    setCursor( sizeVerCursor );
		    d->action = A_BR_TOP;
		} else if ( my > bottom - 5 && my < bottom + 5 ) {
		    setCursor( sizeVerCursor );
		    d->action = A_BR_BOTTOM;
		}
	    } else {
		switch ( d->action ) {
		    case A_BR_TOP: {
			if ( d->canvas && my+diffy < bottom-20 ) {
			    QPainter p;
			    p.begin( d->canvas );
			    p.setRasterOp( NotROP );
			    p.setPen( QPen( black, 1, SolidLine ) );
			    p.drawLine( 0, d->oldMy, d->canvas->width(), d->oldMy );
			    p.drawLine( 0, my, d->canvas->width(), my );
			    p.end();
			    layout.top = layout.mmTop = cPOINT_TO_MM(unZoomIt(my + diffy));
			    layout.ptTop = unZoomIt(my + diffy);
			    layout.inchTop = cPOINT_TO_INCH( layout.ptTop );
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    case A_BR_BOTTOM: {
			if ( d->canvas && my+diffy > top+20 && my+diffy < ph-2) {
			    QPainter p;
			    p.begin( d->canvas );
			    p.setRasterOp( NotROP );
			    p.setPen( QPen( black, 1, SolidLine ) );
			    p.drawLine( 0, d->oldMy, d->canvas->width(), d->oldMy );
			    p.drawLine( 0, my, d->canvas->width(), my );
			    p.end();
			    layout.bottom = layout.mmBottom = cPOINT_TO_MM( static_cast<int>( unZoomIt(ph - ( my + diffy )) ) );
			    layout.ptBottom = unZoomIt(ph - ( my + diffy ));
			    layout.inchBottom = cPOINT_TO_INCH( layout.ptBottom );
			    d->oldMx = mx;
			    d->oldMy = my;
			    repaint( false );
			}
		    } break;
		    default: break;
		}
	    }
	} break;
    }

    d->oldMx = mx;
    d->oldMy = my;
}

/*================================================================*/
void KoRuler::resizeEvent( QResizeEvent *e )
{
    QFrame::resizeEvent( e );
    buffer.resize( size() );
}

/*================================================================*/
void KoRuler::mouseDoubleClickEvent( QMouseEvent* )
{
    if ( d->tabChooser && ( d->flags & F_TABS ) && d->tabChooser->getCurrTabType() != 0 && d->removeTab ) {
	d->tabList.remove( d->tabList.count() - 1 );
	emit tabListChanged( &d->tabList );
	repaint( false );
	d->removeTab=false;
    }

    emit openPageLayoutDia();
}

/*================================================================*/
void KoRuler::setTabList( const QList<KoTabulator>* _tabList )
{
    d->tabList.setAutoDelete( true );
    d->tabList.clear();
    d->tabList.setAutoDelete( false );
    QListIterator<KoTabulator> it( *_tabList );
    for ( it.toFirst(); it.current(); ++it ) {
	KoTabulator *t = new KoTabulator;
	t->type = it.current()->type;
	t->mmPos = it.current()->mmPos;
	t->inchPos = it.current()->inchPos;
	t->ptPos = it.current()->ptPos;
	d->tabList.append( t );
    }
    repaint( false );
}

/*================================================================*/
unsigned int KoRuler::makeIntern( float _v )
{
    if ( unit == "mm" ) return cMM_TO_POINT( _v );
    if ( unit == "inch" ) return cINCH_TO_POINT( _v );
    return static_cast<unsigned int>( _v );
}

/*================================================================*/
void KoRuler::setupMenu()
{
    d->rb_menu = new QPopupMenu();
    CHECK_PTR( d->rb_menu );
    d->mMM = d->rb_menu->insertItem( i18n( "Millimeters (mm)" ), this, SLOT( rbMM() ) );
    d->mPT = d->rb_menu->insertItem( i18n( "Points (pt)" ), this, SLOT( rbPT() ) );
    d->mINCH = d->rb_menu->insertItem( i18n( "Inches (inch)" ), this, SLOT( rbINCH() ) );
    d->rb_menu->insertSeparator();
    d->rb_menu->insertItem(i18n("Page Layout..."), this, SLOT(pageLayoutDia()));
    d->rb_menu->setCheckable( false );
    d->rb_menu->setItemChecked( d->mMM, true );
}

/*================================================================*/
void KoRuler::uncheckMenu()
{
    d->rb_menu->setItemChecked( d->mMM, false );
    d->rb_menu->setItemChecked( d->mPT, false );
    d->rb_menu->setItemChecked( d->mINCH, false );
}

/*================================================================*/
void KoRuler::setUnit( const QString& _unit )
{
    unit = _unit;
    uncheckMenu();

    if ( unit == "mm" ) {
	d->rb_menu->setItemChecked( d->mMM, true );
	layout.unit = PG_MM;
    } else if ( unit == "pt" ) {
	d->rb_menu->setItemChecked( d->mPT, true );
	layout.unit = PG_PT;
    } else if ( unit == "inch" ) {
	d->rb_menu->setItemChecked( d->mINCH, true );
	layout.unit = PG_INCH;
    }
    repaint( false );
}

void KoRuler::setZoom( const double& zoom )
{
    if(zoom==m_zoom)
	return;
    diffx=unZoomIt(diffx);
    diffy=unZoomIt(diffy);
    m_zoom=zoom;
    m_1_zoom=1/m_zoom;
    diffx=zoomIt(diffx);
    diffy=zoomIt(diffy);
    repaint( false );
}

#include "koRuler.moc"
