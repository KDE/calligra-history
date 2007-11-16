/* This file is part of the KDE project
  Copyright (C) 2005 - 2006 Dag Andersen kplato@kde.org>

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

#include "kptaccountsview.h"

#include "kptaccountsviewconfigdialog.h"
#include "kptdatetime.h"
#include "kptproject.h"
#include "kpteffortcostmap.h"

#include <KoDocument.h>

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <qpalette.h>
#include <QPushButton>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>

#include <kcalendarsystem.h>
#include <kglobal.h>
#include <klocale.h>

#include <kdebug.h>

namespace KPlato
{

AccountsView::AccountItem::AccountItem( Account *a, QTreeWidget *parent, bool highlight )
        : DoubleListViewBase::MasterListItem( parent, a->name(), highlight ),
        account( a )
{
    if ( parent->columnCount() >= 3 ) {
        setText( 2, a->description() );
    }
    //kDebug();
}
AccountsView::AccountItem::AccountItem( Account *a, QTreeWidgetItem *p, bool highlight )
        : DoubleListViewBase::MasterListItem( p, a->name(), highlight ),
        account( a )
{
    if ( treeWidget() && columnCount() >= 3 ) {
        setText( 2, a->description() );
    }
    //kDebug();
}

AccountsView::AccountItem::AccountItem( const QString& text, Account *a, QTreeWidgetItem *parent, bool highlight )
        : DoubleListViewBase::MasterListItem( parent, text, highlight ),
        account( a )
{
    //kDebug();
}

void AccountsView::AccountItem::add
    ( int col, const QDate &date, const EffortCost &ec )
{
    EffortCost & cm = costMap.add( date, ec );
    if ( m_slaveItem )
        m_slaveItem->setText( col, KGlobal::locale() ->formatMoney( cm.cost(), "", 0 ) );
}

AccountsView::AccountsView( Project *project, KoDocument *part, QWidget *parent )
        : ViewBase( part, parent ),
        m_project(0),
        m_manager( 0 )
{
    m_date = QDate::currentDate();
    m_period = 0;
    m_periodTexts << i18n( "Day" ) << i18n( "Week" ) << i18n( "Month" );
    m_cumulative = false;
    m_project=project;
    QVBoxLayout *lay1 = new QVBoxLayout( this, 0, KDialog::spacingHint() );

    QHBoxLayout *lay2 = new QHBoxLayout( 0, 0, KDialog::spacingHint() );
    m_label = new QLabel( this );
    m_label->setFrameShape( QLabel::StyledPanel );
    m_label->setFrameShadow( QLabel::Sunken );
    m_label->setAlignment( int( Qt::TextWordWrap | Qt::AlignVCenter ) );
    lay2->addWidget( m_label );
    m_changeBtn = new QPushButton( i18n( "Configure..." ), this );
    m_changeBtn->setSizePolicy( QSizePolicy( ( QSizePolicy::SizeType ) 0, ( QSizePolicy::SizeType ) 0, 0, 0, m_changeBtn->sizePolicy().hasHeightForWidth() ) );
    lay2->addWidget( m_changeBtn );
    lay1->addLayout( lay2 );

    m_dlv = new DoubleListViewBase( this, true );
    m_dlv->setNameHeader( i18n( "Account" ) );

    setProject( project );
    init();

    lay1->addWidget( m_dlv );

    connect( this, SIGNAL( configChanged() ), SLOT( slotUpdate() ) );
    connect( m_changeBtn, SIGNAL( clicked() ), SLOT( slotConfigure() ) );

    QList<int> list = m_dlv->sizes();
    int tot = list[ 0 ] + list[ 1 ];
    list[ 0 ] = qMin( 35, tot );
    list[ 1 ] = tot - list[ 0 ];
    //m_dlv->setSizes(list);
}

void AccountsView::setZoom( double zoom )
{
    Q_UNUSED( zoom );
}

void AccountsView::init()
{
    m_date = QDate::currentDate();
    m_period = 0;
    if ( m_project ) {
        initAccList( m_project->accounts().accountList() );
    } else {
        m_dlv->clearLists();
    }
}

void AccountsView::setProject( Project *project )
{
    if ( m_project && project != m_project ) {
        m_dlv->clearLists();
    }
    if ( project && project != m_project ) {
        m_project = project;
        draw();
    }
}

void AccountsView::setScheduleManager( ScheduleManager *sm )
{
    m_manager = sm;
    draw();
}

void AccountsView::draw()
{
    if ( m_project == 0 || m_manager == 0 ) {
        m_dlv->clearLists();
        return;
    }
    //kDebug();
/*    Context::Accountsview context;
    getContextClosedItems( context, m_dlv->masterListView() ->topLevelItem( 0 ) );*/
    initAccList( m_project->accounts().accountList() );
