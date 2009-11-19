/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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

#include "kis_exif_io.h"

#include <exiv2/exif.hpp>

#include <qendian.h>
#include <QIODevice>
#include <QByteArray>
#include <QVariant>
#include <QDateTime>
#include <QDate>
#include <QTime>

#include <kis_debug.h>

#include "kis_exiv2.h"

#include <kis_meta_data_store.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_value.h>
#include <kis_meta_data_schema.h>
#include <kis_meta_data_schema_registry.h>

struct KisExifIO::Private {
};

// ---- Exception convertion functions ---- //

// convert ExifVersion and FlashpixVersion to a KisMetaData value
KisMetaData::Value exifVersionToKMDValue(const Exiv2::Value::AutoPtr value)
{
    const Exiv2::DataValue* dvalue = dynamic_cast<const Exiv2::DataValue*>(&*value);
    Q_ASSERT(dvalue);
    QByteArray array(dvalue->count(), 0);
    dvalue->copy((Exiv2::byte*)array.data());
    return KisMetaData::Value(QString(array));
}

// convert from KisMetaData value to ExifVersion and FlashpixVersion
Exiv2::Value* kmdValueToExifVersion(const KisMetaData::Value& value)
{
    Exiv2::DataValue* dvalue = new Exiv2::DataValue;
    QString ver = value.asVariant().toString();
    dvalue->read((const Exiv2::byte*)ver.toAscii().data(), ver.size());
    return dvalue;
}

// Convert an exif array of integer string to a KisMetaData array of integer
KisMetaData::Value exifArrayToKMDIntOrderedArray(const Exiv2::Value::AutoPtr value)
{
    QList<KisMetaData::Value> v;
    const Exiv2::DataValue* dvalue = dynamic_cast<const Exiv2::DataValue*>(&*value);
    Q_ASSERT(dvalue);
    QByteArray array(dvalue->count(), 0);
    dvalue->copy((Exiv2::byte*)array.data());
    for (int i = 0; i < array.size(); i++) {
        QChar c((char)array[i]);
        v.push_back(KisMetaData::Value(QString(c).toInt(0)));
    }
    return KisMetaData::Value(v, KisMetaData::Value::OrderedArray);
}

// Convert a KisMetaData array of integer to an exif array of integer string
Exiv2::Value* kmdIntOrderedArrayToExifArray(const KisMetaData::Value& value)
{
    QList<KisMetaData::Value> v = value.asArray();
    QString s;
    for (QList<KisMetaData::Value>::iterator it = v.begin();
            it != v.end(); ++it) {
        int val = it->asVariant().toInt(0);
        s += QString::number(val);
    }
    return new Exiv2::DataValue((const Exiv2::byte*)s.toAscii().data(), s.toAscii().size());
}

QDateTime exivValueToDateTime(const Exiv2::Value::AutoPtr value)
{
    return QDateTime::fromString(value->toString().c_str(), Qt::ISODate);
}

template<typename T>
inline T fixEndianess(T v, Exiv2::ByteOrder order)
{
    switch (order) {
    case Exiv2::invalidByteOrder:
        return v;
    case Exiv2::littleEndian:
        return qFromLittleEndian<T>(v);
    case Exiv2::bigEndian:
        return qFromBigEndian<T>(v);
    }
    qFatal("Unknown byte order");
    return v;
}

Exiv2::ByteOrder invertByteOrder(Exiv2::ByteOrder order)
{
    switch (order) {
    case Exiv2::invalidByteOrder:
        qFatal("Can't invert Exiv2::invalidByteOrder");
    case Exiv2::littleEndian:
        return Exiv2::bigEndian;
    case Exiv2::bigEndian:
        return Exiv2::littleEndian;
    }
    return Exiv2::invalidByteOrder;
}


