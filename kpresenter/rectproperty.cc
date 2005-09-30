// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C) 2005 Thorsten Zachmann <zachmann@kde.org>

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

#include "rectproperty.h"

#include <qspinbox.h>
#include <qtoolbutton.h>

#include "rectpropertyui.h"
#include "rectpreview.h"

namespace {
    /* XPM -- copyright The Gimp */
    const char *chain_broken_24[] = {
    /* columns rows colors chars-per-pixel */
    "9 24 10 1",
    "  c black",
    ". c #020204",
    "X c #5A5A5C",
    "o c gray43",
    "O c #8F8F91",
    "+ c #9A9A98",
    "@ c #B5B5B6",
    "# c #D0D0D1",
    "$ c #E8E8E9",
    "% c None",
    /* pixels */
    "%%.....%%",
    "%.o##@X.%",
    "%.+...$.%",
    "%.#.%.#.%",
    "%.#.%.#.%",
    "%.@.%.#.%",
    "%.+...#.%",
    "%.O.o.O.%",
    "%%..@..%%",
    "%%%.#.%%%",
    "%%%%%%%%%",
    "%%%%%%%%%",
    "%%%%%%%%%",
    "%%%%%%%%%",
    "%%%.#.%%%",
    "%%..#..%%",
    "%.o.@.O.%",
    "%.@...@.%",
    "%.@.%.$.%",
    "%.@.%.$.%",
    "%.@.%.$.%",
    "%.#...$.%",
    "%.o$#$@.%",
    "%%.....%%"
    };
    
    /* XPM  -- copyright The Gimp */
    const char *chain_24[] = {
    /* columns rows colors chars-per-pixel */
    "9 24 10 1",
    "  c black",
    ". c #020204",
    "X c #5A5A5C",
    "o c gray43",
    "O c #8F8F91",
    "+ c #9A9A98",
    "@ c #B5B5B6",
    "# c #D0D0D1",
    "$ c #E8E8E9",
    "% c None",
    /* pixels */
    "%%%%%%%%%",
    "%%%%%%%%%",
    "%%.....%%",
    "%.o##@X.%",
    "%.+...$.%",
    "%.#.%.#.%",
    "%.#.%.#.%",
    "%.@.%.#.%",
    "%.+...#.%",
    "%.O.o.O.%",
    "%%..@..%%",
    "%%%.#.%%%",
    "%%%.#.%%%",
    "%%..#..%%",
    "%.o.@.O.%",
    "%.@...@.%",
    "%.@.%.$.%",
    "%.@.%.$.%",
    "%.@.%.$.%",
    "%.#...$.%",
    "%.o$#$@.%",
    "%%.....%%",
    "%%%%%%%%%",
    "%%%%%%%%%"
    };


}

RectProperty::RectProperty( QWidget *parent, const char *name, RectValueCmd::RectValues &rectValue )
: QWidget( parent, name )
, m_rectValue( rectValue )
{
    formerVerticalValue = 0;
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->addWidget( m_ui = new RectPropertyUI( this ) );
    m_ui->combineButton->setPixmap(chain_24);

    connect( m_ui->xRndInput, SIGNAL( valueChanged( int ) ), this, SLOT( slotRndChanged() ) );
    connect( m_ui->yRndInput, SIGNAL( valueChanged( int ) ), this, SLOT( slotRndChanged() ) );
    connect( m_ui->combineButton, SIGNAL( toggled( bool ) ), this, SLOT( combineToggled( bool ) ) );

    slotReset();
}

RectProperty::~RectProperty()
{
}


int RectProperty::getRectPropertyChange() const
{
    int flags = 0;

    if ( getXRnd() != m_rectValue.xRnd )
        flags |= RectValueCmd::XRnd;

    if ( getYRnd() != m_rectValue.yRnd )
        flags |= RectValueCmd::YRnd;

    return flags;
}


RectValueCmd::RectValues RectProperty::getRectValues() const
{
    RectValueCmd::RectValues rectValue;
    rectValue.xRnd = getXRnd();
    rectValue.yRnd = getYRnd();

    return rectValue;
}


void RectProperty::setRectValues( const RectValueCmd::RectValues &rectValues )
{
    m_rectValue = rectValues;
    slotReset();
}


void RectProperty::apply()
{
    int flags = getRectPropertyChange();

    if ( flags & RectValueCmd::XRnd )
        m_rectValue.xRnd = getXRnd();

    if ( flags & RectValueCmd::YRnd )
        m_rectValue.yRnd = getYRnd();
}


int RectProperty::getXRnd() const
{
    return m_ui->xRndInput->value();
}


int RectProperty::getYRnd() const
{
    return m_ui->yRndInput->value();
}


void RectProperty::slotRndChanged()
{
    m_ui->rectPreview->setRnds( getXRnd(), getYRnd() );
}

void RectProperty::slotReset()
{
    m_ui->xRndInput->setValue( m_rectValue.xRnd );
    m_ui->yRndInput->setValue( m_rectValue.yRnd );
    if(m_rectValue.xRnd == m_rectValue.yRnd)
        combineToggled(true);

    m_ui->rectPreview->setRnds( getXRnd(), getYRnd() );
}

void RectProperty::combineToggled( bool on)
{
    if( on ) {
        formerVerticalValue = getYRnd();
        m_ui->yRndInput->setValue( getXRnd() );
        connect(m_ui->yRndInput, SIGNAL( valueChanged ( int ) ),
                m_ui->xRndInput, SLOT( setValue ( int ) ));
        connect(m_ui->xRndInput, SIGNAL( valueChanged ( int ) ),
                m_ui->yRndInput, SLOT( setValue ( int ) ));
        m_ui->combineButton->setPixmap(chain_24);
    }
    else {
        disconnect(m_ui->yRndInput, SIGNAL( valueChanged ( int ) ),
                m_ui->xRndInput, SLOT( setValue ( int ) ));
        disconnect(m_ui->xRndInput, SIGNAL( valueChanged ( int ) ),
                m_ui->yRndInput, SLOT( setValue ( int ) ));
        if(formerVerticalValue != 0)
            m_ui->yRndInput->setValue( formerVerticalValue );
        m_ui->combineButton->setPixmap(chain_broken_24);
    }
}

#include "rectproperty.moc"
