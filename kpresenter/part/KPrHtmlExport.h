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

#ifndef KPRHTMLEXPORT_H
#define KPRHTMLEXPORT_H
#include <QObject>
#include <KUrl>
#include <QStringList>
#include <QWebPage>

class KPrView;
class KoPAPageBase;
class KJob;

struct ExportParameter
{
    KUrl cssUrl;
    KPrView *kprView;
    QList<KoPAPageBase*> slides;
    KUrl dest_url;
    QString author;
    QString title;
    QStringList slidesNames;

};
class KPrHtmlExport : public QObject
{
Q_OBJECT
public:
    /**
    * @param kprView
    * @param kopaDocument 
    * @param url The destination url
    */
    KPrHtmlExport();
    ~KPrHtmlExport();
    void exportHtml(const ExportParameter parameters);

    /**
     * Generates a preview of 1 frame into a tempoary directory
     * @param parameters Presentation data (only 1 slide should be provided in "slides" filed)
     * @param previewUrl  URL of output html
     */
    void exportPreview(ExportParameter parameters, KUrl &previewUrl);
protected:
    void generateHtml();
    void generateToc();
    void exportImageToTmpDir();
    void writeHtmlFileToTmpDir(const QString &fileName, const QString &htmlBody);
    void copyFromTmpToDest();
private slots:
    void moveResult(KJob *job);
private:
  
    KUrl::List m_fileUrlList;
    QString m_tmpDirPath;
    ExportParameter m_parameters;
};
#endif /* KPRHTMLEXPORT_H */
