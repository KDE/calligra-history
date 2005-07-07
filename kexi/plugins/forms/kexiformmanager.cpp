/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexiformmanager.h"
#include "kexidbform.h"
#include "kexiformscrollview.h"
#include "kexiformview.h"
#include "kexidatasourcepage.h"

#include <formeditor/formmanager.h>
#include <formeditor/widgetpropertyset.h>
#include <formeditor/form.h>
#include <formeditor/widgetlibrary.h>
#include <formeditor/commands.h>
#include <formeditor/objecttree.h>

#include <koproperty/set.h>
#include <koproperty/property.h>

KexiFormManager::KexiFormManager(KexiPart::Part *parent, const QStringList& supportedFactoryGroups,
	const char* name)
 : KFormDesigner::FormManager(parent, supportedFactoryGroups, 
		KFormDesigner::FormManager::HideEventsInPopupMenu |
		KFormDesigner::FormManager::SkipFileActions |
		KFormDesigner::FormManager::HideSignalSlotConnections
	, name)
 , m_part(parent)
{
	lib()->setAdvancedPropertiesVisible(false);
}

KexiFormManager::~KexiFormManager()
{
}

KAction* KexiFormManager::action( const char* name )
{
	KActionCollection *col = m_part->actionCollectionForMode(Kexi::DesignViewMode);
	if (!col)
		return 0;
	QCString n( translateName( name ).latin1() );
	KAction *a = col->action(n);
	if (a)
		return a;
	KexiDBForm *dbform;
	if (!activeForm() || !activeForm()->designMode()
		|| !(dbform = dynamic_cast<KexiDBForm*>(activeForm()->formWidget())))
		return 0;
	KexiFormScrollView *scrollViewWidget = dynamic_cast<KexiFormScrollView*>(dbform->dataAwareObject());
	if (!scrollViewWidget)
		return 0;
	KexiFormView* formViewWidget = dynamic_cast<KexiFormView*>(scrollViewWidget->parent());
	if (!formViewWidget)
		return 0;
	return formViewWidget->parentDialog()->mainWin()->actionCollection()->action(n);
}

void KexiFormManager::enableAction( const char* name, bool enable )
{
	KexiDBForm *dbform;
	if (!activeForm() || !activeForm()->designMode()
		|| !(dbform = dynamic_cast<KexiDBForm*>(activeForm()->formWidget())))
		return;
	KexiFormScrollView *scrollViewWidget = dynamic_cast<KexiFormScrollView*>(dbform->dataAwareObject());
	if (!scrollViewWidget)
		return;
	KexiFormView* formViewWidget = dynamic_cast<KexiFormView*>(scrollViewWidget->parent());
	if (!formViewWidget)
		return;
	if (QString(name)=="layout_menu")
		kdDebug() << "!!!!!!!!!!! " << enable << endl;
	formViewWidget->setAvailable(translateName( name ).latin1(), enable);
}

void KexiFormManager::setFormDataSource(const QCString& mime, const QCString& name)
{
	if (!activeForm())
		return;
	KexiDBForm* formWidget = dynamic_cast<KexiDBForm*>(activeForm()->widget());
	if (!formWidget)
		return;

//	setPropertyValueInDesignMode(formWidget, "dataSource", name);

	QCString oldDataSourceMimeType( formWidget->dataSourceMimeType() );
	QCString oldDataSource( formWidget->dataSource().latin1() );
	if (mime!=oldDataSourceMimeType || name!=oldDataSource) {
		QMap<QCString, QVariant> propValues;
		propValues.insert("dataSource", name);
		propValues.insert("dataSourceMimeType", mime);
		propertySet()->setPropertyValueInDesignMode(formWidget, propValues, 
			i18n("Set form's data source to \"%1\"").arg(name));
	}

/*
	if (activeForm()->selectedWidget() == formWidget) {
		//active form is selected: just use properties system
		KFormDesigner::WidgetPropertySet *set = propertySet();
		if (!set || !set->contains("dataSource"))
			return;
		(*set)["dataSource"].setValue(name);
		if (set->contains("dataSourceMimeType"))
			(*set)["dataSourceMimeType"].setValue(mime);
		return;
	}

	//active form isn't selected: change it's data source and mime type by hand
	QCString oldDataSourceMimeType( formWidget->dataSourceMimeType() );
	QCString oldDataSource( formWidget->dataSource().latin1() );

	if (mime!=oldDataSourceMimeType || name!=oldDataSource) {
		formWidget->setDataSourceMimeType(mime);
		formWidget->setDataSource(name);
		emit dirty(activeForm(), true);

		activeForm()->addCommand( 
			new KFormDesigner::PropertyCommand(propertySet(), QString(formWidget->name()),
				oldDataSource, name, "dataSource"), 
			false );

		// If the property is changed, we add it in ObjectTreeItem modifProp
		KFormDesigner::ObjectTreeItem *fromTreeItem = activeForm()->objectTree()->lookup(formWidget->name());
		fromTreeItem->addModifiedProperty("dataSourceMimeType", mime);
		fromTreeItem->addModifiedProperty("dataSource", name);
 	}*/
}

void KexiFormManager::setDataSourceFieldOrExpression(const QString& string)
{
	if (!activeForm())
		return;
//	KexiFormDataItemInterface* dataWidget = dynamic_cast<KexiFormDataItemInterface*>(activeForm()->selectedWidget());
//	if (!dataWidget)
//		return;
	
	KFormDesigner::WidgetPropertySet *set = propertySet();
	if (!set || !set->contains("dataSource"))
		return;

	(*set)["dataSource"].setValue(string);

/*	QString oldDataSource( dataWidget->dataSource() );
	if (string!=oldDataSource) {
		dataWidget->setDataSource(string);
		emit dirty(activeForm(), true);

		buffer
	}*/
}

void KexiFormManager::slotHistoryCommandExecuted(KCommand* command)
{
	KFormDesigner::CommandGroup *group = dynamic_cast<KFormDesigner::CommandGroup*>(command);
	if (group) {
		if (group->commands().count()==2) {
			KexiDBForm* formWidget = dynamic_cast<KexiDBForm*>(activeForm()->widget());
			if (!formWidget)
				return;
			const KFormDesigner::PropertyCommand* pc1 = dynamic_cast<const KFormDesigner::PropertyCommand*>(group->commands().at(0));
			const KFormDesigner::PropertyCommand* pc2 = dynamic_cast<const KFormDesigner::PropertyCommand*>(group->commands().at(1));
			if (pc1 && pc2 && pc1->property()=="dataSource" && pc2->property()=="dataSourceMimeType") {
				const QMap<QCString, QVariant>::const_iterator it1( pc1->oldValues().constBegin() );
				const QMap<QCString, QVariant>::const_iterator it2( pc2->oldValues().constBegin() );
				if (it1.key()==formWidget->name() && it2.key()==formWidget->name())
					static_cast<KexiFormPart*>(m_part)->dataSourcePage()->setDataSource(
						formWidget->dataSourceMimeType(), formWidget->dataSource().latin1());
			}
		}
	}
}

/*
bool KexiFormManager::loadFormFromDomInternal(Form *form, QWidget *container, QDomDocument &inBuf)
{
	QMap<QCString,QString> customProperties;
	FormIO::loadFormFromDom(myform, container, domDoc, &customProperties);
}

bool KexiFormManager::saveFormToStringInternal(Form *form, QString &dest, int indent)
{
	QMap<QCString,QString> customProperties;
	return KFormDesigner::FormIO::saveFormToString(form, dest, indent, &customProperties);
}

*/

#include "kexiformmanager.moc"
