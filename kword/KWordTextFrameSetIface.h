/* This file is part of the KDE project
   Copyright (C) 2002, Laurent MONTEL <lmontel@mandrakesoft.com>

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

#ifndef KWORD_TEXTFRAMESET_IFACE_H
#define KWORD_TEXTFRAMESET_IFACE_H

#include <KoDocumentIface.h>
#include <dcopref.h>

#include <qstring.h>
#include <qcolor.h>
#include <qfont.h>
#include "KWordFrameSetIface.h"
class KWTextFrameSet;
class KWordViewIface;

class KWordTextFrameSetIface :  virtual public KWordFrameSetIface
{
    K_DCOP
public:
    KWordTextFrameSetIface( KWTextFrameSet *_frametext );

k_dcop:
    virtual DCOPRef startEditing();
    bool hasSelection() const;
    int numberOfParagraphs();
    int paragraphsSelected();
    QString name() const;
    QString selectedText() const;
    void selectAll( bool select );
    void setBoldText( bool b );
    void setItalicText( bool b );
    void setUnderlineText( bool b );
    void setStrikeOutText( bool b );
    void setTextColor( const QColor &col );
    void setTextPointSize( int s );
    void setTextSubScript( bool b );
    void setTextSuperScript( bool b );
    void setTextDefaultFormat();
    void setTextBackgroundColor(const QColor & col);
    void setTextFamilyFont(const QString &font);

    void setDoubleUnderlineText( bool b );

    void changeCaseOfText( const QString & caseType);

    QColor textColor() const;
    QFont textFont() const;
    QString textFontFamily()const;
    QColor textBackgroundColor()const;

    bool textDoubleUnderline() const;

    bool textItalic() const;
    bool textBold() const;
    bool textUnderline()const;
    bool textStrikeOut()const;
    bool textSubScript() const;
    bool textSuperScript() const;


private:
    KWTextFrameSet *m_frametext;

};

#endif
