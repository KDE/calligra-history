/* This file is part of the KDE project

   Copyright 2007-2008 Johannes Simon <johannes.simon@gmail.com>
   Copyright 2009      Inge Wallin    <inge@lysator.liu.se>

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

// Own
#include "ChartConfigWidget.h"

// Qt
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QMenu>

// KDE
#include <KLocale>
#include <KIconLoader>
#include <KDebug>
#include <KMessageBox>

// KOffice
#include <interfaces/KoChartModel.h>

// KDChart
#include <KDChartChart>
#include <KDChartPosition>
#include <KDChartCartesianAxis>
#include <KDChartGridAttributes>
#include <KDChartAbstractCartesianDiagram>
#include <KDChartLegend>
#include <KDChartDataValueAttributes>

// KChart
#include "ChartProxyModel.h"
#include "PlotArea.h"
#include "Legend.h"
#include "DataSet.h"
#include "Axis.h"
#include "ui_ChartTableEditor.h"
#include "ui_ChartConfigWidget.h"
#include "NewAxisDialog.h"
#include "AxisScalingDialog.h"
#include "CellRegionDialog.h"
#include "TableEditorDialog.h"
#include "commands/ChartTypeCommand.h"
#include "CellRegionStringValidator.h"
#include "ChartTableModel.h"
#include "TableSource.h"

using namespace KChart;


class ChartConfigWidget::Private
{
public:
    Private( QWidget *parent );
    ~Private();

    // The owner of this struct.
    ChartShape            *shape;

    // Basic properties of the chart.
    ChartType              type;
    ChartSubtype           subtype;
    bool                   threeDMode;

    // Layouts and widgets.
    QVBoxLayout           *leftLayout;
    QVBoxLayout           *rightLayout;
    Ui::ChartConfigWidget  ui;
    bool                   isExternalDataSource;

    // Menus
    QMenu *dataSetBarChartMenu;
    QMenu *dataSetLineChartMenu;
    QMenu *dataSetAreaChartMenu;

    // chart type selection actions
    QAction  *normalBarChartAction;
    QAction  *stackedBarChartAction;
    QAction  *percentBarChartAction;

    QAction  *normalLineChartAction;
    QAction  *stackedLineChartAction;
    QAction  *percentLineChartAction;

    QAction  *normalAreaChartAction;
    QAction  *stackedAreaChartAction;
    QAction  *percentAreaChartAction;

    QAction  *circleChartAction;
    QAction  *ringChartAction;
    QAction  *radarChartAction;

    QAction  *scatterChartAction;
    QAction  *bubbleChartAction;

    QAction  *stockChartAction;
    QAction  *surfaceChartAction;
    QAction  *ganttChartAction;

    // chart type selection actions for datasets
    QAction  *dataSetNormalBarChartAction;
    QAction  *dataSetStackedBarChartAction;
    QAction  *dataSetPercentBarChartAction;

    QAction  *dataSetNormalLineChartAction;
    QAction  *dataSetStackedLineChartAction;
    QAction  *dataSetPercentLineChartAction;

    QAction  *dataSetNormalAreaChartAction;
    QAction  *dataSetStackedAreaChartAction;
    QAction  *dataSetPercentAreaChartAction;

    QAction  *dataSetCircleChartAction;
    QAction  *dataSetRingChartAction;
    QAction  *dataSetRadarChartAction;
    QAction  *dataSetScatterChartAction;
    QAction  *dataSetBubbleChartAction;
    QAction  *dataSetStockChartAction;
    QAction  *dataSetSurfaceChartAction;
    QAction  *dataSetGanttChartAction;

    // Table Editor (a.k.a. the data editor)
    TableEditorDialog    *tableEditorDialog;
    // Source containing all tables the chart uses (name/model pairs)
    TableSource          *tableSource;

    // Legend
    QButtonGroup         *positionButtonGroup;
    int                   lastHorizontalAlignment;
    int                   lastVerticalAlignment;
    KDChart::Position     fixedPosition;
    KDChart::Position     lastFixedPosition;

    int                   selectedDataSet;
    int                   selectedDataSet_CellRegionDialog;

    // Axes
    QList<Axis*>    dataSetAxes;
    QList<Axis*>    axes;
    QList<DataSet*> dataSets;

    // Dialogs
    NewAxisDialog     newAxisDialog;
    AxisScalingDialog axisScalingDialog;
    CellRegionDialog  cellRegionDialog;

    CellRegionStringValidator *cellRegionStringValidator;
};


ChartConfigWidget::Private::Private( QWidget *parent )
    : tableEditorDialog( 0 )
    , newAxisDialog( parent )
    , axisScalingDialog( parent )
    , cellRegionDialog( parent )

{
    lastHorizontalAlignment = 1; // Qt::AlignCenter
    lastVerticalAlignment   = 1; // Qt::AlignCenter
    fixedPosition           = KDChart::Position::East;
    lastFixedPosition       = KDChart::Position::East;
    selectedDataSet = 0;
    shape = 0;
    tableSource = 0;

    type = KChart::LastChartType;
    subtype = KChart::NoChartSubtype;
    threeDMode = false;

    isExternalDataSource = false;
    cellRegionStringValidator = 0;

    dataSetBarChartMenu = 0;
    dataSetLineChartMenu = 0;
    dataSetAreaChartMenu = 0;
    dataSetNormalBarChartAction = 0;
    dataSetStackedBarChartAction = 0;
    dataSetPercentBarChartAction = 0;
    dataSetNormalLineChartAction = 0;
    dataSetStackedLineChartAction = 0;
    dataSetPercentLineChartAction = 0;
    dataSetNormalAreaChartAction = 0;
    dataSetStackedAreaChartAction = 0;
    dataSetPercentAreaChartAction = 0;
    dataSetCircleChartAction = 0;
    dataSetRingChartAction = 0;
    dataSetScatterChartAction = 0;
    dataSetRadarChartAction = 0;
    dataSetStockChartAction = 0;
    dataSetBubbleChartAction = 0;
    dataSetSurfaceChartAction = 0;
    dataSetGanttChartAction = 0;
}

ChartConfigWidget::Private::~Private()
{
}


// ================================================================
//                     class ChartConfigWidget

// TODO:
// 1) Allow user to change axis' "visible" property

/**
 * Returns, if existent, the name of the icon representing
 * a given chart type, following the KDE4 icon naming convention.
 */
static QString chartTypeIcon( ChartType type, ChartSubtype subtype )
{
    switch( type ) {
    case BarChartType:
        switch( subtype ) {
        case NormalChartSubtype:
            return "office-chart-bar";
        case StackedChartSubtype:
            return "office-chart-bar-stacked";
        case PercentChartSubtype:
            return "office-chart-bar-percentage";
        default:
            Q_ASSERT( "Invalid bar chart subtype!" );
        }
    case LineChartType:
        switch( subtype ) {
        case NormalChartSubtype:
            return "office-chart-line";
        case StackedChartSubtype:
            return "office-chart-line-stacked";
        case PercentChartSubtype:
            return "office-chart-line-percentage";
        default:
            Q_ASSERT( "Invalid line chart subtype!" );
        }
    case AreaChartType:
        switch( subtype ) {
        case NormalChartSubtype:
            return "office-chart-area";
        case StackedChartSubtype:
            return "office-chart-area-stacked";
        case PercentChartSubtype:
            return "office-chart-area-percentage";
        default:
            Q_ASSERT( "Invalid area chart subtype!" );
        }
    case CircleChartType:
        return "office-chart-pie";
    case RingChartType:
        return "office-chart-ring";
    case RadarChartType:
        return "office-chart-polar";
    default:
        return "";
    }

    return "";
}

