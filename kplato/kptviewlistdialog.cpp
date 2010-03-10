/* This file is part of the KDE project
   Copyright (C) 2007 - 2010 Dag Andersen <danders@get2net.dk>

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
 * Boston, MA 02110-1301, USA.
*/

#include "kptviewlistdialog.h"
#include "kptviewlist.h"
#include "kptview.h"
#include "reports/reportview.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

namespace KPlato
{

// little helper stolen from kmail/kword
static inline QPixmap loadIcon( const char * name ) {
  return KIconLoader::global()->loadIcon( QString::fromLatin1(name), KIconLoader::NoGroup, KIconLoader::SizeMedium );
}


ViewListDialog::ViewListDialog( View *view, ViewListWidget &viewlist, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18nc( "@title:window", "Add View") );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new AddViewPanel( view, viewlist, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );
    connect( m_panel, SIGNAL( viewCreated( ViewBase* ) ), SIGNAL( viewCreated( ViewBase* ) ) );

    connect(&viewlist, SIGNAL(viewListItemRemoved(ViewListItem*)), SLOT(slotViewListItemRemoved(ViewListItem*)));
}

void ViewListDialog::slotViewListItemRemoved( ViewListItem * )
{
    reject();
}

void ViewListDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

//------------------------
AddViewPanel::AddViewPanel( View *view, ViewListWidget &viewlist, QWidget *parent )
    : QWidget( parent ),
      m_view( view ),
      m_viewlist( viewlist ),
      m_viewnameChanged( false ),
      m_viewtipChanged( false )
{
    widget.setupUi( this );
    
    // NOTE: these lists must match switch in ok() FIXME: refactor
    m_viewtypes << "ResourceEditor"
            << "TaskEditor"
            << "CalendarEditor"
            << "AccountsEditor"
            << "DependencyEditor"
            << "PertEditor"
            << "ScheduleHandlerView"
            << "TaskStatusView"
            << "TaskView"
            << "TaskWorkPackageView"
            << "GanttView"
            << "MilestoneGanttView"
            << "ResourceAppointmentsView"
            << "ResourceAppointmentsGanttView"
            << "AccountsView"
            << "ProjectStatusView"
            << "PerformanceStatusView";
    /* Deactivate for koffice 2.0
            << "Tasks by Resources";
    */
    QStringList lst;
    lst << i18n( "Resource Editor" )
            << i18n( "Task Editor" )
            << i18n( "Work & Vacation Editor" )
            << i18n( "Accounts Editor" )
            << i18n( "Dependency Editor (Graphic)" )
            << i18n( "Dependency Editor (List)" )
            << i18n( "Schedule Handler" )
            << i18n( "Task Status" )
            << i18n( "Task View" )
            << i18n( "Work Package View" )
            << i18n( "Gantt View" )
            << i18n( "Milestone Gantt View" )
            << i18n( "Resource Assignments" )
            << i18n( "Resource Assignments (Gantt)" )
            << i18n( "Cost Breakdown" )
            << i18n( "Project Performance Chart" )
            << i18n( "Tasks Performance Chart" );
    /* Deactivate for koffice 2.0
            << i18n( "Tasks by Resources" );
    */
    widget.viewtype->addItems( lst );
    
    foreach ( ViewListItem *item, m_viewlist.categories() ) {
        m_categories.insert( item->text( 0 ), item );
    }
    widget.category->addItems( m_categories.keys() );
    ViewListItem *curr = m_viewlist.currentCategory();
    if ( curr ) {
        widget.category->setCurrentIndex( m_categories.values().indexOf( curr ) );
    }
    fillAfter( m_categories.value( widget.category->currentText() ) );

    viewtypeChanged( widget.viewtype->currentIndex() );

    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( viewnameChanged( const QString& ) ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( viewtipChanged( const QString& ) ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
    connect( widget.viewtype, SIGNAL( currentIndexChanged( int ) ), SLOT( viewtypeChanged( int ) ) );
    connect( widget.category, SIGNAL( editTextChanged( const QString& ) ), SLOT( categoryChanged() ) );
}

void AddViewPanel::viewnameChanged( const QString &text )
{
    m_viewnameChanged = ! text.isEmpty();
    qDebug()<<"viewnameChanged:"<<m_viewnameChanged;
}

void AddViewPanel::viewtipChanged( const QString &text )
{
    m_viewtipChanged = ! text.isEmpty();
    qDebug()<<"viewtipChanged:"<<m_viewtipChanged;
}

void AddViewPanel::viewtypeChanged( int idx )
{
    ViewInfo vi = m_view->defaultViewInfo( m_viewtypes.value( idx ) );
    qDebug()<<"viewtypeChanged:"<<idx<<m_viewtypes.value( idx )<<vi.name<<vi.tip;
    if ( widget.viewname->text().isEmpty() ) {
        m_viewnameChanged = false;
    }
    if ( ! m_viewnameChanged ) {
        widget.viewname->setText( vi.name );
        m_viewnameChanged = false;
    }
    if ( widget.tooltip->text().isEmpty() ) {
        m_viewtipChanged = false;
    }
    if ( ! m_viewtipChanged ) {
        widget.tooltip->setText( vi.tip );
        m_viewtipChanged = false;
    }
}

void AddViewPanel::categoryChanged()
{
    kDebug()<<widget.category->currentText();
    fillAfter( m_categories.value( widget.category->currentText() ) );
    changed();
}

void AddViewPanel::fillAfter( ViewListItem *cat )
{
    kDebug()<<cat;
    widget.insertAfter->clear();
    if ( cat ) {
        widget.insertAfter->addItem( i18n( "Top" ) );
        int idx = 0;
        for ( int i = 0; i < cat->childCount(); ++i ) {
            ViewListItem *itm = static_cast<ViewListItem*>( cat->child( i ) );
            widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
        }
        if ( cat == m_viewlist.currentCategory() ) {
            ViewListItem *v = m_viewlist.currentItem();
            if ( v && v->type() != ViewListItem::ItemType_Category ) {
                widget.insertAfter->setCurrentIndex( cat->indexOfChild( v ) + 1 );
            }
        }
    }
}

bool AddViewPanel::ok()
{
    QString n = widget.category->currentText();
    ViewListItem *curr = m_categories.value( n );
    QString c = curr == 0 ? n : curr->tag();
    
    ViewListItem *cat = m_viewlist.addCategory( c, n );
    if ( cat == 0 ) {
        return false;
    }
    ViewBase *v = 0;
    int index = widget.insertAfter->currentIndex();
    int viewtype = widget.viewtype->currentIndex();
    switch ( viewtype ) {
        case 0: { // Resource editor
            v = m_view->createResourceEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 1: { // Task editor
            v = m_view->createTaskEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 2: { // Work & Vacation Editor
            v = m_view->createCalendarEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 3: { // Accounts Editor
            v = m_view->createAccountsEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 4: { // Dependency Editor (Graphic)
            v = m_view->createDependencyEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 5: { // Dependency Editor (List)
            v = m_view->createPertEditor( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 6: { // Schedules Handler
            v = m_view->createScheduleHandler( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 7: { // Task status
            v = m_view->createTaskStatusView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 8: { // Task status
            v = m_view->createTaskView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 9: { // Task work package
            v = m_view->createTaskWorkPackageView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 10: { // Gantt View
            v = m_view->createGanttView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 11: { // Milestone Gantt View
            v = m_view->createMilestoneGanttView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 12: { // Resource Assignments
            v = m_view->createResourceAppointmentsView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 13: { // Resource Assignments (Gantt)
            v = m_view->createResourceAppointmentsGanttView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 14: { // Cost Breakdown
            v = m_view->createAccountsView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 15: { // Project Performance Chart
            v = m_view->createProjectStatusView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        case 16: { // Task Performance Chart
            v = m_view->createPerformanceStatusView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        default:
            kError()<<"Unknown view type!";
            break;
    }
    emit viewCreated( v );
    return true;
}

void AddViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.viewtype->currentText().isEmpty() | widget.category->currentText().isEmpty();
    emit enableButtonOk( ! disable );
}

//------------------------
ViewListEditViewDialog::ViewListEditViewDialog( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18nc( "@title:window", "Configure View" ) );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new EditViewPanel( viewlist, item, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );

    connect(&viewlist, SIGNAL(viewListItemRemoved(ViewListItem*)), SLOT(slotViewListItemRemoved(ViewListItem*)));
}

void ViewListEditViewDialog::slotViewListItemRemoved( ViewListItem * )
{
    reject();
}

void ViewListEditViewDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

EditViewPanel::EditViewPanel( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : QWidget( parent ),
      m_item( item ),
      m_viewlist( viewlist )
{
    widget.setupUi( this );
    
    widget.viewname->setText( item->text( 0 ) );
    widget.tooltip->setText( item->toolTip( 0 ) );
    
    QList<ViewListItem*> lst = m_viewlist.categories();
    if ( ! lst.isEmpty() ) {
        int idx = 0;
        for ( int i = 0; i < lst.count(); ++i ) {
            ViewListItem *citem = lst[ i ];
            widget.category->addItem( citem->text( 0 ), QVariant::fromValue( (void*)citem ) );
            if ( citem == item->parent() ) {
                idx = i;
                fillAfter( citem );
            }
        }
        widget.category->setCurrentIndex( idx );
    }
    
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
    connect( widget.category, SIGNAL( currentIndexChanged( int ) ), SLOT( categoryChanged( int ) ) );
}

bool EditViewPanel::ok()
{
    ViewListItem *cat = static_cast<ViewListItem*>( widget.category->itemData( widget.category->currentIndex() ).value<void*>() );
    if ( cat == 0 ) {
        return false;
    }
    if ( widget.viewname->text() != m_item->text( 0 ) ) {
        m_item->setText( 0, widget.viewname->text() );
    }
    if ( widget.tooltip->text() != m_item->toolTip( 0 ) ) {
        m_item->setToolTip( 0, widget.tooltip->text() );
    }
    m_viewlist.removeViewListItem( m_item );
    int index = widget.insertAfter->currentIndex();
    m_viewlist.addViewListItem( m_item, cat, index );
    
    return true;
}

void EditViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.category->currentText().isEmpty();
    emit enableButtonOk( ! disable );
}

void EditViewPanel::categoryChanged( int index )
{
    kDebug();
    ViewListItem *cat = static_cast<ViewListItem*>( widget.category->itemData( index ).value<void*>() );
    fillAfter( cat );
    changed();
}

void EditViewPanel::fillAfter( ViewListItem *cat )
{
    kDebug();
    widget.insertAfter->clear();
    if ( cat ) {
        widget.insertAfter->addItem( i18n( "Top" ) );
        int idx = 0;
        for ( int i = 0; i < cat->childCount(); ++i ) {
            ViewListItem *itm = static_cast<ViewListItem*>( cat->child( i ) );
            if ( m_item == itm ) {
                idx = i;
            } else {
                widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
            }
        }
        widget.insertAfter->setCurrentIndex( idx );
    }
}

//------------------------
ViewListEditCategoryDialog::ViewListEditCategoryDialog( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18nc( "@title:window", "Configure Category" ) );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new EditCategoryPanel( viewlist, item, this );

    setMainWidget( m_panel );
    
    enableButtonOk(false);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );

    connect(&viewlist, SIGNAL(viewListItemRemoved(ViewListItem*)), SLOT(slotViewListItemRemoved(ViewListItem*)));
}

void ViewListEditCategoryDialog::slotViewListItemRemoved( ViewListItem * )
{
    reject();
}

void ViewListEditCategoryDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

EditCategoryPanel::EditCategoryPanel( ViewListWidget &viewlist, ViewListItem *item, QWidget *parent )
    : QWidget( parent ),
      m_item( item ),
      m_viewlist( viewlist )
{
    widget.setupUi( this );
    
    widget.viewname->setText( item->text( 0 ) );
    widget.tooltip->setText( item->toolTip( 0 ) );
    
    fillAfter();
    
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
}

bool EditCategoryPanel::ok()
{
    if ( widget.viewname->text() != m_item->text( 0 ) ) {
        m_item->setText( 0, widget.viewname->text() );
    }
    if ( widget.tooltip->text() == m_item->toolTip( 0 ) ) {
        m_item->setToolTip( 0, widget.tooltip->text() );
    }
    m_viewlist.removeViewListItem( m_item );
    int index = widget.insertAfter->currentIndex();
    m_viewlist.addViewListItem( m_item, 0, index );
    
    return true;
}

void EditCategoryPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty();
    emit enableButtonOk( ! disable );
}

void EditCategoryPanel::fillAfter()
{
    kDebug();
    widget.insertAfter->clear();
    widget.insertAfter->addItem( i18n( "Top" ) );
    int idx = 0;
    QList<ViewListItem*> lst = m_viewlist.categories();
    for ( int i = 0; i < lst.count(); ++i ) {
        ViewListItem *itm = lst[ i ];
        if ( m_item == itm ) {
            idx = i;
        } else {
            widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
        }
    }
    widget.insertAfter->setCurrentIndex( idx );
}

//------ Reports
ViewListReportsDialog::ViewListReportsDialog( View *view, ViewListWidget &viewlist, QWidget *parent )
    : KDialog(parent)
{
    setCaption( i18nc( "@title:window", "Add Report" ) );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( Ok );

    m_panel = new AddReportsViewPanel( view, viewlist, this );

    setMainWidget( m_panel );
    
    enableButtonOk(true);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect( m_panel, SIGNAL( enableButtonOk( bool ) ), SLOT( enableButtonOk( bool ) ) );
    connect( m_panel, SIGNAL( viewCreated( ViewBase* ) ), SIGNAL( viewCreated( ViewBase* ) ) );

    connect(&viewlist, SIGNAL(viewListItemRemoved(ViewListItem*)), SLOT(slotViewListItemRemoved(ViewListItem*)));
}

void ViewListReportsDialog::slotViewListItemRemoved( ViewListItem * )
{
    reject();
}

void ViewListReportsDialog::slotOk() {
    if ( m_panel->ok() ) {
        accept();
    }
}

//------------------------
AddReportsViewPanel::AddReportsViewPanel( View *view, ViewListWidget &viewlist, QWidget *parent )
    : QWidget( parent ),
      m_view( view ),
      m_viewlist( viewlist ),
      m_viewnameChanged( false ),
      m_viewtipChanged( false )
{
    widget.setupUi( this );
    
    // NOTE: these lists must match switch in ok() FIXME: refactor
    m_viewtypes << "ReportView";
    QStringList lst;
    lst << i18n( "Report" );
    widget.viewtype->addItems( lst );
    
    foreach ( ViewListItem *item, m_viewlist.categories() ) {
        m_categories.insert( item->text( 0 ), item );
    }
    widget.category->addItems( m_categories.keys() );
    ViewListItem *curr = m_viewlist.currentCategory();
    if ( curr ) {
        widget.category->setCurrentIndex( m_categories.values().indexOf( curr ) );
    }
    fillAfter( m_categories.value( widget.category->currentText() ) );

    viewtypeChanged( widget.viewtype->currentIndex() );

    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( changed() ) );
    connect( widget.viewname, SIGNAL( textChanged( const QString& ) ), SLOT( viewnameChanged( const QString& ) ) );
    connect( widget.tooltip, SIGNAL( textChanged( const QString& ) ), SLOT( viewtipChanged( const QString& ) ) );
    connect( widget.insertAfter, SIGNAL( currentIndexChanged( int ) ), SLOT( changed() ) );
    connect( widget.viewtype, SIGNAL( currentIndexChanged( int ) ), SLOT( viewtypeChanged( int ) ) );
    connect( widget.category, SIGNAL( editTextChanged( const QString& ) ), SLOT( categoryChanged() ) );
}

void AddReportsViewPanel::viewnameChanged( const QString &text )
{
    m_viewnameChanged = ! text.isEmpty();
    qDebug()<<"viewnameChanged:"<<m_viewnameChanged;
}

void AddReportsViewPanel::viewtipChanged( const QString &text )
{
    m_viewtipChanged = ! text.isEmpty();
    qDebug()<<"viewtipChanged:"<<m_viewtipChanged;
}

void AddReportsViewPanel::viewtypeChanged( int idx )
{
    ViewInfo vi = m_view->defaultViewInfo( m_viewtypes.value( idx ) );
    qDebug()<<"viewtypeChanged:"<<idx<<m_viewtypes.value( idx )<<vi.name<<vi.tip;
    if ( widget.viewname->text().isEmpty() ) {
        m_viewnameChanged = false;
    }
    if ( ! m_viewnameChanged ) {
        widget.viewname->setText( vi.name );
        m_viewnameChanged = false;
    }
    if ( widget.tooltip->text().isEmpty() ) {
        m_viewtipChanged = false;
    }
    if ( ! m_viewtipChanged ) {
        widget.tooltip->setText( vi.tip );
        m_viewtipChanged = false;
    }
}

void AddReportsViewPanel::categoryChanged()
{
    kDebug()<<widget.category->currentText();
    fillAfter( m_categories.value( widget.category->currentText() ) );
    changed();
}

void AddReportsViewPanel::fillAfter( ViewListItem *cat )
{
    kDebug()<<cat;
    widget.insertAfter->clear();
    if ( cat ) {
        widget.insertAfter->addItem( i18n( "Top" ) );
        int idx = 0;
        for ( int i = 0; i < cat->childCount(); ++i ) {
            ViewListItem *itm = static_cast<ViewListItem*>( cat->child( i ) );
            widget.insertAfter->addItem( itm->text( 0 ), QVariant::fromValue( (void*)itm ) );
        }
        if ( cat == m_viewlist.currentCategory() ) {
            ViewListItem *v = m_viewlist.currentItem();
            if ( v && v->type() != ViewListItem::ItemType_Category ) {
                widget.insertAfter->setCurrentIndex( cat->indexOfChild( v ) + 1 );
            }
        }
    }
}

bool AddReportsViewPanel::ok()
{
    QString n = widget.category->currentText();
    ViewListItem *curr = m_categories.value( n );
    QString c = curr == 0 ? n : curr->tag();
    
    ViewListItem *cat = m_viewlist.addCategory( c, n );
    if ( cat == 0 ) {
        return false;
    }
    ViewBase *v = 0;
    int index = widget.insertAfter->currentIndex();
    int viewtype = widget.viewtype->currentIndex();
    switch ( viewtype ) {
        case 0: { // Report view
            v = m_view->createReportView( cat, m_viewtypes.value( viewtype ), widget.viewname->text(), widget.tooltip->text(), index );
            break; }
        default:
            kError()<<"Unknown view type!";
            break;
    }
    emit viewCreated( v );
    return true;
}

void AddReportsViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.viewtype->currentText().isEmpty() | widget.category->currentText().isEmpty();
    emit enableButtonOk( ! disable );
}



}  //KPlato namespace

#include "kptviewlistdialog.moc"
