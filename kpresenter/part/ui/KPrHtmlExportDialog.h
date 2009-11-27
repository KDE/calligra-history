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

#ifndef KPRHTMLEXPORTDIALOG_H
#define KPRHTMLEXPORTDIALOG_H

#include <KDialog>
#include <KoPAPageBase.h>
#include "ui_KPrHtmlExport.h"

class KPrHtmlExportDialog  : public KDialog
{
    Q_OBJECT
public:
    explicit KPrHtmlExportDialog(QList<KoPAPageBase*> slides, QWidget *parent=0);
    QList<KoPAPageBase*> chekedSlides();
    QStringList slidesNames();
    KUrl css();
    
private slots:
    void checkAllItems();
    void uncheckAllItems();

private:
    Ui::KPrHtmlExport ui;
    QList<KoPAPageBase*> m_allSlides;
    void generateSlidesNames(QList<KoPAPageBase*> slides);
    void loadCssList();
};



#endif // KPRHTMLEXPORTDIALOG_H