ChartConfigWidget::ChartConfigWidget()
    : d( new Private( this ) )
{
    setObjectName("Chart Type");
    d->ui.setupUi( this );

    // Chart type button with its associated menu
    QMenu *chartTypeMenu = new QMenu( this );
    chartTypeMenu->setIcon( KIcon( "office-chart-bar" ) );

    // Bar charts
    QMenu *barChartMenu = chartTypeMenu->addMenu( KIcon( "office-chart-bar" ), i18n( "Bar Chart" ) );
    d->normalBarChartAction  = barChartMenu->addAction( KIcon( "office-chart-bar" ), i18n("Normal") );
    d->stackedBarChartAction = barChartMenu->addAction( KIcon( "office-chart-bar-stacked" ), i18n("Stacked") );
    d->percentBarChartAction = barChartMenu->addAction( KIcon( "office-chart-bar-percentage" ), i18n("Percent") );

    // Line charts
    QMenu *lineChartMenu = chartTypeMenu->addMenu( KIcon( "office-chart-line" ), i18n( "Line Chart" ) );
    d->normalLineChartAction  = lineChartMenu->addAction( KIcon( "office-chart-line" ), i18n("Normal") );
    d->stackedLineChartAction = lineChartMenu->addAction( KIcon( "office-chart-line-stacked" ), i18n("Stacked") );
    d->percentLineChartAction = lineChartMenu->addAction( KIcon( "office-chart-line-percentage" ), i18n("Percent") );

    // Area charts
    QMenu *areaChartMenu = chartTypeMenu->addMenu( KIcon( "office-chart-area" ), i18n( "Area Chart" ) );
    d->normalAreaChartAction  = areaChartMenu->addAction( KIcon( "office-chart-area" ), i18n("Normal") );
    d->stackedAreaChartAction = areaChartMenu->addAction( KIcon( "office-chart-area-stacked" ), i18n("Stacked") );
    d->percentAreaChartAction = areaChartMenu->addAction( KIcon( "office-chart-area-percentage" ), i18n("Percent") );

    chartTypeMenu->addSeparator();

    // Circular charts: pie and ring
    d->circleChartAction = chartTypeMenu->addAction( KIcon( "office-chart-pie" ), i18n("Pie Chart") );
    d->ringChartAction = chartTypeMenu->addAction( KIcon( "office-chart-ring" ), i18n("Ring Chart") );

    chartTypeMenu->addSeparator();

    // Polar charts: radar
    d->radarChartAction = chartTypeMenu->addAction( KIcon( "office-chart-polar" ), i18n("Polar Chart") );

    chartTypeMenu->addSeparator();

    // X/Y charts: scatter and bubble
    d->scatterChartAction = chartTypeMenu->addAction( KIcon( "office-chart-scatter" ), i18n("Scatter Chart") );
    d->bubbleChartAction = chartTypeMenu->addAction( i18n("Bubble Chart") );

    chartTypeMenu->addSeparator();

    d->stockChartAction = chartTypeMenu->addAction( i18n("Stock Chart") );
    d->surfaceChartAction = chartTypeMenu->addAction( i18n("Surface Chart") );
    d->surfaceChartAction->setEnabled( false );
    d->ganttChartAction = chartTypeMenu->addAction( i18n("Gantt Chart") );
    d->ganttChartAction->setEnabled( false );

    d->ui.chartTypeMenu->setMenu( chartTypeMenu );
    d->ui.chartTypeMenu->setIconSize( QSize( 32, 32 ) );

    connect( chartTypeMenu, SIGNAL( triggered( QAction* ) ),
             this,          SLOT( chartTypeSelected( QAction* ) ) );

    // Data set chart type button
    QMenu *dataSetChartTypeMenu = new QMenu( this );

    // Default chart type is a bar chart
    dataSetChartTypeMenu->setIcon( KIcon( "chart-bar" ) );


    d->dataSetBarChartMenu = dataSetChartTypeMenu->addMenu( KIcon( "office-chart-bar" ), "Bar Chart" );
    d->dataSetNormalBarChartAction  = d->dataSetBarChartMenu->addAction( KIcon( "office-chart-bar" ), i18n("Normal") );
    d->dataSetStackedBarChartAction = d->dataSetBarChartMenu->addAction( KIcon( "office-chart-bar-stacked" ), i18n("Stacked") );
    d->dataSetPercentBarChartAction = d->dataSetBarChartMenu->addAction( KIcon( "office-chart-bar-percentage" ), i18n("Percent") );

    d->dataSetLineChartMenu = dataSetChartTypeMenu->addMenu( KIcon( "office-chart-line" ), "Line Chart" );
    d->dataSetNormalLineChartAction  = d->dataSetLineChartMenu->addAction( KIcon( "office-chart-line" ), i18n("Normal") );
    d->dataSetStackedLineChartAction = d->dataSetLineChartMenu->addAction( KIcon( "office-chart-line-stacked" ), i18n("Stacked") );
    d->dataSetPercentLineChartAction = d->dataSetLineChartMenu->addAction( KIcon( "office-chart-line-percentage" ), i18n("Percent") );

    d->dataSetAreaChartMenu = dataSetChartTypeMenu->addMenu( KIcon( "office-chart-area" ), "Area Chart" );
    d->dataSetNormalAreaChartAction  = d->dataSetAreaChartMenu->addAction( KIcon( "office-chart-area" ), i18n("Normal") );
    d->dataSetStackedAreaChartAction = d->dataSetAreaChartMenu->addAction( KIcon( "office-chart-area-stacked" ), i18n("Stacked") );
    d->dataSetPercentAreaChartAction = d->dataSetAreaChartMenu->addAction( KIcon( "office-chart-area-percentage" ), i18n("Percent") );

    d->dataSetCircleChartAction = dataSetChartTypeMenu->addAction( KIcon( "office-chart-pie" ), i18n("Pie Chart") );
    d->dataSetRingChartAction = dataSetChartTypeMenu->addAction( KIcon( "office-chart-ring" ), i18n("Ring Chart") );
    d->dataSetRadarChartAction = dataSetChartTypeMenu->addAction( KIcon( "office-chart-polar" ), i18n("Polar Chart") );
    d->dataSetStockChartAction = dataSetChartTypeMenu->addAction( i18n("Stock Chart") );
    d->dataSetBubbleChartAction = dataSetChartTypeMenu->addAction( i18n("Bubble Chart") );

    d->dataSetScatterChartAction = dataSetChartTypeMenu->addAction( KIcon( "office-chart-scatter" ), i18n("Scatter Chart") );

    d->ui.dataSetChartTypeMenu->setMenu( dataSetChartTypeMenu );

    connect( dataSetChartTypeMenu, SIGNAL( triggered( QAction* ) ),
             this,                 SLOT( dataSetChartTypeSelected( QAction* ) ) );

    connect( d->ui.dataSetHasChartType, SIGNAL( toggled( bool ) ),
             this,                      SLOT( ui_dataSetHasChartTypeChanged( bool ) ) );

    // "Plot Area" tab
    connect( d->ui.showTitle,    SIGNAL( toggled( bool ) ),
             this,               SIGNAL( showTitleChanged( bool ) ) );
    connect( d->ui.showSubTitle, SIGNAL( toggled( bool ) ),
             this,               SIGNAL( showSubTitleChanged( bool ) ) );
    connect( d->ui.showFooter,   SIGNAL( toggled( bool ) ),
             this,               SIGNAL( showFooterChanged( bool ) ) );

    connect( d->ui.threeDLook, SIGNAL( toggled( bool ) ),
             this,             SLOT( setThreeDMode( bool ) ) );
    connect( d->ui.showLegend, SIGNAL( toggled( bool ) ),
             this,             SIGNAL( showLegendChanged( bool ) ) );

    // "Datasets" tab
    connect( d->ui.datasetColor, SIGNAL( changed( const QColor& ) ),
             this, SLOT( datasetColorSelected( const QColor& ) ) );
    connect( d->ui.datasetShowValues, SIGNAL( toggled( bool ) ),
             this, SLOT( ui_datasetShowValuesChanged( bool ) ) );
    connect( d->ui.gapBetweenBars, SIGNAL( valueChanged( int ) ),
             this, SIGNAL( gapBetweenBarsChanged( int ) ) );
    connect( d->ui.gapBetweenSets, SIGNAL( valueChanged( int ) ),
             this, SIGNAL( gapBetweenSetsChanged( int ) ) );
    connect( d->ui.pieExplodeFactor, SIGNAL( valueChanged( int ) ),
             this, SLOT( ui_dataSetPieExplodeFactorChanged( int ) ) );

    // "Legend" tab
    connect( d->ui.legendTitle, SIGNAL( textChanged( const QString& ) ),
             this, SIGNAL( legendTitleChanged( const QString& ) ) );
    connect( d->ui.legendShowFrame, SIGNAL( toggled( bool ) ),
             this, SIGNAL( legendShowFrameChanged( bool ) ) );
    connect( d->ui.legendOrientationIsVertical, SIGNAL( toggled( bool ) ),
             this, SLOT( setLegendOrientationIsVertical( bool ) ) );

    // Second part of "Plot Area" tab.
    // FIXME: Is there any particular reason it's separated from the Labels?
    d->ui.addAxis->setIcon( KIcon( "list-add" ) );
    d->ui.removeAxis->setIcon( KIcon( "list-remove" ) );

    connect( d->ui.axisTitle, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( ui_axisTitleChanged( const QString& ) ) );
    connect( d->ui.axisShowTitle, SIGNAL( toggled( bool ) ),
             this, SLOT( ui_axisShowTitleChanged( bool ) ) );
    connect( d->ui.axisShowGridLines, SIGNAL( toggled( bool ) ),
             this, SLOT( ui_axisShowGridLinesChanged( bool ) ) );
    connect ( d->ui.axes, SIGNAL( currentIndexChanged( int ) ),
              this, SLOT( ui_axisSelectionChanged( int ) ) );

    connect( d->ui.dataSets, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( ui_dataSetSelectionChanged( int ) ) );
    connect( d->ui.dataSetAxes, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( ui_dataSetAxisSelectionChanged( int ) ) );

    setupDialogs();
    createActions();

    // Activate spin box "acceleration" for "Data Sets"->"Foo Properties"
    // where Foo is one of the chart types with special property settings.
    d->ui.gapBetweenBars->setAccelerated( true );
    d->ui.gapBetweenSets->setAccelerated( true );
    d->ui.pieExplodeFactor->setAccelerated( true );
}

