/* This file is part of the KDE project
 * Copyright (C) 2007 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010 Casper Boemann <cbo@boemann.dk>
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
#ifndef SIMPLEPARAGRAPHWIDGET_H
#define SIMPLEPARAGRAPHWIDGET_H

#include <ui_SimpleParagraphWidget.h>

#include <QWidget>
#include <QTextBlock>

class TextTool;
class KoStyleManager;

class SimpleParagraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimpleParagraphWidget(TextTool *tool, QWidget *parent = 0);

public slots:
    void setCurrentBlock(const QTextBlock &block);
    void setStyleManager(KoStyleManager *sm);

signals:
    void doneWithFocus();
    void insertTableQuick(int, int);

private slots:
    void directionChangeRequested();
    void listStyleChanged(int id);

private:
    enum DirectionButtonState {
        LTR,
        RTL,
        Auto
    };

    void updateDirection(DirectionButtonState state);


    void fillListButtons();

    Ui::SimpleParagraphWidget widget;
    KoStyleManager *m_styleManager;
    bool m_blockSignals;
    QTextBlock m_currentBlock;
    TextTool *m_tool;
    DirectionButtonState m_directionButtonState;
};

#endif
