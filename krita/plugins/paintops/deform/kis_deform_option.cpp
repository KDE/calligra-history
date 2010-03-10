/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#include "kis_deform_option.h"
#include "ui_wdgdeformoptions.h"

class KisDeformOptionsWidget: public QWidget, public Ui::WdgDeformOptions
{
public:
    KisDeformOptionsWidget(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

KisDeformOption::KisDeformOption()
        : KisPaintOpOption(i18n("Deform Options"), false)
{
    m_checkable = false;
    m_options = new KisDeformOptionsWidget();

    connect(m_options->deformAmountSPBox, SIGNAL(valueChanged(double)),SIGNAL(sigSettingChanged()));
    connect(m_options->interpolationChBox, SIGNAL(toggled(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->addPaintChBox, SIGNAL(toggled(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->useCounter, SIGNAL(toggled(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->useOldData, SIGNAL(toggled(bool)),SIGNAL(sigSettingChanged()));

    connect(m_options->growBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->shrinkBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->swirlCWBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->swirlCCWBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->moveBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->lensBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->lensOutBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    connect(m_options->colorBtn, SIGNAL(clicked(bool)),SIGNAL(sigSettingChanged()));
    
    setConfigurationPage(m_options);
}

KisDeformOption::~KisDeformOption()
{
    delete m_options;
}

void  KisDeformOption::readOptionSetting(const KisPropertiesConfiguration * config)
{
    m_options->deformAmountSPBox->setValue(config->getDouble(DEFORM_AMOUNT));
    m_options->interpolationChBox->setChecked(config->getBool(DEFORM_USE_BILINEAR));
    m_options->addPaintChBox->setChecked(config->getBool(DEFORM_USE_MOVEMENT_PAINT));
    m_options->useCounter->setChecked(config->getBool(DEFORM_USE_COUNTER));
    m_options->useOldData->setChecked(config->getBool(DEFORM_USE_OLD_DATA));

    int deformAction = config->getInt(DEFORM_ACTION);
    if (deformAction == 1) {
        m_options->growBtn->setChecked(true);
    } else if (deformAction == 2) {
        m_options->shrinkBtn->setChecked(true);
    } else if (deformAction == 3) {
        m_options->swirlCWBtn->setChecked(true);
    } else if (deformAction == 4) {
        m_options->swirlCCWBtn->setChecked(true);
    } else if (deformAction == 5) {
        m_options->moveBtn->setChecked(true);
    } else if (deformAction == 6) {
        m_options->lensBtn->setChecked(true);
    } else if (deformAction == 7) {
        m_options->lensOutBtn->setChecked(true);
    } else if (deformAction == 8) {
        m_options->colorBtn->setChecked(true);
    }
}


void KisDeformOption::writeOptionSetting(KisPropertiesConfiguration* config) const
{
    config->setProperty(DEFORM_AMOUNT, m_options->deformAmountSPBox->value());
    config->setProperty(DEFORM_ACTION, deformAction());
    config->setProperty(DEFORM_USE_BILINEAR, m_options->interpolationChBox->isChecked());
    config->setProperty(DEFORM_USE_MOVEMENT_PAINT, m_options->addPaintChBox->isChecked());
    config->setProperty(DEFORM_USE_COUNTER, m_options->useCounter->isChecked());
    config->setProperty(DEFORM_USE_OLD_DATA, m_options->useOldData->isChecked());
}

int  KisDeformOption::deformAction() const
{
    //TODO: make it nicer using enums or something
    if (m_options->growBtn->isChecked()) {
        return 1;
    } else if (m_options->shrinkBtn->isChecked()) {
        return 2;
    } else if (m_options->swirlCWBtn->isChecked()) {
        return 3;
    } else if (m_options->swirlCCWBtn->isChecked()) {
        return 4;
    } else if (m_options->moveBtn->isChecked()) {
        return 5;
    } else if (m_options->lensBtn->isChecked()) {
        return 6;
    } else if (m_options->lensOutBtn->isChecked()) {
        return 7;
    } else if (m_options->colorBtn->isChecked()) {
        return 8;
    } else {
        return -1;
    }
}

