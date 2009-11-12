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

#ifndef KPRHTMLEXPORT_H
#define KPRHTMLEXPORT_H
#include <KoPADocument.h>
#include "KPrView.h"

class KPrHtmlExport:public QObject
{
    Q_OBJECT
    public:
        /**
        * @param kprView
        * @param kopaDocument 
        * @param url The destination url
        */
        KPrHtmlExport(KPrView* kprView, KoPADocument* kopaDocument, const KUrl& url);
        ~KPrHtmlExport();
    private slots:
        void moveResult(KJob *job);
    protected:
        void exportImageToTmpDir();
        void writeHtmlFileToTmpDir(const QString &fileName, const QString &htmlBody);
        void copyFromTmpToDest();
    private:
        KPrView *m_kprView;
        KoPADocument *m_kopaDocument;
        KUrl m_dest_url;
        KUrl::List m_fileUrlList;
        QString m_tmpDirPath;
};
#endif /* KPRHTMLEXPORT_H */