//    setContextClosedItems( context );
    slotUpdate();
}

void AccountsView::initAccList( const AccountList &list )
{
    m_dlv->clearLists();
    AccountListIterator it = list;
    for ( it.toBack(); it.hasPrevious(); it.previous() ) {
        AccountsView::AccountItem * a = new AccountsView::AccountItem( it.peekPrevious(), m_dlv->masterListView() );
        /*        a->setOpen(true);
                a->setExpandable(!it.peekPrevious()->isElement());*/
        initAccSubItems( it.peekPrevious(), a );
    }
    createPeriods();
}

void AccountsView::initAccSubItems( Account *acc, AccountsView::AccountItem *parent )
{
    if ( !acc->accountList().isEmpty() ) {
        /*        AccountsView::AccountItem *a = new AccountsView::AccountItem(i18n("Subaccounts"), acc, parent);
                DoubleListViewBase::SlaveListItem *i = new DoubleListViewBase::SlaveListItem(a, parent->period);
                a->period = i;*/

        initAccList( acc->accountList(), parent );
    }
    //     AccountsView::AccountItem *a = new AccountsView::AccountItem(i18n("Variance"), acc, parent, true);
    //     DoubleListViewBase::SlaveListItem *i = new DoubleListViewBase::SlaveListItem(a, parent->period, true);
    //     a->period = i;
    //
    //     a = new AccountsView::AccountItem(i18n("Actual"), acc, parent);
    //     i = new DoubleListViewBase::SlaveListItem(a, parent->period);
    //     a->period = i;
    //
    //     a = new AccountsView::AccountItem(i18n("Planned"), acc, parent);
    //     i = new DoubleListViewBase::SlaveListItem(a, parent->period);
    //     a->period = i;

}

void AccountsView::initAccList( const AccountList &list, AccountsView::AccountItem *parent )
{
    AccountListIterator it = list;
    for ( it.toBack(); it.hasPrevious(); it.previous() ) {
        AccountsView::AccountItem * a = new AccountsView::AccountItem( it.peekPrevious(), parent );
        /*        a->setOpen(true);
                a->setExpandable(!it.peekPrevious()->isElement());*/
        initAccSubItems( it.peekPrevious(), a );
    }
}

void AccountsView::clearPeriods()
{
    m_dlv->clearSlaveList();
}

void AccountsView::createPeriods()
{
    m_dlv->createSlaveItems();
}