KisMetaData::Value exifOECFToKMDOECFStructure(const Exiv2::Value::AutoPtr value, Exiv2::ByteOrder order)
{
    QMap<QString, KisMetaData::Value> oecfStructure;
    const Exiv2::DataValue* dvalue = dynamic_cast<const Exiv2::DataValue*>(&*value);
    Q_ASSERT(dvalue);
    QByteArray array(dvalue->count(), 0);
    dvalue->copy((Exiv2::byte*)array.data());
    int columns = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[0], order);
    int rows = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[1], order);

    if ((columns * rows + 4) > dvalue->count()) { // Sometime byteOrder get messed up (especially if metadata got saved with kexiv2 library, or any library that doesn't save back with the same byte order as the camera)
        order = invertByteOrder(order);
        columns = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[0], order);
        rows = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[1], order);
        Q_ASSERT((columns * rows + 4) > dvalue->count());
    }
    oecfStructure["Columns"] = KisMetaData::Value(columns);
    oecfStructure["Rows"] = KisMetaData::Value(rows);
    int index = 4;
    QList<KisMetaData::Value> names;
    for (int i = 0; i < columns; i++) {
        int lastIndex = array.indexOf((char)0, index);
        QString name = array.mid(index, lastIndex - index);
        if (index != lastIndex) {
            index = lastIndex + 1;
            dbgFile << "Name [" << i << "] =" << name;
            names.append(KisMetaData::Value(name));
        } else {
            names.append(KisMetaData::Value(""));
        }
    }
    oecfStructure["Names"] = KisMetaData::Value(names, KisMetaData::Value::OrderedArray);
    QList<KisMetaData::Value> values;
    qint16* dataIt = reinterpret_cast<qint16*>(array.data() + index);
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            values.append(KisMetaData::Value(KisMetaData::Rational(fixEndianess<qint32>(dataIt[0], order), fixEndianess<qint32>(dataIt[1], order))));
            dataIt += 8;
        }
    }
    oecfStructure["Values"] = KisMetaData::Value(values, KisMetaData::Value::OrderedArray);
    dbgFile << "OECF: " << ppVar(columns) << ppVar(rows) << ppVar(dvalue->count());
    return KisMetaData::Value(oecfStructure);
}

Exiv2::Value* kmdOECFStructureToExifOECF(const KisMetaData::Value& value)
{
    QMap<QString, KisMetaData::Value> oecfStructure = value.asStructure();
    quint16 columns = oecfStructure["Columns"].asVariant().toInt(0);
    quint16 rows = oecfStructure["Rows"].asVariant().toInt(0);

    QList<KisMetaData::Value> names = oecfStructure["Names"].asArray();
    QList<KisMetaData::Value> values = oecfStructure["Values"].asArray();
    Q_ASSERT(columns*rows == values.size());
    int length = 4 + rows * columns * 8; // The 4 byte for storing rows/columns and the rows*columns*sizeof(rational)
    bool saveNames = (!names.empty() && names[0].asVariant().toString().size() > 0);
    if (saveNames) {
        for (int i = 0; i < columns; i++) {
            length += names[i].asVariant().toString().size() + 1;
        }
    }
    QByteArray array(length, 0);
    (reinterpret_cast<quint16*>(array.data()))[0] = columns;
    (reinterpret_cast<quint16*>(array.data()))[1] = rows;
    int index = 4;
    if (saveNames) {
        for (int i = 0; i < columns; i++) {
            QByteArray name = names[i].asVariant().toString().toAscii();
            name.append((char)0);
            memcpy(array.data() + index, name.data(), name.size());
            index += name.size();
        }
    }
    qint16* dataIt = reinterpret_cast<qint16*>(array.data() + index);
    for (QList<KisMetaData::Value>::iterator it = values.begin();
            it != values.end(); it++) {
        dataIt[0] = it->asRational().numerator;
        dataIt[1] = it->asRational().denominator;
        dataIt += 2;
    }
    return new Exiv2::DataValue((const Exiv2::byte*)array.data(), array.size());
}

