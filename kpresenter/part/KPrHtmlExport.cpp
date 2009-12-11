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

KPrHtmlExport::KPrHtmlExport ()
{ 

}

void KPrHtmlExport::init (KPrView* kprView, QList<KoPAPageBase*> slides, const KUrl &url, const QString& author, const QString& title, const KUrl css, QStringList slidesNames)
{ 
    m_kprView = kprView;
    m_slides = slides;
    m_dest_url = url;
    m_author = author;
    m_title = title;
    m_css = css;
    m_slidesNames = slidesNames;
    
    // Create a temporary dir
    KTempDir tmpDir;
    m_tmpDirPath = tmpDir.name();
    tmpDir.setAutoRemove(false);
    exportImageToTmpDir();
    generateHtml();
    generateToc();
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
    foreach(KoPAPageBase* slide, m_slides) {
        fileUrl = m_tmpDirPath;
        fileUrl.addPath("slide" + QString::number(i) + ".png");
        m_kprView->exportPageThumbnail(slide,fileUrl, slide->size().toSize(),"PNG",-1);
        m_fileUrlList.append(fileUrl);
        i++;
    }
}

void KPrHtmlExport::generateHtml()
{
    KUrl fileUrl;
    int i=0;
    QFile file;
    QString fileName = KStandardDirs::locate( "data","kpresenter/templates/exportHTML/slides.html" );
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    QString slideContent = file.readAll();
    file.close();
    int nbSlides = m_slides.size();
    foreach(KoPAPageBase* slide, m_slides) {
        QString content = slideContent;
        content.replace("::TITLE::", m_title);
        content.replace("::AUTHOR::", m_author);
        content.replace("::IMAGE_PATH::", "slide"+QString::number(i)+".png");
        content.replace("::SLIDE_NUM::", QString::number(i+1));
        content.replace("::NB_SLIDES::", QString::number(nbSlides));
        content.replace("::TITLE_SLIDE::", slide->name());
        content.replace("::LAST_PATH::", "slide"+QString::number(nbSlides-1)+".html");
        content.replace("::NEXT_PATH::", "slide"+QString::number((((i+1) < nbSlides) ? i+1 : i))+".html");
        content.replace("::PREVIOUS_PATH::", "slide"+QString::number(((i>0) ? i-1 : 0))+".html");
        content.replace("::FIRST_PATH::", "slide"+QString::number(0)+".html");
        writeHtmlFileToTmpDir("slide" + QString::number(i) + ".html", content);
        i++;
    }
    QString style = m_css.pathOrUrl();
    QFile styleFile;
    styleFile.setFileName(style);
    styleFile.open(QIODevice::ReadOnly);
    writeHtmlFileToTmpDir("style.css", styleFile.readAll());
    styleFile.close();
}

void KPrHtmlExport::generateToc()
{
    QString toc = "<ul>";
    int i = 0; 
    foreach(QString slide, m_slidesNames) {
        toc.append("<li><a href=\"slide"+QString::number(i)+".html\">"+slide+"</a></li>");
        i++;
    }
    toc.append("</ul>");
    QString fileName = KStandardDirs::locate( "data","kpresenter/templates/exportHTML/toc.html" );
    QFile file;
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    QString content = file.readAll();
    file.close();
    int nbSlides = m_slides.size();
    content.replace("::TITLE::", m_title);
    content.replace("::AUTHOR::", m_author);
    content.replace("::SLIDE_NUM::", QString::number(i+1));
    content.replace("::NB_SLIDES::", QString::number(nbSlides));
    content.replace("::LAST_PATH::", "slide"+QString::number(nbSlides-1)+".html");
    content.replace("::NEXT_PATH::", "slide"+QString::number(0)+".html");
    content.replace("::TOC::", toc);
    writeHtmlFileToTmpDir("index.html", content);
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
    m_fileUrlList.append(fileUrl);
}

void KPrHtmlExport::copyFromTmpToDest()
{
    KIO::CopyJob *job = KIO::move(m_fileUrlList, m_dest_url);
    job->setUiDelegate(new KPrHtmlExportUiDelegate);
    connect( job, SIGNAL(result(KJob *)), this, SLOT(moveResult(KJob *)));
    job->exec();
}

void KPrHtmlExport::moveResult(KJob *job)
{
    KTempDir::removeDir(m_tmpDirPath);
    if(job->error()){
        KMessageBox::error(m_kprView,job->errorText());
    }
    else {
        if(KMessageBox::questionYesNo(0,i18n("Open in browser"),i18n("Export successfull")) == KMessageBox::Yes){
            KUrl url = m_dest_url;
            url.addPath("index.html");
            KRun::runUrl(url, "text/html", m_kprView);
        }
    }
}

#include "KPrHtmlExport.moc"
