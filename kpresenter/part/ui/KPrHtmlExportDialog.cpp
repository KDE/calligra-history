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

KPrHtmlExportDialog::KPrHtmlExportDialog(QList<KoPAPageBase*> slides, QString title, QString author, QWidget *parent)
    : KDialog(parent)
    , m_allSlides(slides)
    , m_title(title)
{
    QWidget *widget = new QWidget( this );
    ui.setupUi( widget );
    setMainWidget( widget );
    setCaption( i18n( "Html Export"));
    setButtonText(Ok, i18n("Export"));
    ui.klineedit_title->setText(m_title);
    ui.klineedit_author->setText(author);

    connect( ui.kpushbuttonBrowseCSS, SIGNAL( clicked() ), this, SLOT( browserAction()));

    connect( &preview, SIGNAL(loadFinished(bool)), this, SLOT(renderPreview()));
    connect( ui.klineedit_title, SIGNAL(editingFinished()), this, SLOT(generatePreview()));
    connect( ui.klineedit_author, SIGNAL(editingFinished()), this, SLOT(generatePreview()));
    connect( ui.kListBox_slides, SIGNAL(currentRowChanged(int)), this, SLOT(generatePreview(int)));
    connect( ui.kcombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(generatePreview()));
    connect( ui.kPushButton_selectAll, SIGNAL(clicked()), this, SLOT(checkAllItems()));
    connect( ui.kPushButton_deselectAll, SIGNAL(clicked()), this, SLOT(uncheckAllItems()));
    connect( ui.toolButton_previous, SIGNAL(clicked()), this, SLOT(generatePrevious()));
    connect( ui.toolButton_next, SIGNAL(clicked()), this, SLOT(generateNext()));
    connect( ui.kPushButton_Favorite, SIGNAL(clicked()), this, SLOT(favoriteAction()));
    connect( ui.kcombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFavoriteButton()));

    this->updateFavoriteButton();
    this->frameToRender = 0;
    this->generateSlidesNames(slides);
    this->loadCssList();
    this->generatePreview();
}

QList<KoPAPageBase*> KPrHtmlExportDialog::checkedSlides()
{
    QList<KoPAPageBase*> selectedSlides;
    int countItems = ui.kListBox_slides->count();
    for (int i = 0; i < countItems; i++) {
        if (ui.kListBox_slides->item(i)->checkState() == Qt::Checked) {
            selectedSlides.append(this->m_allSlides.at(i));
        }
    }
    return selectedSlides;
}

void KPrHtmlExportDialog::checkAllItems()
{
    int countItems = ui.kListBox_slides->count();
    for (int i = 0; i < countItems; i++ ){
        ui.kListBox_slides->item(i)->setCheckState(Qt::Checked);
    }
}

void KPrHtmlExportDialog::uncheckAllItems()
{
    int countItems = ui.kListBox_slides->count();
    for (int i = 0; i < countItems; i++) {
        ui.kListBox_slides->item(i)->setCheckState(Qt::Unchecked);
    }
}

QStringList KPrHtmlExportDialog::slidesNames(){
    QStringList names;
    int countItems = ui.kListBox_slides->count();
    for (int i = 0; i < countItems; i++) {
        if (ui.kListBox_slides->item(i)->checkState() == Qt::Checked) {
            names.append(ui.kListBox_slides->item(i)->text());
        }
    }
    return names;
}