KisMetaData::Value deviceSettingDescriptionExifToKMD(const Exiv2::Value::AutoPtr value)
{
    QMap<QString, KisMetaData::Value> deviceSettingStructure;
    const Exiv2::DataValue* dvalue = dynamic_cast<const Exiv2::DataValue*>(&*value);
    Q_ASSERT(dvalue);
    QByteArray array(dvalue->count(), 0);
    dvalue->copy((Exiv2::byte*)array.data());
    int columns = (reinterpret_cast<quint16*>(array.data()))[0];
    int rows = (reinterpret_cast<quint16*>(array.data()))[1];
    deviceSettingStructure["Columns"] = KisMetaData::Value(columns);
    deviceSettingStructure["Rows"] = KisMetaData::Value(rows);
    QList<KisMetaData::Value> settings;
    int index = 4;
    for (int i = 0; i < columns * rows; i++) {
        int lastIndex = array.indexOf((char)0, index);
        QString setting = array.mid(index, lastIndex - index);
        index = lastIndex + 2;
        dbgFile << "Setting [" << i << "] =" << setting;
        settings.append(KisMetaData::Value(setting));
    }
    deviceSettingStructure["Settings"] = KisMetaData::Value(settings, KisMetaData::Value::OrderedArray);
    return KisMetaData::Value(deviceSettingStructure);
}

Exiv2::Value* deviceSettingDescriptionKMDToExif(const KisMetaData::Value& value)
{
    QMap<QString, KisMetaData::Value> deviceSettingStructure = value.asStructure();
    quint16 columns = deviceSettingStructure["Columns"].asVariant().toInt(0);
    quint16 rows = deviceSettingStructure["Rows"].asVariant().toInt(0);

    QList<KisMetaData::Value> settings = deviceSettingStructure["Settings"].asArray();
    Q_ASSERT(columns*rows == settings.size());
    QByteArray array(4, 0);
    (reinterpret_cast<quint16*>(array.data()))[0] = columns;
    (reinterpret_cast<quint16*>(array.data()))[1] = rows;
    for (int i = 0; i < columns * rows; i++) {
        QByteArray setting = settings[i].asVariant().toString().toAscii();
        setting.append((char)0);
        setting.append((char)0);
        array.append(setting);
    }
    return new Exiv2::DataValue((const Exiv2::byte*)array.data(), array.size());
}

KisMetaData::Value cfaPatternExifToKMD(const Exiv2::Value::AutoPtr value, Exiv2::ByteOrder order)
{
    QMap<QString, KisMetaData::Value> cfaPatternStructure;
    const Exiv2::DataValue* dvalue = dynamic_cast<const Exiv2::DataValue*>(&*value);
    Q_ASSERT(dvalue);
    QByteArray array(dvalue->count(), 0);
    dvalue->copy((Exiv2::byte*)array.data());
    int columns = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[0], order);
    int rows = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[1], order);
    if ((columns * rows + 4) != dvalue->count()) { // Sometime byteOrder get messed up (especially if metadata got saved with kexiv2 library, or any library that doesn't save back with the same byte order as the camera)
        order = invertByteOrder(order);
        columns = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[0], order);
        rows = fixEndianess<quint16>((reinterpret_cast<quint16*>(array.data()))[1], order);
        Q_ASSERT((columns * rows + 4) == dvalue->count());
    }
    cfaPatternStructure["Columns"] = KisMetaData::Value(columns);
    cfaPatternStructure["Rows"] = KisMetaData::Value(rows);
    QList<KisMetaData::Value> values;
    int index = 4;
    for (int i = 0; i < columns * rows; i++) {
        values.append(KisMetaData::Value(*(array.data() + index)));
        index++;
    }
    cfaPatternStructure["Values"] = KisMetaData::Value(values, KisMetaData::Value::OrderedArray);
    dbgFile << "CFAPattern " << ppVar(columns) << " " << ppVar(rows) << ppVar(values.size() << ppVar(dvalue->count()));
    return KisMetaData::Value(cfaPatternStructure);
}

