/* This file is part of the KDE project
   Copyright (C) 2009 Benjamin Port <port.benjamin@gmail.com>
   Copyright (C) 2009 Yannick Motta <yannick.motta@gmail.com>

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
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <krun.h>
#include "KPrHtmlExportUiDelegate.h"
#include "KPrView.h"
#include <KoPADocument.h>

#include <kdebug.h>
KPrHtmlExport::KPrHtmlExport()
{
}

KPrHtmlExport::~KPrHtmlExport()
{
}

void KPrHtmlExport::exportHtml(const KPrHtmlExport::Parameter & parameters)
{ 
    m_parameters = parameters;
    
    // Create a temporary dir
    KTempDir tmpDir;
    m_tmpDirPath = tmpDir.name();
    tmpDir.setAutoRemove(false);
    exportImageToTmpDir();
    generateHtml();
    generateToc();
    copyFromTmpToDest();
}

KUrl KPrHtmlExport::exportPreview(Parameter parameters) {
    m_parameters = parameters;

    // Create a temporary dir
    KTempDir tmpDir;
    m_tmpDirPath = tmpDir.name();
    tmpDir.setAutoRemove(false);
    exportImageToTmpDir();
    generateHtml();

    KUrl previewUrl;
    previewUrl.setPath(tmpDir.name());
    previewUrl.addPath("slide0.html");
    return previewUrl;
}

void KPrHtmlExport::exportImageToTmpDir()
{
    // Export slides as image into the temporary export directory
    KUrl fileUrl;
    for(int i=0; i < m_parameters.slides.size(); i++){
        fileUrl = m_tmpDirPath;
        fileUrl.addPath(QString( "slide%1.png" ).arg(i));
        KoPAPageBase* slide = m_parameters.slides.at(i);
        m_parameters.kprView->exportPageThumbnail(slide,fileUrl, slide->size().toSize(),"PNG",-1);
        m_fileUrlList.append(fileUrl);
    }
}

void KPrHtmlExport::generateHtml()
{
    QFile file(KStandardDirs::locate( "data", "kpresenter/templates/exportHTML/slides.html" ));
    file.open(QIODevice::ReadOnly);
    QString slideContent = file.readAll();
    file.close();
    int nbSlides = m_parameters.slides.size();
    for(int i=0; i < m_parameters.slidesNames.size();i++){
        QString content = slideContent;
        content.replace("::TITLE::", m_parameters.title);
        content.replace("::AUTHOR::", m_parameters.author);
        content.replace("::IMAGE_PATH::", QString("slide%1.png").arg(i));
        content.replace("::SLIDE_NUM::", QString("%1").arg(i+1));
        content.replace("::NB_SLIDES::", QString("%1").arg(nbSlides));
        content.replace("::TITLE_SLIDE::", m_parameters.slidesNames.at(i));
        content.replace("::LAST_PATH::", QString("slide%1.html").arg(nbSlides-1));
        content.replace("::NEXT_PATH::", QString("slide%1.html").arg(((i+1) < nbSlides) ? i+1 : i));
        content.replace("::PREVIOUS_PATH::", QString("slide%1.html").arg((i>0) ? i-1 : 0));
        content.replace("::FIRST_PATH::", QString("slide%1.html").arg(0));
        writeHtmlFileToTmpDir(QString("slide%1.html").arg(i), content);
    }
    QString style = m_parameters.cssUrl.pathOrUrl();
    QFile styleFile;
    styleFile.setFileName(style);
    styleFile.open(QIODevice::ReadOnly);
    writeHtmlFileToTmpDir("style.css", styleFile.readAll());
    styleFile.close();
}

void KPrHtmlExport::generateToc()
{
    QString toc = "<ul>";
    for(int i=0; i < m_parameters.slidesNames.size(); i++){
        toc.append(QString("<li><a href=\"slide%1.html\">%2</a></li>").arg(i).arg(m_parameters.slidesNames.at(i)));
    }
    toc.append("</ul>");
    QFile file(KStandardDirs::locate( "data", "kpresenter/templates/exportHTML/toc.html" ));
    file.open(QIODevice::ReadOnly);
    QString content = file.readAll();
    file.close();
    content.replace("::TITLE::", m_parameters.title);
    content.replace("::AUTHOR::", m_parameters.author);
    content.replace("::TOC::", toc);
    writeHtmlFileToTmpDir("index.html", content);
}

void KPrHtmlExport::writeHtmlFileToTmpDir(const QString &fileName, const QString &htmlBody)
{
    KUrl fileUrl = m_tmpDirPath;
    fileUrl.addPath(fileName);
    QFile file(fileUrl.toLocalFile());
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream << htmlBody;
    stream.flush();
    file.close();
    m_fileUrlList.append(fileUrl);
}

void KPrHtmlExport::copyFromTmpToDest()
{
    KIO::CopyJob *job = KIO::move(m_fileUrlList, m_parameters.destination);
    job->setUiDelegate(new KPrHtmlExportUiDelegate);
    connect(job, SIGNAL(result(KJob *)), this, SLOT(moveResult(KJob *)));
    job->exec();
}

void KPrHtmlExport::moveResult(KJob *job)
{
    KTempDir::removeDir(m_tmpDirPath);
    if(job->error()) {
        KMessageBox::error(m_parameters.kprView, job->errorText());
    }
    else {
        if(KMessageBox::questionYesNo(0, i18n("Open in browser"), i18n("Export successfull")) == KMessageBox::Yes){
            KUrl url = m_parameters.destination;
            url.addPath("index.html");
            KRun::runUrl(url, "text/html", m_parameters.kprView);
        }
    }
}

#include "KPrHtmlExport.moc"
