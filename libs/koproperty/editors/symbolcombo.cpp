/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004  Alexander Dymo <cloudtemple@mskat.net>

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

#include <QLineEdit>
#include <QPushButton>
#include <QLayout>
#include <QPainter>
#include <QVariant>
#include <QHBoxLayout>

#include <kcharselect.h>
#include <klocale.h>
#include <kdialog.h>

#include "symbolcombo.h"

using namespace KoProperty;

SymbolCombo::SymbolCombo(Property *property, QWidget *parent)
        : Widget(property, parent)
{
    setHasBorders(false);
    QHBoxLayout *l = new QHBoxLayout(this);

    m_edit = new QLineEdit(this);
// m_edit->setLineWidth(0);
    m_edit->setReadOnly(true);
    m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_edit->setMinimumHeight(5);
    m_edit->setMaxLength(1);
    l->addWidget(m_edit);
    m_select = new QPushButton("...", this);
    m_select->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    m_select->setMinimumHeight(5);
    l->addWidget(m_select);

    connect(m_select, SIGNAL(clicked()), this, SLOT(selectChar()));
    connect(m_edit, SIGNAL(textChanged(const QString&)), this, SLOT(slotValueChanged(const QString&)));
}

SymbolCombo::~SymbolCombo()
{}

QVariant
SymbolCombo::value() const
{
    if (!(m_edit->text().isNull()))
        return m_edit->text().at(0).unicode();
    else
        return 0;
}

void
SymbolCombo::setValue(const QVariant &value, bool emitChange)
{
    if (!(value.isNull()))
    {
        m_edit->blockSignals(true);
        m_edit->setText(QChar(value.toInt()));
        m_edit->blockSignals(false);
        if (emitChange)
            emit valueChanged(this);
    }
}

void
SymbolCombo::drawViewer(QPainter *p, const QColorGroup &cg, const QRect &r, const QVariant &value)
{
// p->eraseRect(r);
// p->drawText(r, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, QChar(value.toInt()));
    Widget::drawViewer(p, cg, r, QString(QChar(value.toInt())));
}

void
SymbolCombo::selectChar()
{
    KDialog dialog(this->topLevelWidget());
    dialog.setWindowTitle(i18n("Select Char"));
    dialog.setObjectName("charselect_dialog");
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);
    dialog.setModal(false);
    dialog.showButtonSeparator(true);

    KCharSelect *select = new KCharSelect(&dialog);
    dialog.setObjectName("select_char");
    dialog.setMainWidget(select);

    if (!(m_edit->text().isNull()))
        select->setCurrentChar(m_edit->text().at(0));

    if (dialog.exec() == QDialog::Accepted)
        m_edit->setText(select->currentChar());
}

void
SymbolCombo::slotValueChanged(const QString&)
{
    emit valueChanged(this);
}

void
SymbolCombo::setReadOnlyInternal(bool readOnly)
{
    m_select->setEnabled(!readOnly);
}

#include "symbolcombo.moc"