Exiv2::Value* cfaPatternKMDToExif(const KisMetaData::Value& value)
{
    QMap<QString, KisMetaData::Value> cfaStructure = value.asStructure();
    quint16 columns = cfaStructure["Columns"].asVariant().toInt(0);
    quint16 rows = cfaStructure["Rows"].asVariant().toInt(0);

    QList<KisMetaData::Value> values = cfaStructure["Values"].asArray();
    Q_ASSERT(columns*rows == values.size());
    QByteArray array(4 + columns*rows, 0);
    (reinterpret_cast<quint16*>(array.data()))[0] = columns;
    (reinterpret_cast<quint16*>(array.data()))[1] = rows;
    for (int i = 0; i < columns * rows; i++) {
        int val = values[i].asVariant().toInt();
        *(array.data() + 4 + i) = val;
    }
    dbgFile << "Cfa Array " << ppVar(columns) << ppVar(rows) << ppVar(array.size());
    return new Exiv2::DataValue((const Exiv2::byte*)array.data(), array.size());
}

// Read and write Flash //

KisMetaData::Value flashExifToKMD(const Exiv2::Value::AutoPtr value)
{
    uint16_t v = value->toLong();
    QMap<QString, KisMetaData::Value> flashStructure;
    bool fired = (v & 0x01);  // bit 1 is whether flash was fired or not
    flashStructure["Fired"] = QVariant(fired);
    int ret = ((v >> 1) & 0x03);  // bit 2 and 3 are Return
    flashStructure["Return"] = QVariant(ret);
    int mode = ((v >> 3) & 0x03);  // bit 4 and 5 are Mode
    flashStructure["Mode"] = QVariant(mode);
    bool function = ((v >> 5) & 0x01);  // bit 6 if function
    flashStructure["Function"] = QVariant(function);
    bool redEye = ((v >> 6) & 0x01);  // bit 7 if function
    flashStructure["RedEyeMode"] = QVariant(redEye);
    return KisMetaData::Value(flashStructure);
}

Exiv2::Value* flashKMDToExif(const KisMetaData::Value& value)
{
    uint16_t v = 0;
    QMap<QString, KisMetaData::Value> flashStructure = value.asStructure();
    v = flashStructure["Fired"].asVariant().toBool();
    v |= ((flashStructure["Return"].asVariant().toInt() & 0x03) << 1);
    v |= ((flashStructure["Mode"].asVariant().toInt() & 0x03) << 3);
    v |= ((flashStructure["Function"].asVariant().toInt() & 0x03) << 5);
    v |= ((flashStructure["RedEyeMode"].asVariant().toInt() & 0x03) << 6);
    return new Exiv2::ValueType<uint16_t>(v);
}



// ---- Implementation of KisExifIO ----//
KisExifIO::KisExifIO() : d(new Private)
{
}

KisExifIO::~KisExifIO()
{
    delete d;
}


