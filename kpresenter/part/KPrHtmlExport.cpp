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

#include "KPrHtmlExport.h"
#include "KPrPage.h"
#include <kio/copyjob.h>
#include <ktempdir.h>

KPrHtmlExport::KPrHtmlExport (KPrView* kprView, KoPADocument* kopaDocument, const KUrl &url ):m_kprView(kprView), m_kopaDocument(kopaDocument), m_dest_url(url)
{ 
    // Create a temporary dir
    KTempDir tmpDir;
    m_tmpDirPath = tmpDir.name();
    tmpDir.setAutoRemove(false);
    exportImageToTmpDir();
    copyFromTmpToDest();
}


KPrHtmlExport::~KPrHtmlExport()
{
}


void KPrHtmlExport::exportImageToTmpDir()
{
    // Export slides as image into the temporary export directory
    KUrl fileUrl;
    int i=0;
    foreach(KoPAPageBase* slide, m_kopaDocument->pages()) {
        fileUrl = m_tmpDirPath;
        fileUrl.addPath("slide" + QString::number(i) + ".png");
        m_kprView->exportPageThumbnail(slide,fileUrl, slide->size().toSize(),"PNG",-1);
        m_fileUrlList.append(fileUrl);
        i++;
    }
}

void KPrHtmlExport::writeHtmlFileToTmpDir(const QString &fileName, const QString &htmlBody)
{
    QTextStream stream;
    QFile file;
    KUrl fileUrl = m_tmpDirPath;
    fileUrl.addPath(fileName);
    file.setFileName(fileUrl.toLocalFile());
    file.open(QIODevice::WriteOnly);
    stream.setDevice(&file);
    stream << htmlBody;
    stream.flush();
    file.close();
}

void KPrHtmlExport::copyFromTmpToDest()
{
    KIO::CopyJob *job = KIO::move(m_fileUrlList, m_dest_url);
    connect( job, SIGNAL(result( KJob * )), this, SLOT( moveResult( KJob * ) ) );
    job->exec();
}

void KPrHtmlExport::moveResult(KJob *job)
{
    KTempDir::removeDir(m_tmpDirPath);
}

#include "KPrHtmlExport.moc"
