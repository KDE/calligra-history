/* This file is part of the KDE project
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kexinamedialog.h"

KexiNameDialog::KexiNameDialog(const QString& message, 
	QWidget * parent, const char * name)
 : KDialogBase(KDialogBase::Plain, QString::null,
	KDialogBase::Ok|KDialogBase::Cancel|KDialogBase::Help,
	KDialogBase::Ok,
	parent, name)
{
//	QHBox ( QWidget * parent = 0, const char * name = 0, WFlags f = 0 )
	m_widget= new KexiNameWidget(message, plainPage(), "KexiNameWidget");
	init();
}

KexiNameDialog::KexiNameDialog(const QString& message, 
	const QString& nameLabel, const QString& nameText, 
	const QString& captionLabel, const QString& captionText, 
	QWidget * parent, const char * name)
 : KDialogBase(KDialogBase::Plain, QString::null,
	KDialogBase::Ok|KDialogBase::Cancel,
	KDialogBase::Ok,
	parent, name)
{
	m_widget= new KexiNameWidget(message, nameLabel, nameText,
		captionLabel, captionText, plainPage(), "KexiNameWidget");
	init();
}

KexiNameDialog::~KexiNameDialog()
{
}

void KexiNameDialog::init()
{
	QGridLayout *lyr = new QGridLayout(plainPage(), 2, 3);
	m_icon = new QLabel( plainPage(), "icon" );
	m_icon->setAlignment( int( AlignTop | AlignLeft ) );
	m_icon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred,1,0));
	m_icon->setFixedWidth(50);
	lyr->addWidget(m_icon,0,0);

	m_widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred,1,0));
	lyr->addWidget(m_widget,0,1);
	lyr->addItem(new QSpacerItem( 25, 10, QSizePolicy::Expanding, QSizePolicy::Minimum ), 0, 2);
	lyr->addItem(new QSpacerItem( 5, 10, QSizePolicy::Minimum, QSizePolicy::Expanding ), 1, 1);
	m_widget->captionLineEdit()->selectAll();
	m_widget->captionLineEdit()->setFocus();
	resize( QSize(400, 140 + (m_widget->lbl_message->isVisible()?m_widget->lbl_message->height():0) )
		.expandedTo(minimumSizeHint()) );
	enableButtonOK( true );
	slotTextChanged();
	connect(m_widget, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void KexiNameDialog::slotTextChanged()
{
	if (!m_widget->acceptsEmptyValue()) {
		enableButtonOK( !m_widget->captionLineEdit()->text().isEmpty() 
			&& !m_widget->nameLineEdit()->text().isEmpty() );
	}
}

void KexiNameDialog::accept()
{
	if (!m_widget->checkValidity())
		return;
	KDialogBase::accept();
}

void KexiNameDialog::setDialogIcon(const QPixmap& icon)
{
	m_icon->setPixmap(icon);
}

#include "kexinamedialog.moc"