bool KisExifIO::saveTo(KisMetaData::Store* store, QIODevice* ioDevice, HeaderType headerType) const
{
    ioDevice->open(QIODevice::WriteOnly);
    Exiv2::ExifData exifData;
    if (headerType == KisMetaData::IOBackend::JpegHeader) {
        QByteArray header(6, 0);
        header[0] = 0x45;
        header[1] = 0x78;
        header[2] = 0x69;
        header[3] = 0x66;
        header[4] = 0x00;
        header[5] = 0x00;
        ioDevice->write(header);
    }

    for (QHash<QString, KisMetaData::Entry>::const_iterator it = store->begin();
            it != store->end(); ++it) {
        const KisMetaData::Entry& entry = *it;
        dbgFile << "Trying to save: " << entry.name() << " of " << entry.schema()->prefix() << ":" << entry.schema()->uri();
        QString exivKey = "";
        if (entry.schema()->uri() == KisMetaData::Schema::TIFFSchemaUri) {
            exivKey = "Exif.Image." + entry.name();
        } else if (entry.schema()->uri() == KisMetaData::Schema::EXIFSchemaUri) { // Distinguish between exif and gps
            if (entry.name().left(3) == "GPS") {
                exivKey = "Exif.GPS." + entry.name();
            } else {
                exivKey = "Exif.Photo." + entry.name();
            }
        } else if (entry.schema()->uri() == KisMetaData::Schema::DublinCoreSchemaUri) {
            if (entry.name() == "description") {
                exivKey = "Exif.Image.ImageDescription";
            } else if (entry.name() == "creator") {
                exivKey = "Exif.Image.Artist";
            } else if (entry.name() == "rights") {
                exivKey = "Exif.Image.Copyright";
            }
        } else if (entry.schema()->uri() == KisMetaData::Schema::XMPSchemaUri) {
            if (entry.name() == "ModifyDate") {
                exivKey = "Exif.Image.DateTime";
            } else if (entry.name() == "CreatorTool") {
                exivKey = "Exif.Image.Software";
            }
        } else if (entry.schema()->uri() == KisMetaData::Schema::MakerNoteSchemaUri) {
            if (entry.name() == "RawData") {
                exivKey = "Exif.Photo.MakerNote";
            }
        }
        dbgFile << "Saving " << entry.name() << " to " << exivKey;
        if (exivKey.isEmpty()) {
            dbgFile << entry.qualifiedName() << " is unsavable to EXIF";
        } else {
            Exiv2::ExifKey exifKey(qPrintable(exivKey));
            Exiv2::Value* v = 0;
            if (exivKey == "Exif.Photo.ExifVersion" || exivKey == "Exif.Photo.FlashpixVersion") {
                v = kmdValueToExifVersion(entry.value());
            } else if (exivKey == "Exif.Photo.FileSource") {
                char s[] = { 0x03 };
                v = new Exiv2::DataValue((const Exiv2::byte*)s, 1);
            } else if (exivKey == "Exif.Photo.SceneType") {
                char s[] = { 0x01 };
                v = new Exiv2::DataValue((const Exiv2::byte*)s, 1);
            } else if (exivKey == "Exif.Photo.ComponentsConfiguration") {
                v = kmdIntOrderedArrayToExifArray(entry.value());
            } else if (exivKey == "Exif.Image.Artist") { // load as dc:creator
                KisMetaData::Value creator = entry.value().asArray()[0];
                v = kmdValueToExivValue(creator, Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
            } else if (exivKey == "Exif.Photo.OECF") {
                v = kmdOECFStructureToExifOECF(entry.value());
            } else if (exivKey == "Exif.Photo.DeviceSettingDescription") {
                v = deviceSettingDescriptionKMDToExif(entry.value());
            } else if (exivKey == "Exif.Photo.CFAPattern") {
                v = cfaPatternKMDToExif(entry.value());
            } else if (exivKey == "Exif.Photo.Flash") {
                v = flashKMDToExif(entry.value());
            } else if (exivKey == "Exif.Photo.UserComment") {
                Q_ASSERT(entry.value().type() == KisMetaData::Value::LangArray);
                QMap<QString, KisMetaData::Value> langArr = entry.value().asLangArray();
                if (langArr.contains("x-default")) {
                    v = kmdValueToExivValue(langArr.value("x-default"), Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
                } else if (langArr.size() > 0) {
                    v = kmdValueToExivValue(langArr.begin().value(), Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
                }
            } else {
                dbgFile << exifKey.tag();
                v = kmdValueToExivValue(entry.value(), Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
            }
            if (v && v->typeId() != Exiv2::invalidTypeId) {
                dbgFile << "Saving key" << exivKey; // << " of KMD value" << entry.value();
                exifData.add(exifKey, v);
            } else {
                dbgFile << "No exif value was created for" << entry.qualifiedName() << " as" << exivKey;// << " of KMD value" << entry.value();
            }
        }
    }
#if EXIV2_MAJOR_VERSION == 0 && EXIV2_MINOR_VERSION <= 17
    Exiv2::DataBuf rawData = exifData.copy();
    ioDevice->write((const char*) rawData.pData_, rawData.size_);
#else
    Exiv2::Blob rawData;
    Exiv2::ExifParser::encode(rawData, Exiv2::littleEndian, exifData);
    ioDevice->write((const char*) &*rawData.begin(), rawData.size());
#endif
    ioDevice->close();
    return true;
}

bool KisExifIO::canSaveAllEntries(KisMetaData::Store* /*store*/) const
{
    return false; // It's a known fact that exif can't save all information, but TODO: write the check
}


bool KisExifIO::loadFrom(KisMetaData::Store* store, QIODevice* ioDevice) const
{
    ioDevice->open(QIODevice::ReadOnly);
    if (!ioDevice->isOpen()) {
        return false;
    }
    QByteArray arr = ioDevice->readAll();
    Exiv2::ExifData exifData;
    Exiv2::ByteOrder byteOrder;
#if EXIV2_MAJOR_VERSION == 0 && EXIV2_MINOR_VERSION <= 17
    exifData.load((const Exiv2::byte*)arr.data(), arr.size());
    byteOrder = exifData.byteOrder();
#else
    byteOrder = Exiv2::ExifParser::decode(exifData, (const Exiv2::byte*)arr.data(), arr.size());
#endif
    dbgFile << "Byte order = " << byteOrder << ppVar(Exiv2::bigEndian) << ppVar(Exiv2::littleEndian);
    dbgFile << "There are" << exifData.count() << " entries in the exif section";
    const KisMetaData::Schema* tiffSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::TIFFSchemaUri);
    Q_ASSERT(tiffSchema);
    const KisMetaData::Schema* exifSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::EXIFSchemaUri);
    Q_ASSERT(exifSchema);
    const KisMetaData::Schema* dcSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::DublinCoreSchemaUri);
    Q_ASSERT(dcSchema);
    const KisMetaData::Schema* xmpSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::XMPSchemaUri);
    Q_ASSERT(xmpSchema);
    for (Exiv2::ExifMetadata::const_iterator it = exifData.begin();
            it != exifData.end(); ++it) {
        dbgFile << "Reading info for key" << it->key().c_str();
        if (it->key() == "Exif.Photo.StripOffsets"
                || it->key() == "RowsPerStrip"
                || it->key() == "StripByteCounts"
                || it->key() == "JPEGInterchangeFormat"
                || it->key() == "JPEGInterchangeFormatLength") {
            dbgFile << it->key().c_str() << " is ignored";
        }
        if (it->key() == "Exif.Photo.MakerNote") {
            const KisMetaData::Schema* makerNoteSchema = KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::MakerNoteSchemaUri);
            store->addEntry(KisMetaData::Entry(makerNoteSchema, "RawData", exivValueToKMDValue(it->getValue(), false)));
        } else if (it->key() == "Exif.Image.DateTime") { // load as xmp:ModifyDate
            store->addEntry(KisMetaData::Entry(xmpSchema, "ModifyDate", KisMetaData::Value(exivValueToDateTime(it->getValue()))));
        } else if (it->key() == "Exif.Image.ImageDescription") { // load as "dc:description"
            store->addEntry(KisMetaData::Entry(dcSchema, "description", exivValueToKMDValue(it->getValue(), false)));
        } else if (it->key() == "Exif.Image.Software") { // load as "xmp:CreatorTool"
            store->addEntry(KisMetaData::Entry(xmpSchema, "CreatorTool", exivValueToKMDValue(it->getValue(), false)));
        } else if (it->key() == "Exif.Image.Artist") { // load as dc:creator
            QList<KisMetaData::Value> creators;
            creators.push_back(exivValueToKMDValue(it->getValue(), false));
            store->addEntry(KisMetaData::Entry(dcSchema, "creator", KisMetaData::Value(creators, KisMetaData::Value::OrderedArray)));
        } else if (it->key() == "Exif.Image.Copyright") { // load as dc:rights
            store->addEntry(KisMetaData::Entry(dcSchema, "rights", exivValueToKMDValue(it->getValue(), false)));
        } else if (it->groupName() == "Image") {
            // Tiff tags
            QString fixedTN = it->tagName().c_str();
            if (it->key() == "Exif.Image.ExifTag") {
                dbgFile << "Ignoring " << it->key().c_str();
            } else if (KisMetaData::Entry::isValidName(fixedTN)) {
                store->addEntry(KisMetaData::Entry(tiffSchema, fixedTN, exivValueToKMDValue(it->getValue(), false))) ;
            } else {
                dbgFile << "Invalid tag name: " << fixedTN;
            }
        } else if (it->groupName() == "Photo" || (it->groupName() == "GPS")) {
            // Exif tags (and GPS tags)
            KisMetaData::Value v;
            if (it->key() == "Exif.Photo.ExifVersion" || it->key() == "Exif.Photo.FlashpixVersion") {
                v = exifVersionToKMDValue(it->getValue());
            } else if (it->key() == "Exif.Photo.FileSource") {
                v = KisMetaData::Value(3);
            } else if (it->key() == "Exif.Photo.SceneType") {
                v = KisMetaData::Value(1);
            } else if (it->key() == "Exif.Photo.ComponentsConfiguration") {
                v = exifArrayToKMDIntOrderedArray(it->getValue());
            } else if (it->key() == "Exif.Photo.OECF") {
                v = exifOECFToKMDOECFStructure(it->getValue(), byteOrder);
            } else if (it->key() == "Exif.Photo.DateTimeDigitized" || it->key() == "Exif.Photo.DateTimeOriginal") {
                v = KisMetaData::Value(exivValueToDateTime(it->getValue()));
            } else if (it->key() == "Exif.Photo.DeviceSettingDescription") {
                v = deviceSettingDescriptionExifToKMD(it->getValue());
            } else if (it->key() == "Exif.Photo.CFAPattern") {
                v = cfaPatternExifToKMD(it->getValue(), byteOrder);
            } else if (it->key() == "Exif.Photo.Flash") {
                v = flashExifToKMD(it->getValue());
            } else if (it->key() == "Exif.Photo.UserComment") {
                KisMetaData::Value vUC = exivValueToKMDValue(it->getValue(), false);
                Q_ASSERT(vUC.type() == KisMetaData::Value::Variant);
                QVariant commentVar = vUC.asVariant();
                QString comment;
                if (commentVar.type() == QVariant::String) {
                    comment = commentVar.toString();
                } else if (commentVar.type() == QVariant::ByteArray) {
                    comment = QString::fromLatin1(commentVar.toByteArray().data(), commentVar.toByteArray().size());
                } else {
                    qFatal("Unhandled UserComment value type.");
                }
                KisMetaData::Value vcomment(comment);
                vcomment.addPropertyQualifier("xml:lang", KisMetaData::Value("x-default"));
                QList<KisMetaData::Value> alt;
                alt.append(vcomment);
                v = KisMetaData::Value(alt, KisMetaData::Value::LangArray);
            } else {
                bool forceSeq = false;
                KisMetaData::Value::ValueType arrayType = KisMetaData::Value::UnorderedArray;
                if (it->key() == "Exif.Photo.ISOSpeedRatings") {
                    forceSeq = true;
                    arrayType = KisMetaData::Value::OrderedArray;
                }
                v = exivValueToKMDValue(it->getValue(), forceSeq, arrayType);
            }
            if (it->key() == "Exif.Photo.InteroperabilityTag" || it->key() == "Exif.Photo.0xea1d") { // InteroperabilityTag isn't useful for XMP, 0xea1d isn't a valid Exif tag
                dbgFile << "Ignoring " << it->key().c_str();
            } else {
                store->addEntry(KisMetaData::Entry(exifSchema, it->tagName().c_str(), v));
            }
        } else if (it->groupName() == "Thumbnail") {
            dbgFile << "Ignoring thumbnail tag :" << it->key().c_str();
        } else {
            dbgFile << "Unknown exif tag, cannot load:" << it->key().c_str();
        }
    }
    ioDevice->close();
    return true;
}
