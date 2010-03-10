/*
 *  Copyright (c) 2008-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#include "kis_sumi_shape_option.h"
#include <klocale.h>

#include "ui_wdgshapeoptions.h"

class KisShapeOptionsWidget: public QWidget, public Ui::WdgShapeOptions
{
public:
    KisShapeOptionsWidget(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

KisSumiShapeOption::KisSumiShapeOption()
        : KisPaintOpOption(i18n("Brush shape"), false)
{
    m_checkable = false;
    m_options = new KisShapeOptionsWidget();

    connect(m_options->oneDimBrushBtn, SIGNAL(toggled(bool)), SIGNAL(sigSettingChanged()));
    connect(m_options->twoDimBrushBtn, SIGNAL(toggled(bool)), SIGNAL(sigSettingChanged()));
    connect(m_options->radiusSpinBox, SIGNAL(valueChanged(int)), SIGNAL(sigSettingChanged()));
    connect(m_options->sigmaSpinBox, SIGNAL(valueChanged(double)), SIGNAL(sigSettingChanged()));

    setConfigurationPage(m_options);
}

KisSumiShapeOption::~KisSumiShapeOption()
{
    delete m_options;
}


int KisSumiShapeOption::radius() const
{
    return m_options->radiusSpinBox->value();
}

void KisSumiShapeOption::setRadius(int radius) const
{
    m_options->radiusSpinBox->setValue(radius);
}


void KisSumiShapeOption::readOptionSetting(const KisPropertiesConfiguration* config)
{
    m_options->radiusSpinBox->setValue(config->getInt(SUMI_RADIUS));
    m_options->sigmaSpinBox->setValue(config->getDouble(SUMI_SIGMA));
    if (config->getBool(SUMI_IS_DIMENSION_1D)) {
        m_options->oneDimBrushBtn->setChecked(true);
    } else {
        m_options->twoDimBrushBtn->setChecked(true);
    }
}


void KisSumiShapeOption::writeOptionSetting(KisPropertiesConfiguration* config) const
{
    config->setProperty(SUMI_RADIUS,radius());
    config->setProperty(SUMI_SIGMA,m_options->sigmaSpinBox->value());
    config->setProperty(SUMI_IS_DIMENSION_1D,m_options->oneDimBrushBtn->isChecked()); 
}

