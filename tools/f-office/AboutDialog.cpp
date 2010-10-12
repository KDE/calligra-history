/*
 * This file is part of Maemo 5 Office UI for KOffice
 *
 * Copyright (C) 2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Manikandaprasad N C <manikandaprasad.chandrasekar@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "Common.h"

#include <klocalizedstring.h>

AboutDialog::AboutDialog(QWidget *parent) :
        QDialog(parent),
        m_ui(new Ui::aboutDialog)
{
    QString message = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \
    \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" \
    content=\"1\" /><style type=\"text/css\">\n p, li { white-space: pre-wrap; }\n \
    </style></head><body style=\" font-family:'Sans Serif'; font-size:12pt; font-weight:400; \
    font-style:normal;\">\n <p>" +

    i18n("Freoffice is a mobile edition of KOffice Open source project. <br/>"
     "Freoffice code can be found in the KOffice Open Source project. <br/>"
     "Freoffice is in beta edition and it supports viewing of spreadsheet, presentation and document in both<br/>"
     "Open document format (ODF) and Microsoft format. <br/>"
     "The editing functionality is limited to ODF documents only.")+
    "</p></body></html>";

    m_ui->setupUi(this);
    m_ui->label->setText(message);
    m_ui->label_koffice->setPixmap(ABOUT_DIALOG_KOFFICE_PIXMAP);
    m_ui->label_nokia->setPixmap(ABOUT_DIALOG_NOKIA_PIXMAP);
    m_ui->label_koffice->adjustSize();
    m_ui->label_nokia->adjustSize();
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}
