/*
 *  kis_util.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
 *                1999 Andrew Richards <A.Richards@phys.canterbury.ac.nz>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <kglobal.h>
#include <klocale.h>
#include <kimageio.h>

#include "kis_global.h"
#include "kis_util.h"

// A number which can be added to any image coordinate to make it positive
// Used to make numbers round towards + or - infinity regardless of sign
const long BIGNUM = (TILE_SIZE*10000);

void KisUtil::printRect( const QRect& r, const QString& name )
{
  qDebug("%s:: l:%d t:%d r:%d b:%d w:%d h:%d", name.latin1(), r.left(), r.top(), r.right(),
	 r.bottom(), r.width(), r.height());
}

void KisUtil::printPoint( const QPoint& p, const QString& name )
{
  qDebug("%s:: x:%d y:%d", name.latin1(), p.x(), p.y()); 
}

void KisUtil::enlargeRectToContainPoint( QRect& r, QPoint p )
{
  if (r.contains(p))
    {
      qDebug("KisUtil::enlargeRectToContainPoint: point already contained\n");	
      return;
    }
  if (p.x()<r.left())   r.setLeft(p.x());
  if (p.x()>r.right())  r.setRight(p.x());
  if (p.y()<r.top())    r.setTop(p.y());
  if (p.y()>r.bottom()) r.setBottom(p.y());
}

// Find a rectangle which encloses r whose coordinates are divisible
// by TILE_SIZE (ie no remainder)
QRect KisUtil::findTileExtents( QRect r )
{
  r.setLeft(((r.left()+BIGNUM)/TILE_SIZE)*TILE_SIZE-BIGNUM);
  r.setTop(((r.top()+BIGNUM)  /TILE_SIZE)*TILE_SIZE-BIGNUM);
  r.setBottom(((r.bottom()+TILE_SIZE+BIGNUM)/TILE_SIZE)*TILE_SIZE-BIGNUM-1);
  r.setRight(((r.right()+TILE_SIZE+BIGNUM)  /TILE_SIZE)*TILE_SIZE-BIGNUM-1);
  return(r);
}

QString KisUtil::channelIdtoString(cId cid)
{
  switch (cid)
	{
	case ci_Indexed:
	  return i18n("indexed");
	  break;
	case ci_Alpha:
	  return ("alpha");
	  break;
	case ci_Red:
	  return i18n("red");
	  break;
	case ci_Green:
	  return i18n("green");
	  break;
	case ci_Blue:
	  return i18n("blue");
	  break;
	case ci_Cyan:
	  return i18n("cyan");
	  break;
	case ci_Magenta:
	  return i18n("magenta");
	  break;
	case ci_Yellow:
	  return i18n("yellow");
	  break;
	case ci_Black:
	  return i18n("black");
	  break;
	case ci_L:
	  return "L";
	  break;
	case ci_a:
	  return "a";
	  break;
	case ci_b:
	  return "b";
	  break;
	default:
	  return "unknown";
	  break;
	}
}

QString KisUtil::readFilters()
{
  return KImageIO::pattern( KImageIO::Reading ) + "\n*.kis|KImageShop picture\n";
}

QString KisUtil::writeFilters()
{
  return KisUtil::readFilters();
}
