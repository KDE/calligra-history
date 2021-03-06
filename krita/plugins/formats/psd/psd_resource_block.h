/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef PSD_RESOURCE_BLOCK_H
#define PSD_RESOURCE_BLOCK_H

class QIODevice;

#include <QString>
#include "kis_annotation.h"
#include "psd_resource_section.h"


/**
 * Contains the unparsed contents of the image resource blocks
 *
 * XXX: make KisAnnotations out of the resource blocks so we can load/save them. Need to
 *      expand KisAnnotation to make that work.
 */
class PSDResourceBlock //: public KisAnnotation
{

public:
    PSDResourceBlock();

    bool read(QIODevice* io);
    bool write(QIODevice* io);
    bool valid();

    quint16     identifier;
    QString     name;
    quint32     dataSize;
    QByteArray  data;

    QString error;
};

#endif // PSD_RESOURCE_BLOCK_H
