/* This file is part of the KDE project
   Copyright (C) 1999,2000 Matthias Kalle Dalheimer <kalle@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include "kchartConfigDialog.h"
#include "kchartConfigDialog.moc"


#include "kchartBackgroundPixmapConfigPage.h"
#include "kchartSubTypeChartPage.h"
#include "kchartHeaderFooterConfigPage.h"
#include "kchartColorConfigPage.h"
#include "kchartParameterPieConfigPage.h"
#include "kchartFontConfigPage.h"
#include "kchartDataConfigPage.h"
#include "kchartComboConfigPage.h"
#include "kchartParameterConfigPage.h"
#include "kchartPieConfigPage.h"
#include "kchartParameter3dConfigPage.h"
#include "kchartLegendConfigPage.h"
#include "kchartLine3dConfigPage.h"
#include "kchartParameterPolarConfigPage.h"

#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>

#include <qradiobutton.h>

#include "kchart_params.h"

namespace KChart
{

KChartConfigDialog::KChartConfigDialog( KChartParams* params,
					QWidget* parent, int flags,
					KoChart::Data *dat, 
					KChartAuxiliary *aux ) :
    QTabDialog( parent, "Chart config dialog", true ),
    m_params( params ),
    m_aux(aux),

    m_datapage(0),
    m_subTypePage(0),
    m_headerfooterpage(0),
    m_legendPage(0),
    m_axespage(0),

    _colorpage(0),
    _parameter3dpage(0),
    _parameterpiepage(0),
    _parameterfontpage(0),
    _piepage(0),
    _backgroundpixpage(0),
    _linepage3d(0),
    _polarpage(0)
{
    // Geometry page
    //_geompage = new KChartGeometryConfigPage( this );
    //addTab( _geompage, i18n( "&Geometry" ) );
    setCaption( i18n( "Chart Config Dialog" ) );

    // Color page
    if (flags & KC_SUBTYPE)
    {
        subtypePage();
    }
    else if(flags & KC_HEADERFOOTER)
    {
        m_headerfooterpage=new KChartHeaderFooterConfigPage(m_params,this);
        addTab( m_headerfooterpage,i18n("Header/Footer"));
    }
    else if(flags & KC_COLORS )
    {
        _colorpage = new KChartColorConfigPage( m_params,  this, dat );
        addTab( _colorpage, i18n( "&Colors" ) );
    }
    else if( flags & KC_FONT )
    {
        _parameterfontpage = new KChartFontConfigPage(m_params,this, dat );
        addTab( _parameterfontpage, i18n( "&Font" ) );
    }
    else if( flags & KC_BACK )
    {
        _backgroundpixpage = new KChartBackgroundPixmapConfigPage( m_params, this );
        addTab( _backgroundpixpage, i18n( "&Background" ) );
    }
    else if( flags & KC_LEGEND )
    {
        m_legendPage = new KChartLegendConfigPage(m_params,this );
        addTab( m_legendPage,i18n("Legend"));
    }
    else if( flags & KC_ALL )
    {
	// The data page
        m_datapage = new KChartDataConfigPage(m_params, this, dat, aux);
        addTab( m_datapage, i18n( "&Data" ) );

	// The subtype page
        subtypePage();

	// The Header/Footer page
        m_headerfooterpage=new KChartHeaderFooterConfigPage(m_params, this);
        addTab( m_headerfooterpage, i18n("Header/Footer"));

	// The legend page
	m_legendPage = new KChartLegendConfigPage(m_params,this );
        addTab( m_legendPage,i18n("Legend"));

	// Add axes page if applicable.
        if( m_params->chartType() != KDChartParams::Pie
	    && m_params->chartType() != KDChartParams::Ring )	{
            m_axespage = new KChartParameterConfigPage(m_params,this );
            addTab( m_axespage, i18n( "&Axes" ) );

        } else if( m_params->chartType() != KDChartParams::Ring ) {
	    // FIXME: I don't understand this tab.  /ingwa
            _parameterpiepage = new KChartParameterPieConfigPage(m_params,this );
            addTab( _parameterpiepage, i18n( "&Axes" ) );

            _piepage = new KChartPieConfigPage(m_params, this );
            addTab( _piepage, i18n( "&Pie" ) );
        }

        _colorpage = new KChartColorConfigPage( m_params,  this, dat );
        addTab( _colorpage, i18n( "&Colors" ) );

        _parameterfontpage = new KChartFontConfigPage(m_params,this, dat );
        addTab( _parameterfontpage, i18n( "&Font" ) );

        _backgroundpixpage = new KChartBackgroundPixmapConfigPage( m_params, this );
        addTab( _backgroundpixpage, i18n( "&Background" ) );

        if( m_params->chartType() == KDChartParams::Bar )
        {
            _parameter3dpage = new KChartParameter3dConfigPage(m_params,this );
            addTab( _parameter3dpage,i18n("3D Parameters"));
        }
        else if( m_params->chartType() == KDChartParams::Line)
        {
            _linepage3d= new KChartLine3dConfigPage(m_params,this);
            addTab( _linepage3d,i18n("3D Line Parameters"));
        }
        else if( m_params->chartType() == KDChartParams::Polar)
        {
            _polarpage=new KChartParameterPolarConfigPage(m_params,this);
            addTab( _polarpage,i18n("Polar Parameters"));
        }

        if( m_params->chartType() == KDChartParams::HiLo &&
            ( m_params->hiLoChartSubType() == KDChartParams::HiLoClose ||
              m_params->hiLoChartSubType() == KDChartParams::HiLoOpenClose ) ) {
            // PENDING(kalle) Combo page needs to be redone completely.
            //         _hlcChart=new KChartComboPage(m_params,this);
//         addTab( _hlcChart, i18n( "HLC Chart" ) );
        }
    }

    //init
    init();

    // setup buttons
    setOKButton( i18n( "&OK" ) );
    setApplyButton( i18n( "&Apply" ) );
#if 0
    setDefaultButton( i18n( "&Defaults" ) );
#else
    setDefaultButton( QString::null );
#endif
    setCancelButton( i18n( "&Cancel" ) );

    connect( this, SIGNAL( applyButtonPressed() ),   this, SLOT( apply() ) );
    connect( this, SIGNAL( defaultButtonPressed() ), this, SLOT( defaults() ) );
}


void KChartConfigDialog::apply()
{
    kdDebug(35001) << "***KChartConfig::apply()\n";
    // Copy application data from dialog into parameter structure that is also
    // being used by the application.

    // Data page
    if (m_datapage)
        m_datapage->apply();

    // color page

    // PENDING(kalle) Replace with equivalents
    //     m_params->BGColor = _colorpage->backgroundColor();
    //     m_params->PlotColor = _colorpage->plotColor();
    //m_params->EdgeColor = _colorpage->edgeColor();
    //     m_params->VolColor = _colorpage->volColor();

    if(_colorpage)
    {

        KDChartAxisParams leftparams = m_params->axisParams( KDChartAxisParams::AxisPosLeft );
        leftparams.setAxisGridColor( _colorpage->gridColor() );
        m_params->setOutlineDataColor( _colorpage->lineColor() );
        KDChartAxisParams rightparams = m_params->axisParams( KDChartAxisParams::AxisPosRight );
        KDChartAxisParams bottomparams = m_params->axisParams( KDChartAxisParams::AxisPosBottom );
        if( _colorpage->xTitleColor().isValid() )
            bottomparams.setAxisLineColor( _colorpage->xTitleColor() );
        else
            bottomparams.setAxisLineColor( QColor() );
        if( _colorpage->yTitleColor().isValid() )
            leftparams.setAxisLineColor( _colorpage->yTitleColor() );
        else
            leftparams.setAxisLineColor( QColor() );
#if 0
        if( _colorpage->yTitle2Color().isValid() )
            rightparams.setAxisLineColor( _colorpage->yTitle2Color() );
        else
            rightparams.setAxisLineColor( QColor() );
#endif
        bottomparams.setAxisLabelsColor( _colorpage->xLabelColor() );
        leftparams.setAxisLabelsColor( _colorpage->yLabelColor() );
#if 0
        rightparams.setAxisLabelsColor( _colorpage->yLabel2Color() );
#endif
        m_params->setAxisParams( KDChartAxisParams::AxisPosBottom, bottomparams );
        m_params->setAxisParams( KDChartAxisParams::AxisPosLeft,   leftparams );
#if 0
        m_params->setAxisParams( KDChartAxisParams::AxisPosRight,  rightparams );
#endif
    }
    if((_piepage && _parameterpiepage) ||  m_axespage )
    {
        if( m_params->chartType() != KDChartParams::Pie )
            m_axespage->apply();
        else
        {
            _parameterpiepage->apply();
            _piepage->apply();
        }
    }

    if( _parameter3dpage && m_params->chartType() == KDChartParams::Bar  )
        _parameter3dpage->apply();

    if( _linepage3d && m_params->chartType() == KDChartParams::Line)
        _linepage3d->apply();

    if(_parameterfontpage)
        _parameterfontpage->apply();

    if(_colorpage)
        _colorpage->apply();

    if( m_subTypePage )
        m_subTypePage->apply();

    if( m_params->chartType() == KDChartParams::HiLo &&
        ( m_params->hiLoChartSubType() == KDChartParams::HiLoClose ||
          m_params->hiLoChartSubType() == KDChartParams::HiLoOpenClose ) )
    {
        /* if( _hlcChart)
           _hlcChart->apply();*/
    }

    if(_backgroundpixpage)
        _backgroundpixpage->apply();


    //     for( uint i = 0; i < NUMDATACOLORS; i++ )
    // 	m_params->_datacolors.setColor( i, _colorpage->dataColor( i ) );
    if(m_legendPage)
        m_legendPage->apply();

    if (m_headerfooterpage)
        m_headerfooterpage->apply();
    if( _polarpage)
        _polarpage->apply();

    // data in the params struct has changed; notify application
    emit dataChanged();
}

