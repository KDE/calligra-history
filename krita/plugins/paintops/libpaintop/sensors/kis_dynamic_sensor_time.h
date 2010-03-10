/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_DYNAMIC_SENSOR_TIME_H_
#define _KIS_DYNAMIC_SENSOR_TIME_H_

#include "kis_vec.h"

#include "kis_dynamic_sensor.h"
//
class KisDynamicSensorTime : public QObject, public KisDynamicSensor
{
    Q_OBJECT
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    using KisSerializableConfiguration::fromXML;
    using KisSerializableConfiguration::toXML;


    KisDynamicSensorTime();
    virtual ~KisDynamicSensorTime() { }
    virtual double parameter(const KisPaintInformation&);
    virtual void reset();
    virtual QWidget* createConfigurationWidget(QWidget* parent, KisSensorSelector*);
public slots:
    virtual void setPeriodic(bool periodic);
    virtual void setLength(int length);


    virtual void toXML(QDomDocument&, QDomElement&) const;
    virtual void fromXML(const QDomElement&);
private:
    int m_time;
    int m_length;
    bool m_periodic;
    int m_lastTime;
};

#endif
