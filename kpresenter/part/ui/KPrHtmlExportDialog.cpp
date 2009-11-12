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

KPrHtmlExportDialog::KPrHtmlExportDialog(QList<KoPAPageBase*> slides, QWidget *parent) : KDialog(parent)
{
    allSlides = slides;
    QWidget *widget = new QWidget( this );
    ui.setupUi( widget );
    setMainWidget( widget );
    setCaption( i18n( "Html Export"));
    setButtonText(Ok, i18n("Export"));

    QList<KoPAPageBase*>::iterator it;
    QString slideName = "";
    int i = 1;
    for(it = slides.begin(); it != slides.end(); ++it){
        if ((*it)->name() == NULL)
            slideName = i18n("Slide") + " " + QString::number(i);
        else
            slideName = (*it)->name();
        QListWidgetItem *listItem = new  QListWidgetItem(slideName);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        listItem->setCheckState(Qt::Checked);
        ui.kListBox_slides->addItem(listItem);
        i++;
    }
    connect( ui.kPushButton_selectAll  , SIGNAL( clicked() ), this, SLOT( checkAllItems()  ) );
    connect( ui.kPushButton_deselectAll, SIGNAL( clicked() ), this, SLOT( uncheckAllItems()) );
}

QList<KoPAPageBase*> KPrHtmlExportDialog::chekedSlides()
{
    QList<KoPAPageBase*> qlchekedSlides;
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        if (ui.kListBox_slides->item(i)->checkState() == Qt::Checked)
            qlchekedSlides.append(this->allSlides.at(i));
    }
    return qlchekedSlides;
}

void KPrHtmlExportDialog::renameSlides()
{
    int countItems = ui.kListBox_slides->count();
    for(int i = 0; i < countItems; i++){
        allSlides.at(i)->setName(ui.kListBox_slides->item(i)->text());
    }
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