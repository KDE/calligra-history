/*
 * Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
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

#include "QVariantValue.h"

#include <QColor>
#include <GTLCore/Type.h>
#include <GTLCore/TypesManager.h>
#include "Version.h"

#include "kis_debug.h"

#ifdef OPENSHIVA_13_OR_MORE
#include <GTLCore/Color.h>
#endif

QVariant valueToQVariant(const GTLCore::Value& value)
{
    switch (value.type()->dataType()) {
    default:
    case GTLCore::Type::UNDEFINED:
        return QVariant();
    case GTLCore::Type::BOOLEAN:
        return QVariant(value.asBoolean());
    case GTLCore::Type::INTEGER8:
    case GTLCore::Type::INTEGER16:
    case GTLCore::Type::INTEGER32:
        return QVariant(value.asInt32());
    case GTLCore::Type::UNSIGNED_INTEGER8:
    case GTLCore::Type::UNSIGNED_INTEGER16:
    case GTLCore::Type::UNSIGNED_INTEGER32:
        return QVariant(value.asUnsignedInt32());
    case GTLCore::Type::FLOAT16:
    case GTLCore::Type::FLOAT32:
    case GTLCore::Type::FLOAT64:
#if OPENSHIVA_12
        return QVariant(value.asFloat());
#else
        return QVariant(value.asFloat32());
#endif
    case GTLCore::Type::ARRAY:
    case GTLCore::Type::VECTOR: {
        QList<QVariant> variant;
        foreach(const GTLCore::Value& val, *value.asArray()) {
            variant.push_back(valueToQVariant(val));
        }
        return QVariant(variant);
    }
#if OPENSHIVA_13_OR_MORE
    case GTLCore::Type::STRUCTURE:
        if (value.type() == GTLCore::Type::Color )
        {
            GTLCore::Color c = value.asColor();
            return QVariant(QColor(c.red() * 255, c.green() * 255, c.blue() * 255, c.alpha() * 255) );
        }
    }
#endif
    qFatal("Unsupported type: %i", value.type());
}

GTLCore::Value qvariantToValue(const QVariant& variant, const GTLCore::Type* _type)
{
    switch (_type->dataType()) {
    case GTLCore::Type::BOOLEAN:
        return GTLCore::Value(variant.toBool());
    case GTLCore::Type::FLOAT16:
    case GTLCore::Type::FLOAT32:
    case GTLCore::Type::FLOAT64:
        return GTLCore::Value((float)variant.toDouble());
    case GTLCore::Type::INTEGER8:
    case GTLCore::Type::INTEGER16:
    case GTLCore::Type::INTEGER32:
        return GTLCore::Value(variant.toInt());
    case GTLCore::Type::UNSIGNED_INTEGER8:
    case GTLCore::Type::UNSIGNED_INTEGER16:
    case GTLCore::Type::UNSIGNED_INTEGER32:
        return GTLCore::Value(variant.toUInt());
    case GTLCore::Type::ARRAY:
    case GTLCore::Type::VECTOR: {
        std::vector<GTLCore::Value> values;
        foreach(const QVariant& var, variant.toList()) {
            values.push_back(qvariantToValue(var, _type->embeddedType()));
        }
        if( _type->dataType() == GTLCore::Type::VECTOR && values.size() != _type->vectorSize() )
        {
          dbgPlugins << "Invalid numbers of element for a vector, got: " << values.size() << " but expected: " << _type->vectorSize();
          return GTLCore::Value();
        }
        return GTLCore::Value(values, _type);
    }
#if OPENSHIVA_13_OR_MORE
    case GTLCore::Type::STRUCTURE: {
        if (_type == GTLCore::Type::Color) {
            QColor c = variant.value<QColor>();
            dbgPlugins << c << variant;
            return GTLCore::Value(GTLCore::Color(c.red() / 255.0, c.green() / 255.0, c.blue() / 255.0, c.alpha() / 255.0));
        }
    }
#endif
    default:
    case GTLCore::Type::UNDEFINED: {
        qFatal("Unsupported type: %i", variant.type());
        return GTLCore::Value();
    }
    }
}