KUrl KPrHtmlExportDialog::css()
{
    return KUrl(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
}

void KPrHtmlExportDialog::generateSlidesNames(QList<KoPAPageBase*> slides)
{
    QString slideName;
    for (int i = 0; i < slides.count(); i++) {
        if (slides.at(i)->name().isEmpty()) {
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
    for (QStringList::ConstIterator path=dirs.begin(); path!=dirs.end(); ++path) {
        QDir dir(*path);
        dir.setFilter(QDir::Dirs);
        QStringList entries = dir.entryList();
        for (QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry) {
            if (*entry != "." && *entry != "..") {
                if (dir.exists(*entry + "/style.css")) {
                    ui.kcombobox->addItem(QString(*entry),QVariant(QString(*path + *entry + "/style.css")));
                } 
            }
        }
    }
    ui.kcombobox->insertSeparator(ui.kcombobox->count());
}

//Update the CSS list on add from favorites
void KPrHtmlExportDialog::updateCssListOnAdd(QString basePath)
{
	QString name = ui.kcombobox->itemText(ui.kcombobox->currentIndex());
	QVariant qVariant = basePath;
	ui.kcombobox->removeItem(ui.kcombobox->currentIndex());
        ui.kcombobox->insertItem(0, name, qVariant);
	ui.kcombobox->setCurrentIndex(0);
}

// Update the CSS list on delete from favorites
void KPrHtmlExportDialog::updateCssListOnDelete()
{
    ui.kcombobox->removeItem(ui.kcombobox->currentIndex());
}

void KPrHtmlExportDialog::addFavoriteCSS(){
    QString basePath = KStandardDirs::locateLocal("data","kpresenter/templates/exportHTML");
    QString cssPath(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
	// test if the css already exits
        QFile fileCss(cssPath);
        QStringList list = cssPath.split("/");
        QString shortName = list[list.count()-1];
        shortName.remove(QString(".css"),Qt::CaseInsensitive);
        QDir newdir(basePath);
        basePath.append("/" + shortName);
        basePath.append("/style.css");
        QFile newfile(basePath);
        if(newdir.mkpath(shortName)){
            if(! (QFile::copy(fileCss.fileName(),newfile.fileName()))){
            	// error copy
            }
        }
        updateCssListOnAdd(basePath);
}

void KPrHtmlExportDialog::delFavoriteCSS(){

    if (this->cssIsFavorite()){
        QString cssPath(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
        QStringList list = cssPath.split("/");
        QString shortName = list[list.count()-1];
        cssPath = cssPath.remove(cssPath.size()-shortName.size()-1,shortName.size()+1);
        QDir dir = QDir(cssPath);
		delDirectory(dir);
        updateCssListOnDelete();
    }
}

// delete recursively all files into the selected directory
void KPrHtmlExportDialog::delDirectory(QDir dir){
	dir.setFilter(QDir::AllDirs);
	QStringList dirList = dir.entryList();
    for (QStringList::ConstIterator element = dirList.begin(); element != dirList.end(); ++element) {
		QString test = *element;
		if(*element != "." && *element !=  ".."){
			delDirectory(QDir(dir.filePath(*element)));
		}
	}
	dir.setFilter(QDir::Files);
	dirList = dir.entryList();
    for (QStringList::ConstIterator element = dirList.begin(); element != dirList.end(); ++element) {
		dir.remove(*element);
	}
	QString dirName = dir.dirName();
	dir.cdUp();
	dir.rmdir(dirName);
}

void KPrHtmlExportDialog::browserAction(){
    KFileDialog dialog(KUrl("/"),QString("*.css"),this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name=dialog.selectedFile();
        int index = ui.kcombobox->findData(name);
        if (index==-1) {
            QStringList list = name.split("/");
            QString shortName = list[list.count()-1];
            ui.kcombobox->addItem(shortName,QVariant(name));
            index=ui.kcombobox->count()-1;
        }
            ui.kcombobox->setCurrentIndex(index);
   }
}

QString KPrHtmlExportDialog::title()
{
    return  ui.klineedit_title->text();
}

QString KPrHtmlExportDialog::author()
{
    return ui.klineedit_author->text();
}

void KPrHtmlExportDialog::generatePrevious() {
    generatePreview(--frameToRender);
}

void KPrHtmlExportDialog::generateNext() {
    generatePreview(++frameToRender);
}

void KPrHtmlExportDialog::generatePreview(int item) {
    if(item >= 0) {
        frameToRender = item;
    }

    ui.kListBox_slides->setCurrentRow(frameToRender);
    // refresh button status
    ui.toolButton_previous->setEnabled(frameToRender > 0);
    ui.toolButton_next->setEnabled(frameToRender < (m_allSlides.size() - 1));

    KPrHtmlExport previewGenerator;
    QList<KoPAPageBase*> slides;
    QStringList slidesNames;
    slides.append(this->m_allSlides.at(frameToRender));
    slidesNames.append(ui.kListBox_slides->item(frameToRender)->text());

    KUrl url = previewGenerator.exportPreview(KPrHtmlExport::Parameter(this->css(), (KPrView*)this->parentWidget(), slides, KUrl()
                                                            , this->author()
                                                            , ui.klineedit_title->text()
                                                            , slidesNames));
    preview.mainFrame()->load(url);
}

void KPrHtmlExportDialog::renderPreview()
{
    preview.currentFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    preview.currentFrame()->setScrollBarPolicy(Qt::Vertical,   Qt::ScrollBarAlwaysOff);
    preview.setViewportSize(preview.currentFrame()->contentsSize());
    QImage image(preview.currentFrame()->contentsSize(), QImage::Format_ARGB32);
    QPainter painter(&image);

    preview.mainFrame()->render(&painter);
    painter.end();

    QImage thumbnail = image.scaled(ui.qLabel_preview->size(), Qt::KeepAspectRatio);
    ui.qLabel_preview->setPixmap(QPixmap::fromImage(thumbnail));
}

bool KPrHtmlExportDialog::cssIsFavorite() {
    QString cssPath(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
    return cssPath.contains(KStandardDirs::locateLocal("data","kpresenter/templates/exportHTML"));
}

bool KPrHtmlExportDialog::cssIsSystemFavorite() {
    QString cssPath(ui.kcombobox->itemData(ui.kcombobox->currentIndex()).toString());
    QString dir;

    QStringList dirs(KStandardDirs().findDirs("data", "kpresenter/templates/exportHTML"));
    for (QStringList::ConstIterator path=dirs.begin(); path!=dirs.end(); ++path) {
        if (!path->contains(KStandardDirs::locateLocal("data","kpresenter/templates/exportHTML"))){
            dir = *path;
        }
    }
    return (!dir.isNull()) && cssPath.contains(dir);
}

void KPrHtmlExportDialog::updateFavoriteButton(){
    if (this->cssIsFavorite() || this->cssIsSystemFavorite()){
        ui.kPushButton_Favorite->setText(i18n("Delete from Favorite"));
        if (this->cssIsSystemFavorite()){
            ui.kPushButton_Favorite->setEnabled(false);
            ui.kPushButton_Favorite->setToolTip(i18n("You may not remove the templates provided with the application"));
        }
        else {
            ui.kPushButton_Favorite->setEnabled(true);
            ui.kPushButton_Favorite->setToolTip("");
        }
    }
    else {
        ui.kPushButton_Favorite->setText(i18n("Add as Favorite"));
        ui.kPushButton_Favorite->setEnabled(true);
        ui.kPushButton_Favorite->setToolTip("");
    }
}

void KPrHtmlExportDialog::favoriteAction(){
   if (this->cssIsFavorite()){
        this->delFavoriteCSS();
    }
    else {
        this->addFavoriteCSS();
    }
}