ChartConfigWidget::~ChartConfigWidget()
{
    delete d;
}

void ChartConfigWidget::deleteSubDialogs()
{
    if ( d->tableEditorDialog ) {
        delete d->tableEditorDialog;
        d->tableEditorDialog = 0;
    }
}

void ChartConfigWidget::open( KoShape* shape )
{
    if ( ! shape ) {
        return;
    }

    // There are 3 shapes that we can select: the full chart shape,
    // the plotarea and the legend.
    //
    // Find the selected shape and adapt the tool option window to
    // which of the subshapes of the chart widget that was actually
    // selected.  Then select the tab depending on which one it was.

    // First see if it was the chart shape itself.
    d->shape = dynamic_cast<ChartShape*>( shape );
    if ( !d->shape ) {
        // If not, try to see if it was the plot area.
        PlotArea *plotArea = dynamic_cast<PlotArea*>( shape );
        if ( plotArea ) {
            d->shape = plotArea->parent();
            d->ui.tabWidget->setCurrentIndex( 0 );
        }
        else {
            // And finally try if it was the legend.
            Legend *legend = dynamic_cast<Legend*>( shape );
#ifdef NDEBUG
            Q_UNUSED(legend);
#else
            Q_ASSERT( legend );
#endif
            d->shape = dynamic_cast<ChartShape*>( shape->parent() );
            Q_ASSERT( d->shape );
            d->ui.tabWidget->setCurrentIndex( 2 );
        }
    }

    d->tableSource = d->shape->tableSource();

// NOTE: There's no single source table anymore, a KSpread workbook allows multiple to be used with a chart.
//    KoChart::ChartModel *spreadSheetModel = qobject_cast<KoChart::ChartModel*>( d->shape->internalModel() );
// NOTE: This is obsolete, ChartShape::usesInternalModelOnly() is now used instead.
//    ChartTableModel *tableModel = qobject_cast<ChartTableModel*>( d->shape->model() );
//    d->isExternalDataSource = ( spreadSheetModel != 0 && tableModel == 0 );

    // Update the axis titles
    //d->ui.xAxisTitle->setText( ((KDChart::AbstractCartesianDiagram*)d->shape->chart()->coordinatePlane()->diagram())->axes()[0]->titleText() );
    //d->ui.yAxisTitle->setText( ((KDChart::AbstractCartesianDiagram*)d->shape->chart()->coordinatePlane()->diagram())->axes()[1]->titleText() );

    // Update the legend title
    //d->ui.legendTitle->setText( d->shape->legend()->title() );

    // Fill the data table
    if ( !d->shape->usesInternalModelOnly() ) {
 // FIXME: CellRegion itself together with a TableSource should now be used
 // to validate  the correctness of a table range address.
#if 0
        d->cellRegionStringValidator = new CellRegionStringValidator( spreadSheetModel );
        d->cellRegionDialog.labelDataRegion->setValidator( d->cellRegionStringValidator );
        d->cellRegionDialog.xDataRegion->setValidator( d->cellRegionStringValidator );
        d->cellRegionDialog.yDataRegion->setValidator( d->cellRegionStringValidator );
        d->cellRegionDialog.categoryDataRegion->setValidator( d->cellRegionStringValidator );
#endif

        // If the data source is external, the editData button opens a
        // dialog to edit the data ranges instead of the data itself.
        d->ui.editData->setText( i18n( "Data Ranges..." ) );
        connect( d->ui.editData, SIGNAL( clicked( bool ) ),
                 this, SLOT( slotShowCellRegionDialog() ) );
        connect( d->cellRegionDialog.xDataRegion, SIGNAL( editingFinished() ),
                 this, SLOT( ui_dataSetXDataRegionChanged() ) );
        connect( d->cellRegionDialog.yDataRegion, SIGNAL( editingFinished() ),
                 this, SLOT( ui_dataSetYDataRegionChanged() ) );
        connect( d->cellRegionDialog.labelDataRegion, SIGNAL( editingFinished() ),
                 this, SLOT( ui_dataSetLabelDataRegionChanged() ) );
        //connect( d->cellRegionDialog.customDataRegion, SIGNAL( textEdited( const QString&) ),
        //         this, SLOT( ui_dataSetCustomDataRegionChanged( const QString& ) ) );
        connect( d->cellRegionDialog.categoryDataRegion, SIGNAL( editingFinished() ),
                 this, SLOT( ui_dataSetCategoryDataRegionChanged() ) );
        connect( d->cellRegionDialog.dataSets, SIGNAL( currentIndexChanged( int ) ),
                 this, SLOT( ui_dataSetSelectionChanged_CellRegionDialog( int ) ) );
    }
    else {
        // This part is run when the data source is not external,
        // i.e. the data is handled by the chart shape itself.
        connect( d->ui.editData, SIGNAL( clicked( bool ) ),
                 this,           SLOT( slotShowTableEditor() ) );
    }

    update();
}