void KChartConfigDialog::init()
{
    // Data page
    if (m_datapage)
        m_datapage->init();

    // Color page
    if(_colorpage)
    {
        _colorpage->setLineColor( m_params->outlineDataColor() );
        KDChartAxisParams leftparams( m_params->axisParams( KDChartAxisParams::AxisPosLeft ) );
        KDChartAxisParams rightparams( m_params->axisParams( KDChartAxisParams::AxisPosRight ) );
        KDChartAxisParams bottomparams( m_params->axisParams( KDChartAxisParams::AxisPosBottom ) );
        _colorpage->setGridColor( leftparams.axisGridColor() );
        _colorpage->setXTitleColor( bottomparams.axisLineColor() );
        _colorpage->setYTitleColor( leftparams.axisLineColor() );
#if 0
        _colorpage->setYTitle2Color( rightparams.axisLineColor() );
#endif
        _colorpage->setXLabelColor( bottomparams.axisLabelsColor() );
        _colorpage->setYLabelColor( leftparams.axisLabelsColor() );
#if 0
        _colorpage->setYLabel2Color( rightparams.axisLabelsColor() );
#endif
        // PENDING(kalle) Replace with KDChart equivalents
        //     _colorpage->setBackgroundColor( m_params->BGColor );
        //     _colorpage->setPlotColor( m_params->PlotColor );
        //     _colorpage->setVolColor( m_params->VolColor );
        //_colorpage->setEdgeColor( m_params->EdgeColor );
    }

    if (m_axespage || (_piepage && _parameterpiepage))
    {
        if( m_params->chartType() != KDChartParams::Pie )
        {
            m_axespage->init();
        }
        else
        {
            _parameterpiepage->init();
            _piepage->init();
        }
    }

    if(_parameter3dpage && m_params->chartType() == KDChartParams::Bar  )
        _parameter3dpage->init();

    if( _linepage3d && m_params->chartType() == KDChartParams::Line)
        _linepage3d->init();

    if(_parameterfontpage)
        _parameterfontpage->init();

    if( m_subTypePage )
        m_subTypePage->init();

/*    if( _hlcChart)
    {
        if( m_params->chartType() == KDChartParams::HiLo &&
            ( m_params->hiLoChartSubType() == KDChartParams::HiLoClose ||
              m_params->hiLoChartSubType() == KDChartParams::HiLoOpenClose ) ) {
            _hlcChart->init();
        }
        }*/
    if(_backgroundpixpage)
        _backgroundpixpage->init();
    if(m_legendPage)
        m_legendPage->init();
//     for( uint i = 0; i < NUMDATACOLORS; i++ )
//      	_colorpage->setDataColor( i, m_params->dataColor( i ) );

    if (m_headerfooterpage)
        m_headerfooterpage->init();
    if(_polarpage)
        _polarpage->init();
}


void KChartConfigDialog::subtypePage()
{
    switch( m_params->chartType() ) {
    case KDChartParams::Bar:
        m_subTypePage = new KChartBarSubTypeChartPage( m_params, this );
        break;
    case KDChartParams::Area:
        m_subTypePage = new KChartAreaSubTypeChartPage( m_params, this );
        break;
    case KDChartParams::Line:
        m_subTypePage = new KChartLineSubTypeChartPage( m_params, this );
        break;
    case KDChartParams::HiLo:
        m_subTypePage = new KChartHiloSubTypeChartPage( m_params, this );
        break;
    case KDChartParams::Polar:
        m_subTypePage = new KChartPolarSubTypeChartPage( m_params, this );
        break;
    default:
	// Do nothing.  In particular, don't add a page.
	return;
        ; 
    }
    addTab( m_subTypePage, i18n( "Chart &Subtype" ) );
}


void KChartConfigDialog::defaults()
{
  //FIXME: NYI
}


}  //KChart namespace
