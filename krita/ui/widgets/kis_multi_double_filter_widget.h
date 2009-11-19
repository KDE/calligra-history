/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_MULTI_DOUBLE_FILTER_WIDGET_H_
#define _KIS_MULTI_DOUBLE_FILTER_WIDGET_H_

#include <vector>
#include <knuminput.h>
#include <kis_config_widget.h>
#include "krita_export.h"

class KisDelayedActionDoubleInput : public KDoubleNumInput
{
    Q_OBJECT

public:
    KisDelayedActionDoubleInput(QWidget * parent, const QString & name);

    void cancelDelayedSignal();

private slots:
    void slotValueChanged();
    void slotTimeToUpdate();

signals:
    void valueChangedDelayed(double value);

private:

    QTimer * m_timer;
};


struct KRITAUI_EXPORT KisDoubleWidgetParam {
    KisDoubleWidgetParam(double nmin, double nmax, double ninitvalue, const QString & label, const QString & nname);
    double min;
    double max;
    double initvalue;
    QString label;
    QString name;
};

typedef std::vector<KisDoubleWidgetParam> vKisDoubleWidgetParam;

class KRITAUI_EXPORT KisMultiDoubleFilterWidget : public KisConfigWidget
{
    Q_OBJECT
public:
    KisMultiDoubleFilterWidget(const QString & filterid, QWidget * parent, const QString & caption, vKisDoubleWidgetParam dwparam);
    virtual void setConfiguration(const KisPropertiesConfiguration * cfg);
    virtual KisPropertiesConfiguration* configuration() const;
public:
    inline qint32 nbValues() const {
        return m_nbdoubleWidgets;
    }
    inline double valueAt(qint32 i) {
        return m_doubleWidgets[i]->value();
    }
private:
    KisDelayedActionDoubleInput** m_doubleWidgets;
    qint32 m_nbdoubleWidgets;
    QString m_filterid;
};

#endif