void ChartConfigWidget::save()
{
    ChartTypeCommand command( d->shape );
    command.setChartType( d->type, d->subtype );
    command.redo();
}

KAction* ChartConfigWidget::createAction()
{
    return 0;
}

void ChartConfigWidget::chartTypeSelected( QAction *action )
{
    ChartType     type = LastChartType;
    ChartSubtype  subtype = NoChartSubtype;

    // Bar charts
    if ( action == d->normalBarChartAction ) {
        type    = BarChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->stackedBarChartAction ) {
        type    = BarChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->percentBarChartAction ) {
        type    = BarChartType;
        subtype = PercentChartSubtype;
    }

    // Line charts
    else if ( action == d->normalLineChartAction ) {
        type    = LineChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->stackedLineChartAction ) {
        type    = LineChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->percentLineChartAction ) {
        type    = LineChartType;
        subtype = PercentChartSubtype;
    }

    // Area charts
    else if ( action == d->normalAreaChartAction ) {
        type    = AreaChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->stackedAreaChartAction ) {
        type    = AreaChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->percentAreaChartAction ) {
        type    = AreaChartType;
        subtype = PercentChartSubtype;
    }

    // also known as polar chart.
    else if ( action == d->radarChartAction ) {
        type    = RadarChartType;
        subtype = NoChartSubtype;
    }

    // Also known as pie chart
    else if ( action == d->circleChartAction ) {
        type    = CircleChartType;
        subtype = NoChartSubtype;
    }
    else if ( action == d->ringChartAction ) {
        type    = RingChartType;
        subtype = NoChartSubtype;
    }

    else if ( action == d->scatterChartAction ) {
        type    = ScatterChartType;
        subtype = NoChartSubtype;
    }

    else if ( action == d->stockChartAction ) {
        type    = StockChartType;
        subtype = NoChartSubtype;
    }

    else if ( action == d->bubbleChartAction ) {
        type    = BubbleChartType;
        subtype = NoChartSubtype;
    }

    else if ( action == d->surfaceChartAction ) {
        type    = SurfaceChartType;
        subtype = NoChartSubtype;
    }

    else if ( action == d->ganttChartAction ) {
        type    = GanttChartType;
        subtype = NoChartSubtype;
    }


    // o Make sure polar and cartesian plots can't conflict and
    //   don't allow the user to mix these two types
    // o Hide axis configuration options for polar plots
    if ( isPolar( type ) ) {
        setPolarChartTypesEnabled( true );
        setCartesianChartTypesEnabled( false );

        // Pie charts and ring charts have no axes but radar charts do.
        // Disable choosing of attached axis if there is none.
        bool hasAxes = !(type == CircleChartType || type == RingChartType );
        d->ui.axisConfiguration->setEnabled( hasAxes );
        d->ui.dataSetAxes->setEnabled( hasAxes );
        d->ui.dataSetHasChartType->setEnabled( hasAxes );
        d->ui.dataSetChartTypeMenu->setEnabled( hasAxes );
    } else {
        setPolarChartTypesEnabled( false );
        setCartesianChartTypesEnabled( true );

        // All the cartesian chart types have axes.
        d->ui.axisConfiguration->setEnabled( true );
        d->ui.dataSetAxes->setEnabled( true );
        d->ui.dataSetHasChartType->setEnabled( true );
        d->ui.dataSetChartTypeMenu->setEnabled( true );
    }

    emit chartTypeChanged( type );
    emit chartSubTypeChanged( subtype );

    update();
}

/**
 * Enabled/Disabled menu actions to set a polar chart type
 */
void ChartConfigWidget::setPolarChartTypesEnabled( bool enabled )
{
    d->dataSetCircleChartAction->setEnabled( enabled );
    d->dataSetRingChartAction->setEnabled( enabled );
    d->dataSetRadarChartAction->setEnabled( enabled );
}

/**
 * Enabled/Disabled menu actions to set a cartesian chart type
 */
void ChartConfigWidget::setCartesianChartTypesEnabled( bool enabled )
{
    d->dataSetBarChartMenu->setEnabled( enabled );
    d->dataSetLineChartMenu->setEnabled( enabled );
    d->dataSetAreaChartMenu->setEnabled( enabled );
    d->dataSetScatterChartAction->setEnabled( enabled );
    d->dataSetStockChartAction->setEnabled( enabled );
    d->dataSetBubbleChartAction->setEnabled( enabled );
    // FIXME: Enable for:
    // pie, ring?
    //NYI:
    //surface
    //gantt
}

void ChartConfigWidget::ui_dataSetPieExplodeFactorChanged( int percent )
{
    if ( d->selectedDataSet < 0 )
        return;

    DataSet *dataSet = d->dataSets[ d->selectedDataSet ];
    Q_ASSERT( dataSet );
    if ( !dataSet )
        return;

    emit pieExplodeFactorChanged( dataSet, percent );
}

void ChartConfigWidget::ui_dataSetHasChartTypeChanged( bool b )
{
    if ( d->selectedDataSet < 0 )
        return;

    if ( !b ) {
        DataSet *dataSet = d->dataSets[ d->selectedDataSet ];
        Q_ASSERT( dataSet );
        if ( !dataSet )
            return;

        emit dataSetChartTypeChanged( dataSet, LastChartType );
        emit dataSetChartSubTypeChanged( dataSet, NoChartSubtype );
        d->ui.dataSetChartTypeMenu->setIcon( QIcon() );
    }
}

