/* This file is part of the KDE project
   Copyright (C) 2002, The Karbon Developers
*/

#include <qdom.h>

#include "vgradient.h"

VGradient::VGradient( VGradientType type )
	: m_type( type )
{
}

void
VGradient::addStop( const VColor &color, float rampPoint )
{
	VColorStop stop;
	stop.color = color;
	stop.rampPoint = rampPoint;
	m_colorStops.append( stop );
}

void
VGradient::save( QDomElement& element ) const
{
	QDomElement me = element.ownerDocument().createElement( "GRADIENT" );
	me.setAttribute( "originx", m_origin.x() );
	me.setAttribute( "originy", m_origin.y() );
	me.setAttribute( "vectorx", m_vector.x() );
	me.setAttribute( "vectory", m_vector.y() );
	me.setAttribute( "type", m_type );
	me.setAttribute( "spreadmethod", m_spreadMethod );

	// save stops
	QDomElement stops = element.ownerDocument().createElement( "COLORSTOPS" );
	QValueListConstIterator<VColorStop> itr;
	for( itr = m_colorStops.begin(); itr != m_colorStops.end(); ++itr )
	{
		QDomElement stop = element.ownerDocument().createElement( "COLORSTOP" );
		(*itr).color.save( stop );
		stop.setAttribute( "ramppoint", (*itr).rampPoint );
		stops.appendChild( stop );
	}

	me.appendChild( stops );

	element.appendChild( me );
}

void
VGradient::load( const QDomElement& element )
{
	m_origin.setX( element.attribute( "originx", "0.0" ).toDouble() );
	m_origin.setY( element.attribute( "originy", "0.0" ).toDouble() );
	m_vector.setX( element.attribute( "vectorx", "0.0" ).toDouble() );
	m_vector.setY( element.attribute( "vectory", "0.0" ).toDouble() );
	m_type = element.attribute( "type", 0 ).toInt();
	m_spreadMethod = element.attribute( "spreadmethod", 0 ).toInt();

	m_colorStops.clear();

	// load stops
	QDomNodeList list = element.childNodes();
	for( uint i = 0; i < list.count(); ++i )
	{
		if( list.item( i ).isElement() )
		{
			QDomElement child = list.item( i ).toElement();

			if( child.tagName() == "COLORSTOPS" )
			{
				QDomNodeList colorstops = child.childNodes();
				for( uint j = 0; j < colorstops.count(); ++j )
				{
					if( colorstops.item( j ).isElement() )
					{
						QDomElement colorstop = colorstops.item( j ).toElement();

						if( colorstop.tagName() == "COLORSTOP" )
						{
							VColorStop stop;
							stop.color.load( colorstop.firstChild().toElement() );
							stop.rampPoint = colorstop.attribute( "ramppoint", "0.0" ).toDouble();
							m_colorStops.append( stop );
						}
					}
				}
			}
		}
	}
}

