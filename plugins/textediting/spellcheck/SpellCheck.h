/* This file is part of the KDE project
 * Copyright (C) 2007 Fredy Yanardi <fyanardi@gmail.com>
 * Copyright (C) 2007,2010 Thomas Zander <zander@kde.org>
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

#ifndef SPELLCHECK_H
#define SPELLCHECK_H

#include <KoTextEditingPlugin.h>

#include <sonnet/speller.h>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QPointer>
#include <QQueue>
#include <QTextLayout>

class QTextDocument;
class BgSpellCheck;

class SpellCheck : public KoTextEditingPlugin
{
    Q_OBJECT
public:
    SpellCheck();

    void finishedWord(QTextDocument *document, int cursorPosition);
    void finishedParagraph(QTextDocument *document, int cursorPosition);
    void checkSection(QTextDocument *document, int startPosition, int endPosition);

    QStringList availableBackends() const;
    QStringList availableLanguages() const;

    void setBackgroundSpellChecking(bool b);
    void setSkipAllUppercaseWords(bool b);
    void setSkipRunTogetherWords(bool b);

    QString defaultLanguage() const;
    bool backgroundSpellChecking();
    bool skipAllUppercaseWords();
    bool skipRunTogetherWords();

public slots:
    void setDefaultLanguage(const QString &lang);

private slots:
    void highlightMisspelled(const QString &word, int startPosition, bool misspelled = true);
    void finishedRun();
    void configureSpellCheck();
    void runQueue();

private:
    Sonnet::Speller m_speller;
    QPointer<QTextDocument> m_document;
    QString m_word;
    BgSpellCheck *m_bgSpellCheck;
    struct SpellSections {
        SpellSections(QTextDocument *doc, int start, int end)
            : document(doc)
        {
            from = start;
            to = end;
        }
        QPointer<QTextDocument> document;
        int from;
        int to;
    };
    QQueue<SpellSections> m_documentsQueue;
    bool m_enableSpellCheck;
    bool m_allowSignals;
    bool m_documentIsLoading;
    bool m_isChecking;
    QTextCharFormat m_defaultMisspelledFormat;

    /**
     For a whole text run we accumulate all misspellings and apply them to
     the doc at once when done.
     */
    struct BlockLayout {
        int start;
        int length;
        int checkStart; // in case we partially check this block
        QList<QTextLayout::FormatRange> ranges;
    };
    QList<BlockLayout> m_misspellings;
};

#endif
