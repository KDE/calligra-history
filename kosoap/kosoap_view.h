/* This file is part of the KDE project
   Copyright (C) 2001 Ian Reinhart Geiser <geiseri@yahoo.com>
   This is based off of the KOffice Example found in the KOffice
   CVS.  Torben Weis <weis@kde.org> is the original author.

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

#ifndef KOSoap_VIEW
#define KOSoap_VIEW

#include <koView.h>

class KAction;
class QPaintEvent;

class KOSoapPart;

class KOSoapView : public KoView
{
    Q_OBJECT
public:
    KOSoapView( KOSoapPart* part, QWidget* parent = 0, const char* name = 0 );

protected slots:
    void copy();
    void refresh();
protected:
    void paintEvent( QPaintEvent* );

    virtual void updateReadWrite( bool readwrite );

private:
    //KAction* m_cut;
};

#endif