void ChartConfigWidget::dataSetChartTypeSelected( QAction *action )
{
    if ( d->selectedDataSet < 0 )
        return;

    ChartType     type    = LastChartType;
    ChartSubtype  subtype = NoChartSubtype;

    if ( action == d->dataSetNormalBarChartAction ) {
        type    = BarChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->dataSetStackedBarChartAction ) {
        type    = BarChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->dataSetPercentBarChartAction ) {
        type    = BarChartType;
        subtype = PercentChartSubtype;
    }

    else if ( action == d->dataSetNormalLineChartAction ) {
        type    = LineChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->dataSetStackedLineChartAction ) {
        type    = LineChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->dataSetPercentLineChartAction ) {
        type    = LineChartType;
        subtype = PercentChartSubtype;
    }

    else if ( action == d->dataSetNormalAreaChartAction ) {
        type    = AreaChartType;
        subtype = NormalChartSubtype;
    } else if ( action == d->dataSetStackedAreaChartAction ) {
        type    = AreaChartType;
        subtype = StackedChartSubtype;
    } else if ( action == d->dataSetPercentAreaChartAction ) {
        type    = AreaChartType;
        subtype = PercentChartSubtype;
    }

    else if ( action == d->dataSetRadarChartAction )
        type = RadarChartType;
    else if ( action == d->dataSetCircleChartAction )
        type = CircleChartType;
    else if ( action == d->dataSetRingChartAction )
        type = RingChartType;
    else if ( action == d->dataSetScatterChartAction )
        type = ScatterChartType;
    else if ( action == d->dataSetStockChartAction )
        type = StockChartType;
    else if ( action == d->dataSetBubbleChartAction )
        type = BubbleChartType;

    // FIXME: Not supported by KChart yet:
    //surface
    //gantt

    DataSet *dataSet = d->dataSets[ d->selectedDataSet ];
    Q_ASSERT( dataSet );
    if ( !dataSet )
        return;

    QString iconName = chartTypeIcon( type, subtype );
    if ( !iconName.isEmpty() )
        d->ui.dataSetChartTypeMenu->setIcon( KIcon( iconName ) );

    emit dataSetChartTypeChanged( dataSet, type );
    emit dataSetChartSubTypeChanged( dataSet, subtype );

    update();
}

void ChartConfigWidget::chartSubTypeSelected( int type )
{
    d->subtype = (ChartSubtype) type;
    emit chartSubTypeChanged( d->subtype );
}

void ChartConfigWidget::datasetColorSelected( const QColor& color )
{
    if ( d->selectedDataSet < 0 )
        return;

    emit datasetColorChanged( d->dataSets[ d->selectedDataSet ], color );
}

void ChartConfigWidget::setThreeDMode( bool threeD )
{
    d->threeDMode = threeD;
    emit threeDModeToggled( threeD );
}

void ChartConfigWidget::update()
{
    if ( !d->shape )
        return;

    // We only want to update this widget according to the current
    // state of the shape
    // Note that this does not recursively block signals but only those
    // of ChartConfigWidget.
    blockSignals( true );

    // Update cartesian diagram-specific properties
    // Always update, as e.g. name of axis could have changed
    // if ( d->axes != d->shape->plotArea()->axes() ) {
        // Remove old items from the combo box
        d->ui.axes->clear();
        d->ui.dataSetAxes->clear();
        // Sync the internal list
        d->axes = d->shape->plotArea()->axes();
        d->dataSetAxes.clear();

        if ( !d->axes.isEmpty() ) {
            foreach ( Axis *axis, d->axes ) {
                // This automatically calls ui_axisSelectionChanged()
                // after first insertion
                d->ui.axes->addItem( axis->titleText() );
                if ( axis->dimension() == YAxisDimension ) {
                    d->dataSetAxes.append( axis );
                    d->ui.dataSetAxes->blockSignals( true );
                    QString title = axis->titleText();
                    // TODO (post-2.3): Use "Y Axis" as default label instead
                    if ( title.isEmpty() )
                        title = i18n( "Axis %1", d->ui.dataSetAxes->count() + 1 );
                    d->ui.dataSetAxes->addItem( title );
                    d->ui.dataSetAxes->blockSignals( false );
                }
            }
        } else {
            d->ui.axisShowGridLines->blockSignals( true );
            d->ui.axisShowGridLines->setChecked( false );
            d->ui.axisShowGridLines->setEnabled( false );
            d->ui.axisShowGridLines->blockSignals( false );

            d->ui.axisShowTitle->blockSignals( true );
            d->ui.axisShowTitle->setChecked( false );
            d->ui.axisShowTitle->setEnabled( false );
            d->ui.axisShowTitle->blockSignals( false );

            d->ui.axisTitle->blockSignals( true );
            d->ui.axisTitle->setText( "" );
            d->ui.axisTitle->setEnabled( false );
            d->ui.axisTitle->blockSignals( false );
        }
    // }

    // Update "Labels" section in "Plot Area" tab
    d->ui.showTitle->setChecked( d->shape->title()->isVisible() );
    d->ui.showSubTitle->setChecked( d->shape->subTitle()->isVisible() );
    d->ui.showFooter->setChecked( d->shape->footer()->isVisible() );

    // Update "Bar Properties" in "Data Sets" tab
    d->ui.gapBetweenBars->setValue( d->shape->plotArea()->gapBetweenBars() );
    d->ui.gapBetweenSets->setValue( d->shape->plotArea()->gapBetweenSets() );

    if (    d->type    != d->shape->chartType()
         || d->subtype != d->shape->chartSubType() )
    {
        // Update the chart type specific settings in the "Data Sets" tab
        bool needSeparator = false;
        if ( d->shape->chartType() == BarChartType ) {
            d->ui.barProperties->show();
            d->ui.pieProperties->hide();
            needSeparator = true;
        } else if ( d->shape->chartType() == CircleChartType ) {
            d->ui.barProperties->hide();
            d->ui.pieProperties->show();
            needSeparator = true;
        } else if ( d->shape->chartType() == RingChartType ) {
            d->ui.barProperties->hide();
            d->ui.pieProperties->show();
            needSeparator = true;
        } else {
            d->ui.barProperties->hide();
            d->ui.pieProperties->hide();
        }
        d->ui.propertiesSeparator->setVisible( needSeparator );

        // Set the chart type icon in the chart type button.
        QString iconName = chartTypeIcon( d->shape->chartType(), d->shape->chartSubType() );
        if ( !iconName.isEmpty() )
            d->ui.chartTypeMenu->setIcon( KIcon( iconName ) );

        // Make sure we only allow legal chart type combinations
        if ( isPolar( d->shape->chartType() ) ) {
            setPolarChartTypesEnabled( true );
            setCartesianChartTypesEnabled( false );
        } else {
            setPolarChartTypesEnabled( false );
            setCartesianChartTypesEnabled( true );
        }

        // ...and finally save the new chart type and subtype.
        d->type    = d->shape->chartType();
        d->subtype = d->shape->chartSubType();
    }

    // If the datasets have changed, set up the new ones.
    if ( d->shape->plotArea()->dataSets() != d->dataSets ) {
        d->dataSets = d->shape->plotArea()->dataSets();
        d->ui.dataSets->clear();
        d->cellRegionDialog.dataSets->clear();
        foreach ( DataSet *dataSet, d->dataSets ) {
            QString title = dataSet->labelData().toString();
            if ( title.isEmpty() )
                title = i18n( "Data Set %1", d->ui.dataSets->count() + 1 );
            d->ui.dataSets->addItem( title );
            d->cellRegionDialog.dataSets->addItem( title );
        }

        // Select the first data set
        ui_dataSetSelectionChanged( 0 );
        ui_dataSetSelectionChanged_CellRegionDialog( 0 );
    }

    // If the "3D" checkbox is checked, then adapt the chart to that.
    d->ui.threeDLook->setChecked( d->shape->isThreeD() );
    if ( d->shape->legend() ) {
        d->ui.legendTitle->blockSignals( true );
        d->ui.legendTitle->setText( d->shape->legend()->title() );
        d->ui.legendTitle->blockSignals( false );
    }

    blockSignals( false );
}


