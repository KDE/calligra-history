/* This file is part of the KDE project
   Copyright (C) 2009 Benjamin Port <port.benjamin@gmail.com>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KPrHTMLExport.h"

#include "KPrPage.h"

#include <ktemporaryfile.h>
#include <kio/netaccess.h>

KPrHTMLExport::KPrHTMLExport ( KoPADocument* kopaDocument, const KUrl &url ):m_kopaDocument(kopaDocument), m_url(url)
{
    exportImage();
}


KPrHTMLExport::~KPrHTMLExport()
{

}


void KPrHTMLExport::exportImage()
{
    // Export slides as image into the export directory
    KUrl fileUrl;
    QString tmpFileName;
    int pages = m_kopaDocument->pageCount();
    for(int i=0; i < pages; i++) {
        KoPAPageBase *slide = m_kopaDocument->pageByIndex(i, false);
        QPixmap pixmap = m_kopaDocument->pageThumbnail(slide, slide->size().toSize());
        fileUrl = m_url;
        fileUrl.addPath(QString::number(i) + ".png");
        if(m_url.isLocalFile()) {
            pixmap.save(fileUrl.path(), "PNG");
        } else {
            KTemporaryFile tmpFile;
            tmpFile.setAutoRemove(true);
            tmpFile.open();
            tmpFileName=tmpFile.fileName();
            pixmap.save(tmpFileName, "PNG");
            KIO::NetAccess::upload(tmpFileName, fileUrl, NULL);
        }
    }
}

