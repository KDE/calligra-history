/*
 *  Copyright (c) 2007,2010 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_entry_editor.h"
#include <QString>
#include <QVariant>

#include <kis_debug.h>

#include "kis_meta_data_value.h"
#include <kis_meta_data_store.h>
#include <kis_meta_data_entry.h>

struct KisEntryEditor::Private {
    QObject* object;
    QString propertyName;
    KisMetaData::Store* store;
    QString key;
    QString structField;
    int arrayIndex;
    KisMetaData::Value value() {
        KisMetaData::Value value = store->getEntry(key).value();
        if (value.type() == KisMetaData::Value::Structure && !structField.isEmpty()) {
            QMap<QString, KisMetaData::Value> structure = value.asStructure();
            return structure[ structField ];
        } else if (value.isArray() && arrayIndex > -1) {
            QList<KisMetaData::Value> array = value.asArray();
            if (arrayIndex < array.size()) {
                return array[arrayIndex];
            } else {
                return KisMetaData::Value();
            }
        }
        return value;
    }
    void setValue(const QVariant& variant) {
        KisMetaData::Value& value = store->getEntry(key).value();
        if (value.type() == KisMetaData::Value::Structure && !structField.isEmpty()) {
            QMap<QString, KisMetaData::Value> structure = value.asStructure();
            value = structure[ structField ];
            value.setVariant(variant);
            value.setStructureVariant(structField, variant);
        } else if (value.isArray() && arrayIndex > -1) {
            value.setArrayVariant(arrayIndex, variant);
        } else {
            value.setVariant(variant);
        }
    }
};

KisEntryEditor::KisEntryEditor(QObject* obj, KisMetaData::Store* store, QString key, QString propertyName, QString structField, int arrayIndex) : d(new Private)
{
    Q_ASSERT(obj);
    Q_ASSERT(store);
    d->object = obj;
    d->propertyName = propertyName;
    d->store = store;
    d->key = key;
    d->structField = structField;
    d->arrayIndex = arrayIndex;
    valueChanged();
}

KisEntryEditor::~KisEntryEditor()
{
    delete d;
}

void KisEntryEditor::valueChanged()
{
    if (d->store->containsEntry(d->key)) {
        bool blocked = d->object->blockSignals(true);
        d->object->setProperty(d->propertyName.toAscii(), d->value().asVariant());
        d->object->blockSignals(blocked);
    }
}

void KisEntryEditor::valueEdited()
{
    QVariant val = d->object->property(d->propertyName.toAscii());
    dbgPlugins << "Value edited: " << d->propertyName << val;
    d->setValue(val);
    emit valueHasBeenEdited();
}

#include "kis_entry_editor.moc"