void ChartConfigWidget::slotShowTableEditor()
{
    if ( !d->tableEditorDialog ) {
        d->tableEditorDialog = new TableEditorDialog;
        d->tableEditorDialog->setProxyModel( d->shape->proxyModel() );
        d->tableEditorDialog->setModel( d->shape->internalModel() );
    }

    d->tableEditorDialog->show();
}


void ChartConfigWidget::slotShowCellRegionDialog()
{
    // Update regions of selected dataset
    int selectedDataSet = d->cellRegionDialog.dataSets->currentIndex();
    ui_dataSetSelectionChanged_CellRegionDialog( selectedDataSet );

    d->cellRegionDialog.show();
}


void ChartConfigWidget::setLegendOrientation( int boxEntryIndex )
{
    Q_UNUSED(boxEntryIndex);
    //emit legendOrientationChanged( ( Qt::Orientation ) ( d->ui.orientation->itemData( boxEntryIndex ).toInt() ) );
}
/*
void ChartConfigWidget::setLegendShowTitle( bool show )
{
    if ( show ) {
        d->ui.legendTitle->setEnabled( true );
        emit legendTitleChanged( d->ui.legendTitle->text() );
    } else {
        d->ui.legendTitle->setEnabled( false );
        emit legendTitleChanged( "" );
    }
}
*/
void ChartConfigWidget::setLegendAlignment( int boxEntryIndex )
{
    Q_UNUSED(boxEntryIndex);
    if (    d->fixedPosition == KDChart::Position::North
         || d->fixedPosition == KDChart::Position::South ) {
        //d->lastHorizontalAlignment = d->ui.alignment->currentIndex();
     } else if (   d->fixedPosition == KDChart::Position::East
                || d->fixedPosition == KDChart::Position::West ) {
        //d->lastVerticalAlignment = d->ui.alignment->currentIndex();
    }
    //emit legendAlignmentChanged( ( Qt::Alignment ) ( d->ui.alignment->itemData( boxEntryIndex ).toInt() ) );
}

void ChartConfigWidget::setLegendFixedPosition( int buttonGroupIndex )
{
    Q_UNUSED(buttonGroupIndex);
    d->lastFixedPosition = d->fixedPosition;
    //d->fixedPosition = buttonIndexToFixedPosition[ buttonGroupIndex ];
    //emit legendFixedPositionChanged( buttonIndexToFixedPosition[ buttonGroupIndex ] );
}

void ChartConfigWidget::updateFixedPosition( Position position )
{
    Q_UNUSED(position);
/*
    if (    position == KDChart::Position::North
         || position == KDChart::Position::South ) {
        d->ui.alignment->setEnabled( true );
        d->ui.alignment->setItemText( 0, i18n( "Left" ) );
        d->ui.alignment->setItemData( 0, Qt::AlignLeft );
        d->ui.alignment->setItemData( 1, Qt::AlignCenter );
        d->ui.alignment->setItemText( 2, i18n( "Right" ) );
        d->ui.alignment->setItemData( 2, Qt::AlignRight );
        // Set the alignment to the one last used for horizontal legend alignment
        if (    d->lastFixedPosition != KDChart::Position::North
             && d->lastFixedPosition != KDChart::Position::South ) {
            // Make sure that the combobox gets updated. Since we changed the values of the entries,
            // same index doesn't mean same value, though it will think so. Solution: Select no entry first
            d->ui.alignment->blockSignals( true );
            d->ui.alignment->setCurrentIndex( -1 );
            d->ui.alignment->blockSignals( false );

            d->ui.alignment->setCurrentIndex( d->lastHorizontalAlignment );
        }
    } else if (    position == KDChart::Position::East
                || position == KDChart::Position::West ) {
        d->ui.alignment->setEnabled( true );
        d->ui.alignment->setItemText( 0, i18n( "Top" ) );
        d->ui.alignment->setItemData( 0, Qt::AlignTop );
        d->ui.alignment->setItemData( 1, Qt::AlignCenter );
        d->ui.alignment->setItemText( 2, i18n( "Bottom" ) );
        d->ui.alignment->setItemData( 2, Qt::AlignBottom );
        // Set the alignment to the one last used for vertical legend alignment
        if (    d->lastFixedPosition != KDChart::Position::East
             && d->lastFixedPosition != KDChart::Position::West ) {
            // Make sure that the combobox gets updated. Since we changed the values of the entries,
            // same index doesn't mean same value, though it will think so. Solution: Select no entry first
            d->ui.alignment->blockSignals( true );
            d->ui.alignment->setCurrentIndex( -1 );
            d->ui.alignment->blockSignals( false );

            d->ui.alignment->setCurrentIndex( d->lastVerticalAlignment );
        }
    } else {
        d->ui.alignment->setEnabled( false );
    }

    for( int i = 0; i < NUM_FIXED_POSITIONS; i++ ) {
        if( position == buttonIndexToFixedPosition[i] ) {
            if ( d->positionButtonGroup->checkedId() != i )
                d->positionButtonGroup->button( i )->setChecked( true );
            break;
        }
    }
*/
}


void ChartConfigWidget::setupDialogs()
{
    // Adding/removing axes
    connect( d->ui.addAxis, SIGNAL( clicked() ),
             this, SLOT( ui_addAxisClicked() ) );
    connect( d->ui.removeAxis, SIGNAL( clicked() ),
             this, SLOT( ui_removeAxisClicked() ) );
    connect( &d->newAxisDialog, SIGNAL( accepted() ),
             this, SLOT( ui_axisAdded() ) );

    // Axis scaling
    connect( d->ui.axisScalingButton, SIGNAL( clicked() ),
             this, SLOT( ui_axisScalingButtonClicked() ) );
    connect( d->axisScalingDialog.logarithmicScaling, SIGNAL( toggled( bool ) ),
             this, SLOT( ui_axisUseLogarithmicScalingChanged( bool ) ) );
    connect( d->axisScalingDialog.stepWidth, SIGNAL( valueChanged( double ) ),
             this, SLOT( ui_axisStepWidthChanged( double ) ) );
    connect ( d->axisScalingDialog.automaticStepWidth, SIGNAL( toggled( bool ) ),
              this, SLOT( ui_axisUseAutomaticStepWidthChanged( bool ) ) );
    connect( d->axisScalingDialog.subStepWidth, SIGNAL( valueChanged( double ) ),
             this, SLOT( ui_axisSubStepWidthChanged( double ) ) );
    connect ( d->axisScalingDialog.automaticSubStepWidth, SIGNAL( toggled( bool ) ),
              this, SLOT( ui_axisUseAutomaticSubStepWidthChanged( bool ) ) );
}

void ChartConfigWidget::createActions()
{
}

void ChartConfigWidget::selectDataset( int dataset )
{
    if ( dataset >= 0 ) {
        d->ui.datasetColorLabel->setEnabled( true );
        d->ui.datasetColor->setEnabled( true );

        d->ui.datasetColor->blockSignals( true );
        //d->ui.datasetColor->setColor( d->shape->plotArea()->kdDiagram()->brush( dataset ).color() );
        d->ui.datasetColor->blockSignals( false );

        d->ui.datasetShowValues->blockSignals( true );
        //d->ui.datasetShowValues->setChecked( d->shape->plotArea()->kdDiagram()->dataValueAttributes( dataset ).isVisible() );
        d->ui.datasetShowValues->blockSignals( false );
    } else {
        d->ui.datasetColorLabel->setEnabled( false );
        d->ui.datasetColor->setEnabled( false );
    }
    d->selectedDataSet = dataset;
}

