/* -*- C++ -*-

  $Id$

  This file is part of KIllustrator.
  Copyright (C) 1998 Kai-Uwe Sattler (kus@iti.cs.uni-magdeburg.de)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by  
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU Library General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef GPixmap_h_
#define GPixmap_h_

#include <qobject.h>
#include <qcolor.h>
#include <qwmatrix.h>
#include <qpainter.h>
#include <qdstream.h>
#include <qpicture.h>

#include <kurl.h>

#include "Coord.h"
#include "GObject.h"

class GPixmap : public GObject {
  Q_OBJECT
public:
  GPixmap (const char* filename);
  GPixmap (const list<XmlAttribute>& attribs);

  GPixmap ();
  GPixmap (const GPixmap& obj);
  ~GPixmap ();
  
  virtual void draw (Painter& p, bool withBasePoints = false,
		     bool outline = false);

  virtual const char* typeName ();

  virtual GObject* copy ();

  virtual void writeToXml (XmlWriter&);

protected:
  void calcBoundingBox ();

private:
  QPixmap *pix;
  KURL url;
  float width, height;
};

#endif