void AccountsView::slotUpdate()
{
    if ( m_project == 0  || m_manager == 0 ) {
        return;
    }
    Accounts &accounts = m_project->accounts();
    long id = m_manager->id();
    //kDebug();
    QApplication::setOverrideCursor( Qt::WaitCursor );
    createPeriods();
    KLocale *locale = KGlobal::locale();
    const KCalendarSystem *cal = locale->calendar();

    QString t;
    if ( m_cumulative ) {
        t += " <b>" + i18n( "Cumulative" ) + "</b>  ";
    }
    t += i18n( "Cut-off date:%1", "<b>" + locale->formatDate( m_date, KLocale::ShortDate ) + "</b>" );
    t += ' ' + i18n( "Periodicity:%1", "<b>" + periodText( m_period ) + "</b>" );
    m_label->setText( t );

    // Add columns for selected period/periods
    QDate start = m_project->startTime( id ).date();
    QDate end = m_date;
    //kDebug()<<start<<" -"<<end;
    QStringList df;
    if ( m_period == 0 ) { //Daily
        for ( QDate dt = start; dt <= end; dt = cal->addDays( dt, 1 ) ) {
            df << locale->formatDate( dt, KLocale::ShortDate );
        }
        m_dlv->setSlaveLabels( df );
        foreach ( QTreeWidgetItem * i, m_dlv->masterItems() ) {
            AccountsView::AccountItem * item = dynamic_cast<AccountsView::AccountItem*>( i );
            if ( !item || !item->account || !item->account->isElement() ) {
                continue;
            }
            item->costMap = accounts.plannedCost( *( item->account ), start, end, id );
            double cost = 0.0;
            int col = 0;
            for ( QDate d = start; d <= end; d = cal->addDays( d, 1 ), ++col ) {
                EffortCost & ec = item->costMap.effortCostOnDate( d );
                cost = ( m_cumulative ? cost + ec.cost() : ec.cost() );
                item->setSlaveItem( col, cost );
                m_cumulative ? item->setTotal( cost ) : item->addToTotal( cost );
            }
        }
        m_dlv->calculate();
        QApplication::restoreOverrideCursor();
        return ;
    }
    if ( m_period == 1 ) { //Weekly
        //TODO make this user controlled
        int weekStartDay = locale->weekStartDay();
        QStringList t;
        QDate dt = start;
        QDate pend = cal->addDays( dt, 7 + weekStartDay - 1 - cal->dayOfWeek( dt ) );
        int c = 0;
        for ( ; pend <= end; ++c ) {
            //kDebug()<<c<<":"<<dt<<"-"<<pend<<" :"<<end;
            int y;
            int w = cal->weekNumber( dt, &y );
            t << i18nc( "<week>-<year>", "%1-%2", w, y );
            dt = pend.addDays( 1 );
            pend = cal->addDays( pend, 7 );
            if ( ( pend.year() == end.year() ) && ( pend.weekNumber() == end.weekNumber() ) ) {
                pend = end;
            }
        }
        m_dlv->setSlaveLabels( t );
        if ( c == 0 ) {
            QApplication::restoreOverrideCursor();
            return ;
        }
        foreach ( QTreeWidgetItem * i, m_dlv->masterItems() ) {
            AccountsView::AccountItem * item = dynamic_cast<AccountsView::AccountItem*>( i );
            if ( !item || !item->account || !item->account->isElement() ) {
                continue;
            }
            item->costMap = accounts.plannedCost( *( item->account ), start, end, id );
            double cost = 0.0;
            QDate d = start;
            QDate pend = cal->addDays( d, 7 + weekStartDay - 1 - cal->dayOfWeek( d ) );
            for ( int col = 0; pend <= end; ++col ) {
                double cst = item->costMap.cost( d, d.daysTo( pend ) + 1 );
                cost = ( m_cumulative ? cost + cst : cst );
                item->setSlaveItem( col, cost );
                m_cumulative ? item->setTotal( cost ) : item->addToTotal( cost );
                d = pend.addDays( 1 ); // 1. next week
                pend = cal->addDays( pend, 7 );
                if ( ( pend.year() == end.year() ) && ( pend.weekNumber() == end.weekNumber() ) ) {
                    pend = end;
                }
            }
        }
        m_dlv->calculate();
        QApplication::restoreOverrideCursor();
        return ;
    }
    if ( m_period == 2 ) { //Monthly
        //TODO make this user controlled
        QStringList m;
        QDate dt = start;
        QDate pend;
        cal->setYMD( pend, dt.year(), dt.month(), dt.daysInMonth() );
        int c = 0;
        for ( ; pend <= end; ++c ) {
            //kDebug()<<c<<":"<<dt<<"-"<<pend<<" :"<<end;
            m << cal->monthName( dt, KCalendarSystem::ShortName ) + QString( " %1" ).arg( dt.year() );
            dt = pend.addDays( 1 ); // 1. next month
            pend = cal->addDays( pend, dt.daysInMonth() );
            if ( ( pend.year() == end.year() ) && ( pend.month() == end.month() ) ) {
                pend = end;
            }
        }
        m_dlv->setSlaveLabels( m );
        if ( c == 0 ) {
            QApplication::restoreOverrideCursor();
            return ;
        }
        foreach ( QTreeWidgetItem * i, m_dlv->masterItems() ) {
            AccountsView::AccountItem * item = dynamic_cast<AccountsView::AccountItem*>( i );
            if ( !item || !item->account || !item->account->isElement() ) {
                continue;
            }
            item->costMap = accounts.plannedCost( *( item->account ), start, end, id );
            double cost = 0.0;
            QDate d = start;
            cal->setYMD( pend, d.year(), d.month(), d.daysInMonth() );
            for ( int col = 0; pend <= end; ++col ) {
                double cst = item->costMap.cost( d, d.daysTo( pend ) + 1 );
                cost = ( m_cumulative ? cost + cst : cst );
                item->setSlaveItem( col, cost );
                m_cumulative ? item->setTotal( cost ) : item->addToTotal( cost );
                d = pend.addDays( 1 ); // 1. next month
                pend = cal->addDays( pend, d.daysInMonth() );
                if ( ( pend.year() == end.year() ) && ( pend.month() == end.month() ) ) {
                    pend = end;
                }
            }
        }
        m_dlv->calculate();
        QApplication::restoreOverrideCursor();
        return ;
    }
    QApplication::restoreOverrideCursor();
}