void ChartConfigWidget::setLegendOrientationIsVertical( bool b )
{
    if ( b )
        emit legendOrientationChanged( Qt::Vertical );
    else
        emit legendOrientationChanged( Qt::Horizontal );
}

void ChartConfigWidget::ui_axisSelectionChanged( int index )
{
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    Axis *axis = d->axes[ index ];

    // Count how many axes there are of the same dimension
    int numAxesOfSameDimension = 0;
    foreach ( Axis *a, d->axes )
        if ( axis->dimension() == a->dimension() )
            numAxesOfSameDimension++;

    // Don't let the user remove the last axis of a particular dimension
    d->ui.removeAxis->setEnabled( numAxesOfSameDimension > 1 );

    d->ui.axisTitle->blockSignals( true );
    d->ui.axisTitle->setText( axis->titleText() );
    d->ui.axisTitle->blockSignals( false );
    d->ui.axisShowTitle->blockSignals( true );
    d->ui.axisShowTitle->setChecked( axis->title()->isVisible() );
    d->ui.axisShowTitle->blockSignals( false );
    d->ui.axisShowGridLines->blockSignals( true );
    d->ui.axisShowGridLines->setChecked( axis->showMajorGrid() || axis->showMinorGrid() );
    d->ui.axisShowGridLines->blockSignals( false );

    d->axisScalingDialog.logarithmicScaling->blockSignals( true );
    if ( axis->dimension() == YAxisDimension )
        d->axisScalingDialog.logarithmicScaling->setEnabled( true );
    else
        d->axisScalingDialog.logarithmicScaling->setEnabled( false );
    d->axisScalingDialog.logarithmicScaling->blockSignals( false );

    d->axisScalingDialog.stepWidth->blockSignals( true );
    d->axisScalingDialog.stepWidth->setValue( axis->majorInterval() );
    d->axisScalingDialog.stepWidth->blockSignals( false );

    d->axisScalingDialog.subStepWidth->blockSignals( true );
    d->axisScalingDialog.subStepWidth->setValue( axis->minorInterval() );
    d->axisScalingDialog.subStepWidth->blockSignals( false );

    d->axisScalingDialog.automaticStepWidth->blockSignals( true );
    d->axisScalingDialog.automaticStepWidth->setChecked( axis->useAutomaticMajorInterval() );
    d->axisScalingDialog.stepWidth->setEnabled( !axis->useAutomaticMajorInterval() );
    d->axisScalingDialog.automaticStepWidth->blockSignals( false );

    d->axisScalingDialog.automaticSubStepWidth->blockSignals( true );
    d->axisScalingDialog.automaticSubStepWidth->setChecked( axis->useAutomaticMinorInterval() );
    d->axisScalingDialog.subStepWidth->setEnabled( !axis->useAutomaticMinorInterval() );
    d->axisScalingDialog.automaticSubStepWidth->blockSignals( false );
}


void ChartConfigWidget::ui_dataSetXDataRegionChanged()
{
    // Check for valid index
    if ( d->selectedDataSet_CellRegionDialog < 0 )
        return;

    const QString regionString = d->cellRegionDialog.xDataRegion->text();
    const CellRegion region( d->tableSource, regionString );

    DataSet *dataSet = d->dataSets[ d->selectedDataSet_CellRegionDialog ];

    emit dataSetXDataRegionChanged( dataSet, region );
}

void ChartConfigWidget::ui_dataSetYDataRegionChanged()
{
    // Check for valid index
    if ( d->selectedDataSet_CellRegionDialog < 0 )
        return;

    const QString regionString = d->cellRegionDialog.yDataRegion->text();
    const CellRegion region( d->tableSource, regionString );

    DataSet *dataSet = d->dataSets[ d->selectedDataSet_CellRegionDialog ];

    emit dataSetYDataRegionChanged( dataSet, region );
}

void ChartConfigWidget::ui_dataSetCustomDataRegionChanged()
{
    // Only makes sense when bubble charts are implemented
    // TODO: ui_dataSetCustomDataRegionChanged
    return;

  /*
    // Check for valid index
    if ( d->selectedDataSet_CellRegionDialog < 0 )
        return;

    const QString region = d->cellRegionDialog.customDataRegion->text();

    DataSet *dataSet = d->dataSets[ d->selectedDataSet_CellRegionDialog ];

    emit dataSetCustomDataRegionChanged( dataSet, region );
    */
}

void ChartConfigWidget::ui_dataSetCategoryDataRegionChanged()
{
    // Check for valid index
    if ( d->selectedDataSet_CellRegionDialog < 0 )
        return;

    const QString regionString = d->cellRegionDialog.categoryDataRegion->text();
    const CellRegion region( d->tableSource, regionString );

    DataSet *dataSet = d->dataSets[ d->selectedDataSet_CellRegionDialog ];

    emit dataSetCategoryDataRegionChanged( dataSet, region );
}

void ChartConfigWidget::ui_dataSetLabelDataRegionChanged()
{
    // Check for valid index
    if ( d->selectedDataSet_CellRegionDialog < 0 )
        return;

    const QString regionString = d->cellRegionDialog.labelDataRegion->text();
    const CellRegion region( d->tableSource, regionString );

    DataSet *dataSet = d->dataSets[ d->selectedDataSet_CellRegionDialog ];

    emit dataSetLabelDataRegionChanged( dataSet, region );
}

void ChartConfigWidget::ui_dataSetSelectionChanged_CellRegionDialog( int index )
{
    // Check for valid index
    if ( index < 0 || index >= d->dataSets.size() )
        return;

    DataSet *dataSet = d->dataSets[ index ];
    const int dimensions = dataSet->dimension();

    d->cellRegionDialog.labelDataRegion->setText( dataSet->labelDataRegion().toString() );
    if ( dimensions > 1 )
    {
        d->cellRegionDialog.xDataRegion->setEnabled( true );
        d->cellRegionDialog.xDataRegion->setText( dataSet->xDataRegion().toString() );
    }
    else
        d->cellRegionDialog.xDataRegion->setEnabled( false );
    d->cellRegionDialog.yDataRegion->setText( dataSet->yDataRegion().toString() );
    d->cellRegionDialog.categoryDataRegion->setText( dataSet->categoryDataRegion().toString() );

    d->selectedDataSet_CellRegionDialog = index;
}

