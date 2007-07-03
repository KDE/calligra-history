/* This file is part of the KDE project
   Copyright 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef KSPREAD_FILTER
#define KSPREAD_FILTER

#include <QString>

#include <KoXmlReader.h>

class KoXmlWriter;

namespace KSpread
{
class Database;
class Sheet;

/**
 * OpenDocument, 8.7.1 Table Filter
 */
class Filter
{
public:
    enum Comparison
    {
        Match,
        NotMatch,
        Equal,
        NotEqual,
        Less,
        Greater,
        LessOrEqual,
        GreaterOrEqual,
        Empty,
        NotEmpty,
        TopValues,
        BottomValues,
        TopPercent,
        BottomPercent
    };

    enum Composition
    {
        AndComposition,
        OrComposition
    };

    enum Mode
    {
        Text,
        Number
    };

    /**
     * Constructor.
     */
    Filter();

    /**
     * Constructor.
     */
    Filter(const Filter& other);

    /**
     * Destructor.
     */
    virtual ~Filter();

    void addCondition(Composition composition,
                      int fieldNumber, Comparison comparison, const QString& value,
                      Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive, Mode mode = Text );

    void removeConditions(int fieldNumber = -1);

    bool isEmpty() const;

    void apply(const Database* database) const;

    bool loadOdf(const KoXmlElement& sheetElement, Sheet* const sheet);
    void saveOdf(KoXmlWriter& xmlWriter) const;

    void dump() const;

private:
    class And;
    class Or;
    class Condition;

    void operator=(const Filter&);

    class Private;
    Private * const d;
};

} // namespace KSpread

#endif // KSPREAD_FILTER
