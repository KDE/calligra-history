/*
 * This file is part of Maemo 5 Office UI for KOffice
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Gopalakrishna Bhat A <gopalakbhat@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef MAINWINDOWADAPTOR_H
#define MAINWINDOWADAPTOR_H

#include <QColor>
#include <QtCore/QObject>
#include <QtDBus>
#include <QDBusAbstractAdaptor>

class MainWindow;

class MainWindowAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.nokia.FreOffice.presentation.view")

public:
    MainWindowAdaptor( MainWindow *window);
    ~MainWindowAdaptor();

signals:
    /*!
     * Presentation has entered full screen mode.
     */
    void presentationStarted();
    /*!
     * Presentation has exited from full screen mode.
     */
    void presentationStopped();
    /*!
     * Presentation has moved to the next slide.
     */
    void nextSlide();
    /*!
     * Presentation has moved to the previous slide.
     */
    void previousSlide();

private:
    MainWindow *m_window;
};

#endif
