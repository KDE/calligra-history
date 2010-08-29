/* This file is part of the KDE project
 * Copyright (C) 2006, 2008-2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Pierre Ducroquet <pinaraf@pinaraf.info>
 * Copyright (C) 2008 Sebastian Sauer <mail@dipe.org>
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

#ifndef KWPAGESTYLE_H
#define KWPAGESTYLE_H

#include "KWord.h"
#include "kword_export.h"

#include <KoPageLayout.h>
#include <KoText.h>

#include <QSharedDataPointer>

class KWPageStylePrivate;
class KoShapeBackground;

/**
 * A page style represents a set of all the properties that change the layout and size
 * of a page and the text area on it.
 *
 * For documents that have a main text auto generated we have a lot of little options
 * to do that. This class wraps all these options.
 *
 * KWord has one KWPageManager per document, the manager collects all page styles by
 * name and allows pages to follow this named style. This means that changing a page
 * style will update all pages that are using that page style with the new properties.
 *
 * The KWPageStyle object is a value class, you can pass it by reference and create it
 * on the stack. You should never use 'new' on the KWPageStyle.
 * The KWPageStyle is a so called QExplicitlySharedDataPointer, which is best explained
 * with a code example;
 * @code
 * KWPageStyle style1("mystyle");
 * KWPageStyle style2 = style1;
 * style1.setHeaderPolicy(KWord::HFTypeEvenOdd); // set it on one
 * Q_ASSERT(style2.headerPolicy() == KWord::HFTypeEvenOdd); // the other changes too
 * @endcode
 */
class KWORD_TEST_EXPORT KWPageStyle
{
public:
    /**
     * Creates an empty, invalid page style.
     */
    KWPageStyle();
    /**
     * constructor, initializing the data to some default values.
     *
     * \p masterPageName The name of this page style.
     */
    explicit KWPageStyle(const QString &mastername);
    /**
     * copy constructor
     *
     * \p ps the original that will be copied
     */
    KWPageStyle(const KWPageStyle &ps);
    KWPageStyle &operator=(const KWPageStyle &ps);
    /// destructor
    ~KWPageStyle();

    /// returns true if the KWPageStyle is valid
    bool isValid() const;

    /**
     * Return the current columns settings.
     */
    KoColumns columns() const;
    /**
     * Set the new columns settings
     */
    void setColumns(const KoColumns &columns);

    /// Return the type of header the pages will get.
    KWord::HeaderFooterType headerPolicy() const;
    /// set the type of header the pages will get.
    void setHeaderPolicy(KWord::HeaderFooterType p);

    /// Return the type of footers the pages will get.
    KWord::HeaderFooterType footerPolicy() const;
    /// Set the type of footers the pages will get.
    void setFooterPolicy(KWord::HeaderFooterType p);

    /**
     * This is the main toggle for all automatically generated frames.
     * The generation and placing of the main text frame, as well as headers, footers,
     * end notes and footnotes for the main text flow is enabled as soon as this is on.
     * Turn it off and all the other settings on this class will be ignored.
     * @param on the big switch for auto-generated frames.
     */
    void setHasMainTextFrame(bool on);
    /**
     * Return if the main text frame, but also the headers/footers etc should be autogenerated.
     */
    bool hasMainTextFrame() const;

    /// return the distance between the main text and the header
    qreal headerDistance() const;
    /**
     * Set the distance between the main text and the header
     * @param distance the distance
     */
    void setHeaderDistance(qreal distance);

    /// return the minimum header height.
    qreal headerMinimumHeight() const;
    /**
     * Set the minimum header height.
     * @param height the height
     */
    void setHeaderMinimumHeight(qreal height);

    /// return the minimum footer height.
    qreal footerMinimumHeight() const;
    /**
     * Set the minimum footer height.
     * @param height the height
     */
    void setFooterMinimumHeight(qreal height);

    /// return the distance between the footer and the frame directly above that (footnote or main)
    qreal footerDistance() const;
    /**
     * Set the distance between the footer and the frame directly above that (footnote or main)
     * @param distance the distance
     */
    void setFooterDistance(qreal distance);

    /// return the distance between the footnote and the main frame.
    qreal footnoteDistance() const;
    /**
     * Set the distance between the footnote and the main frame.
     * @param distance the distance
     */
    void setFootnoteDistance(qreal distance);
    /// return the distance between the main text frame and the end notes frame.
    qreal endNoteDistance() const;
    /**
     * Set the distance between the main text frame and the end notes frame.
     * @param distance the distance
     */
    void setEndNoteDistance(qreal distance);

    /// return the line length of the foot note separator line, in percent of the pagewidth
    int footNoteSeparatorLineLength() const;
    /// set the line length of the foot note separator line, in percent of the pagewidth
    void setFootNoteSeparatorLineLength(int length);

    /// return the thickness of the line (in pt) drawn above the foot notes
    qreal footNoteSeparatorLineWidth() const;
    /// set the thickness of the line (in pt) drawn above the foot notes
    void setFootNoteSeparatorLineWidth(qreal width);

    /// return the pen style used to draw the foot note separator line
    Qt::PenStyle footNoteSeparatorLineType() const;
    /// set the pen style used to draw the foot note separator line
    void setFootNoteSeparatorLineType(Qt::PenStyle type);

    /// return the position on the page for the foot note separator line
    KWord::FootNoteSeparatorLinePos footNoteSeparatorLinePosition() const;
    /// set the position on the page for the foot note separator line
    void setFootNoteSeparatorLinePosition(KWord::FootNoteSeparatorLinePos position);

    /// initialize to default settings
    void clear();

    /// return the pageLayout applied for these pages
    KoPageLayout pageLayout() const;

    /// return true if pages of this style can turn into page-spreads
    bool isPageSpread() const;

    /// set the pageLayout applied for these pages
    void setPageLayout(const KoPageLayout &layout);

    /// get the master page name for this page style.
    QString name() const;

    KoText::Direction direction() const;
    void setDirection(KoText::Direction direction);

    /// Get the background.
    KoShapeBackground *background() const;

    /// set the background/
    void setBackground(KoShapeBackground *background);

    /// get the next page master style/
    QString nextStyleName() const;

    /// set the next page master style/
    void setNextStyleName(const QString &nextStyleName);

    /**
     * Save this page style to ODF.
     */
    KoGenStyle saveOdf() const;

    /**
     * Load this page style from ODF
     */
    void loadOdf(KoOdfLoadingContext &context, const KoXmlElement &masterNode, const KoXmlElement &style, KoResourceManager *documentResources);

    bool operator==(const KWPageStyle &other) const;
    inline bool operator!=(const KWPageStyle &other) const { return ! operator==(other); }
    uint hash() const;

    /// internal
    const KWPageStylePrivate *priv() const;
    /// internal
    KWPageStylePrivate *priv();

    /// detach from shared data and set a new name for this one.
    void detach(const QString &name);

private:
    QExplicitlySharedDataPointer<KWPageStylePrivate> d;
};

KWORD_TEST_EXPORT uint qHash(const KWPageStyle &style);

#endif
