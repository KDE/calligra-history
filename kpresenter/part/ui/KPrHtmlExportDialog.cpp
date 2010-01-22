/* This file is part of the KDE project
 * Copyright (C) 2009 Yannick Motta <yannick.motta@gmail.com>
 * Copyright (C) 2009 Benjamin Port <port.benjamin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KPrHtmlExportDialog.h"
#include <QtGui/QDesktopWidget>
#include <KLocale>
#include <KDebug>
#include <QDir>
#include <kstandarddirs.h>
#include <QComboBox>
#include <KoDocument.h>
#include <KFileDialog>
#include "KPrHtmlExport.h"
#include <QWebFrame>
#include <QPainter>
#include <KPrView.h>

KPrHtmlExportDialog::KPrHtmlExportDialog(QList<KoPAPageBase*> slides, QString title, QWidget *parent) : KDialog(parent), m_allSlides(slides), m_title(title) 
{
    QWidget *widget = new QWidget( this );
    ui.setupUi( widget );
    setMainWidget( widget );
    setCaption( i18n( "Html Export"));
    setButtonText(Ok, i18n("Export"));
    ui.klineedit_title->setText(m_title);

    connect( ui.kpushbuttonBrowseCSS, SIGNAL( clicked() ), this, SLOT( browserAction()));

    connect( &preview                  , SIGNAL( loadFinished(bool) ), this, SLOT(renderPreview()));
    connect( ui.klineedit_title        , SIGNAL( editingFinished() ), this, SLOT(generatePreview()));
    connect( ui.kListBox_slides        , SIGNAL( currentRowChanged(int) ), this, SLOT(generatePreview(int)));
    connect( ui.kcombobox              , SIGNAL( currentIndexChanged(int) ), this, SLOT(generatePreview()));
    connect( ui.kPushButton_selectAll  , SIGNAL( clicked() ),        this, SLOT( checkAllItems()  ) );
    connect( ui.kPushButton_deselectAll, SIGNAL( clicked() ),        this, SLOT( uncheckAllItems()) );

    this->generateSlidesNames(slides);
    this->loadCssList();
    this->generatePreview();

}

QList<KoPAPageBase*> KPrHtmlExportDialog::checkedSlides()
{
    QList<KoPAPageBase*> qlchekedSlides;
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        if (ui.kListBox_slides->item(i)->checkState() == Qt::Checked){
            qlchekedSlides.append(this->m_allSlides.at(i));
        }
    }
    return qlchekedSlides;
}

void KPrHtmlExportDialog::checkAllItems()
{
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        ui.kListBox_slides->item(i)->setCheckState(Qt::Checked);
    }
}

void KPrHtmlExportDialog::uncheckAllItems()
{
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        ui.kListBox_slides->item(i)->setCheckState(Qt::Unchecked);
    }
}

QStringList KPrHtmlExportDialog::slidesNames(){
    QStringList names;
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        if (ui.kListBox_slides->item(i)->checkState() == Qt::Checked){
            names.append(ui.kListBox_slides->item(i)->text());
        }
    }
    return names;
}

KUrl KPrHtmlExportDialog::css(){
    return KUrl(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
}

void KPrHtmlExportDialog::generateSlidesNames(QList<KoPAPageBase*> slides)
{
    QString slideName;
    for(int i = 0; i < slides.count(); i++){
        if (slides.at(i)->name().isEmpty()){
            slideName = i18n("Slide %1", QString::number(i+1));
        }
        else {
            slideName = slides.at(i)->name();
        }
        QListWidgetItem *listItem = new  QListWidgetItem(slideName);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        listItem->setCheckState(Qt::Checked);
        ui.kListBox_slides->addItem(listItem);
    }
}

void KPrHtmlExportDialog::loadCssList()
{
    KStandardDirs std;
    QStringList dirs = std.findDirs("data", "kpresenter/templates/exportHTML");
    for( QStringList::ConstIterator path=dirs.begin(); path!=dirs.end(); ++path ){
        QDir dir(*path);
        dir.setFilter( QDir::Dirs );
        QStringList entries = dir.entryList();
        for( QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry ){
            if (*entry != "." && *entry != ".."){
                if (dir.exists(*entry + "/style.css")){
                    // Rajouter les items inselectionnables
		        	ui.kcombobox->addItem(QString(*entry),QVariant(QString(*path + *entry + "/style.css")));
                } 
            }
        }        
    }
}

void KPrHtmlExportDialog::browserAction(){
    KFileDialog dialog(KUrl("/"),QString("*.css"),this);
	if ( dialog.exec() == QDialog::Accepted ){
        QString name=dialog.selectedFile();
        if (name.endsWith(QString(".css"),Qt::CaseInsensitive)){
		    ui.kcombobox->addItem(name,QVariant(name));
        }
   }
}

QString KPrHtmlExportDialog::title()
{
    return  ui.klineedit_title->text();
}

void KPrHtmlExportDialog::generatePreview(int item) {
    if(item < 0) {
        item = 0;
    }

    KPrHtmlExport previewGenerator;
    ExportParameter previewParameters;
    previewParameters.author = "John Doe";
    previewParameters.cssUrl = this->css();
    previewParameters.dest_url = "";
    previewParameters.slides.append(this->m_allSlides.at(item));
    previewParameters.slidesNames.append(ui.kListBox_slides->item(item)->text());
    previewParameters.title = ui.klineedit_title->text();
    previewParameters.kprView = (KPrView*)this->parentWidget();
    KUrl url;

    previewGenerator.exportPreview(previewParameters, url);
    preview.mainFrame()->load(url);
}

void KPrHtmlExportDialog::renderPreview()
{
    preview.setViewportSize(preview.currentFrame()->contentsSize());
    QImage image(preview.viewportSize(), QImage::Format_ARGB32);
    QPainter painter(&image);

    preview.mainFrame()->render(&painter);
    painter.end();

    QImage thumbnail = image.scaled(ui.qLabel_preview->size(), Qt::KeepAspectRatio);
    ui.qLabel_preview->setPixmap(QPixmap::fromImage(thumbnail));
}