void ChartConfigWidget::ui_dataSetSelectionChanged( int index )
{
    // Check for valid index
    if ( index < 0 || index >= d->dataSets.size() )
        return;

    DataSet *dataSet = d->dataSets[ index ];
    //d->ui.datasetColor->setText( axis->titleText() );
    d->ui.dataSetAxes->blockSignals( true );
    d->ui.dataSetAxes->setCurrentIndex( d->dataSetAxes.indexOf( dataSet->attachedAxis() ) );
    d->ui.dataSetAxes->blockSignals( false );

    d->ui.datasetColor->blockSignals( true );
    d->ui.datasetColor->setColor( dataSet->color() );
    d->ui.datasetColor->blockSignals( false );

    d->ui.datasetShowValues->blockSignals( true );
    d->ui.datasetShowValues->setChecked( dataSet->valueLabelType() == DataSet::RealValueLabel );
    d->ui.datasetShowValues->blockSignals( false );

    d->ui.dataSetShowLabels->blockSignals( true );
    d->ui.dataSetShowLabels->setChecked( dataSet->showLabels() );
    d->ui.dataSetShowLabels->blockSignals( false );

    if ( dataSet->chartType() != LastChartType ) {
        d->ui.dataSetHasChartType->blockSignals( true );
        d->ui.dataSetHasChartType->setChecked( true );
        d->ui.dataSetHasChartType->blockSignals( false );
        d->ui.dataSetChartTypeMenu->setEnabled( true );
    }
    else {
        d->ui.dataSetHasChartType->blockSignals( true );
        d->ui.dataSetHasChartType->setChecked( false );
        d->ui.dataSetHasChartType->blockSignals( false );
        d->ui.dataSetChartTypeMenu->setEnabled( false );
    }

    Q_ASSERT( d->ui.dataSetChartTypeMenu->menu() );

    if ( dataSet->chartType() == LastChartType ) {
        d->ui.dataSetChartTypeMenu->setIcon( QIcon() );
    }
    else {
        switch ( dataSet->chartType() ) {
        case BarChartType:
            switch ( dataSet->chartSubType() ) {
            case StackedChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_bar_stacked" ) );
                break;
            case PercentChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_bar_layer" ) );
                break;
            default:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_bar_beside" ) );
            }
            break;
        case LineChartType:
            switch ( dataSet->chartSubType() ) {
            case StackedChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_line_stacked" ) );
                break;
            case PercentChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_line_percent" ) );
                break;
            default:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_line_normal" ) );
            }
            break;
        case AreaChartType:
            switch ( dataSet->chartSubType() ) {
            case StackedChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_area_stacked" ) );
                break;
            case PercentChartSubtype:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_area_percent" ) );
                break;
            default:
                d->ui.dataSetChartTypeMenu->setIcon( KIcon( "chart_area_normal" ) );
            }
            break;
        case CircleChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_circle_normal" ) );
            break;
        case RingChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_ring_normal" ) );
            break;
        case ScatterChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_scatter_normal" ) );
            break;
        case RadarChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_radar_normal" ) );
            break;
        case StockChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_stock_normal" ) );
            break;
        case BubbleChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_bubble_normal" ) );
            break;
        case SurfaceChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_surface_normal" ) );
            break;
        case GanttChartType:
            d->ui.dataSetChartTypeMenu->menu()->setIcon( KIcon( "chart_gantt_normal" ) );
            break;

            // Fixes a warning that LastChartType isn't handled.
        default:
            break;
        }
    }

    d->selectedDataSet = index;
}

void ChartConfigWidget::ui_dataSetAxisSelectionChanged( int index )
{
    if ( d->ui.dataSets->currentIndex() < 0 || d->ui.dataSets->currentIndex() >= d->dataSets.count() )
        return;
    DataSet *dataSet = d->dataSets[ d->ui.dataSets->currentIndex() ];

    if ( index < 0 || index >= d->dataSetAxes.count() )
        return;
    Axis *axis = d->dataSetAxes[ index ];

    emit dataSetAxisChanged( dataSet, axis );
}

void ChartConfigWidget::ui_axisTitleChanged( const QString& title )
{
    if( d->ui.axes->currentIndex() < 0 || d->ui.axes->currentIndex() >= d->axes.size() )
        return;

    const int index = d->ui.axes->currentIndex();

    // TODO: This can surely be done better
    int dataSetAxisIndex = d->dataSetAxes.indexOf( d->axes[index] );
    d->ui.dataSetAxes->setItemText( dataSetAxisIndex, title );

    d->ui.axes->setItemText( index, title );
    emit axisTitleChanged( d->axes[ index ], title );
}

void ChartConfigWidget::ui_axisShowTitleChanged( bool b )
{
    if( d->ui.axes->currentIndex() < 0 || d->ui.axes->currentIndex() >= d->axes.size() )
        return;

    // To hide the axis title, we pass an empty string
    emit axisShowTitleChanged( d->axes[ d->ui.axes->currentIndex() ], b );
}

void ChartConfigWidget::ui_axisShowGridLinesChanged( bool b )
{
    if( d->ui.axes->currentIndex() < 0 || d->ui.axes->currentIndex() >= d->axes.size() )
        return;

    emit axisShowGridLinesChanged( d->axes[ d->ui.axes->currentIndex() ], b );
}

void ChartConfigWidget::ui_axisAdded()
{
    AxisPosition position;
    if ( d->newAxisDialog.positionIsTop->isChecked() )
        position = TopAxisPosition;
    else if ( d->newAxisDialog.positionIsBottom->isChecked() )
        position = BottomAxisPosition;
    else if ( d->newAxisDialog.positionIsLeft->isChecked() )
        position = LeftAxisPosition;
    else
        position = RightAxisPosition;

    emit axisAdded( position, d->newAxisDialog.title->text() );
    update();

    if( d->ui.axes->count() > 0 )
        d->ui.axes->setCurrentIndex( d->ui.axes->count() - 1 );
}

void ChartConfigWidget::ui_addAxisClicked()
{
    d->newAxisDialog.show();
}

void ChartConfigWidget::ui_removeAxisClicked()
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    if ( KMessageBox::questionYesNo( this,
                                     i18n("Are you sure you want to remove this axis? All settings specific to this axis will be lost."),
                                     i18n("Axis Removal Confirmation") ) != KMessageBox::Yes )
        return;

    emit axisRemoved( d->axes[ index ] );
    update();

    // Select the axis after the current selection, if possible
    if ( d->ui.axes->count() > 0 ) {
        index = qMin( index, d->ui.axes->count() - 1 );
        d->ui.axes->setCurrentIndex( index );
    }
}

void ChartConfigWidget::ui_axisUseLogarithmicScalingChanged( bool b )
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    emit axisUseLogarithmicScalingChanged( d->axes[ index ], b );
}

void ChartConfigWidget::ui_axisStepWidthChanged( double width )
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    emit axisStepWidthChanged( d->axes[ index ], width );
}

void ChartConfigWidget::ui_axisSubStepWidthChanged( double width )
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    emit axisSubStepWidthChanged( d->axes[ index ], width );
}

void ChartConfigWidget::ui_axisUseAutomaticStepWidthChanged( bool b )
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    emit axisUseAutomaticStepWidthChanged( d->axes[ index ], b );
}

void ChartConfigWidget::ui_axisUseAutomaticSubStepWidthChanged( bool b )
{
    int index = d->ui.axes->currentIndex();
    // Check for valid index
    if ( index < 0 || index >= d->axes.size() )
        return;

    emit axisUseAutomaticSubStepWidthChanged( d->axes[ index ], b );
}

void ChartConfigWidget::ui_axisScalingButtonClicked()
{
    d->axisScalingDialog.show();
}

void ChartConfigWidget::ui_datasetShowValuesChanged( bool b )
{
    if ( d->selectedDataSet < 0 || d->selectedDataSet >= d->dataSets.count() )
        return;

    emit datasetShowValuesChanged( d->dataSets[ d->selectedDataSet ], b );
}

void ChartConfigWidget::ui_datasetShowLabelsChanged( bool b )
{
    if ( d->selectedDataSet < 0 || d->selectedDataSet >= d->dataSets.count() )
        return;

    emit datasetShowValuesChanged( d->dataSets[ d->selectedDataSet ], b );
}


#include "ChartConfigWidget.moc"