void AccountsView::print( QPrinter &printer, QPrintDialog &printDialog )
{
    //kDebug();
    uint top, left, bottom, right;
    printer.margins( &top, &left, &bottom, &right );
    //kDebug()<<m.width()<<"x"<<m.height()<<" :"<<top<<","<<left<<","<<bottom<<","<<right<<" :"<<size();
    QPainter p;
    p.begin( &printer );
    p.setViewport( left, top, printer.width() - left - right, printer.height() - top - bottom );
    p.setClipRect( left, top, printer.width() - left - right, printer.height() - top - bottom );
    QRect preg = p.clipRegion().boundingRect();
    //kDebug()<<"p="<<preg;
    //p.drawRect(preg.x(), preg.y(), preg.width()-1, preg.height()-1);
    double scale = qMin( ( double ) preg.width() / ( double ) size().width(), ( double ) preg.height() / ( double ) ( size().height() ) );
    //kDebug()<<"scale="<<scale;
    if ( scale < 1.0 ) {
        p.scale( scale, scale );
    }
    QPixmap labelPixmap = QPixmap::grabWidget( m_label );
    p.drawPixmap( m_label->pos(), labelPixmap );
    p.translate( 0, m_label->size().height() );
    m_dlv->paintContents( &p );
    p.end();
}

bool AccountsView::loadContext( const KoXmlElement &context )
{
    //kDebug()<<"---->"<<endl;
/*    const Context::Accountsview &context = c.accountsview;
    
    QList<int> list;
    list << context.accountsviewsize << context.periodviewsize;
    //m_dlv->setSizes(list); //NOTE: Doesn't always work!
    m_date = context.date;
    if ( !m_date.isValid() )
        m_date = QDate::currentDate();
    m_period = context.period;
    m_cumulative = context.cumulative;
    setContextClosedItems( context );*/
    //kDebug()<<"<----";
    return true;
}

// void AccountsView::setContextClosedItems( const Context::Accountsview &context )
// {
//     for ( QStringList::ConstIterator it = context.closedItems.begin(); it != context.closedItems.end(); ++it ) {
//                 if (m_accounts.findAccount(*it)) {
//                     QTreeWidgetItemIterator lit(m_dlv->masterListView());
//                     for (; lit.current(); ++lit) {
//                         if (lit.current()->text(0) == (*it)) {
//                             m_dlv->setOpen(lit.current(), false);
//                             break;
//                         }
//                     }
//                 }
//     }
// }

void AccountsView::saveContext( QDomElement *context ) const
{
    //kDebug()<<endl;
/*    Context::Accountsview &context = c.accountsview;
    
    context.accountsviewsize = m_dlv->sizes() [ 0 ];
    context.periodviewsize = m_dlv->sizes() [ 1 ];
    context.date = m_date;
    context.period = m_period;
    context.cumulative = m_cumulative;*/
    //kDebug()<<"sizes="<<sizes()[0]<<","<<sizes()[1]<<endl;

    //getContextClosedItems( context, m_dlv->masterListView() ->topLevelItem( 0 ) );
}

// void AccountsView::getContextClosedItems( Context::Accountsview &context, QTreeWidgetItem *item ) const
// {
//     if ( item == 0 )
//         return ;
//         for (QTreeWidgetItem *i = item; i; i = i->nextSibling()) {
//             if (!i->isOpen()) {
//                 context.closedItems.append(i->text(0));
//                 kDebug()<<"add closed"<<i->text(0);
//             }
//             getContextClosedItems(context, i->firstChild());
//         }
// }

void AccountsView::slotConfigure()
{
    //kDebug();
    AccountsviewConfigDialog * dia = new AccountsviewConfigDialog( m_date, m_period, m_periodTexts, m_cumulative, this );
    if ( dia->exec() ) {
        m_date = dia->date();
        m_period = dia->period();
        m_cumulative = dia->isCumulative();
        emit configChanged();
    }
    delete dia;
}

QString AccountsView::periodText( int offset )
{
    if ( offset < m_periodTexts.count() )
        return m_periodTexts[ offset ];
    return QString();
}

}  //KPlato namespace

#include "kptaccountsview.moc"
