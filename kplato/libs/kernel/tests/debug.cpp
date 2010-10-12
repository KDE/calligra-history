/* This file is part of the KDE project
   Copyright (C) 2009 Dag Andersen <danders@get2net.dk>

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

#include "kptappointment.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptproject.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"

#include <kdebug.h>

#include <QStringList>
#include <QString>

namespace KPlato
{

class Debug
{
    public:
        Debug() {}
static
void print( Calendar *c, const QString &str, bool full = true ) {
    Q_UNUSED(full);
    qDebug()<<"Debug info: Calendar"<<c->name()<<str;
    for ( int wd = 1; wd <= 7; ++wd ) {
        CalendarDay *d = c->weekday( wd );
        qDebug()<<"   "<<wd<<":"<<d->stateToString( d->state() );
        foreach ( TimeInterval *t,  d->timeIntervals() ) {
            qDebug()<<"      interval:"<<t->first<<t->second<<"("<<Duration( qint64(t->second) ).toString()<<")";
        }
    }
    foreach ( const CalendarDay *d, c->days() ) {
        qDebug()<<"   "<<d->date()<<":";
        foreach ( TimeInterval *t,  d->timeIntervals() ) {
            qDebug()<<"      interval:"<<t->first<<t->second;
        }
    }
}
static
QString printAvailable( Resource *r, const QString &lead = QString() ) {
    QStringList sl;
    sl<<lead<<"Available:"
        <<( r->availableFrom().isValid()
                ? r->availableFrom().toString()
                : ( r->project() ? ("("+r->project()->constraintStartTime().toString()+")" ) : QString() ) )
        <<( r->availableUntil().isValid()
                ? r->availableUntil().toString()
                : ( r->project() ? ("("+r->project()->constraintEndTime().toString()+")" ) : QString() ) )
        <<QString::number( r->units() )<<"%";
    return sl.join( " " );
}
static
void print( Resource *r, const QString &str, bool full = true ) {
    qDebug()<<"Debug info: Resource"<<r->name()<<str;
    qDebug()<<"Parent group:"<<( r->parentGroup()
                ? ( r->parentGroup()->name() + " Type: "+ r->parentGroup()->typeToString() )
                : QString( "None" ) );
    qDebug()<<"Available:"
        <<( r->availableFrom().isValid()
                ? r->availableFrom().toString()
                : ( r->project() ? ("("+r->project()->constraintStartTime().toString()+")" ) : QString() ) )
        <<( r->availableUntil().isValid()
                ? r->availableUntil().toString()
                : ( r->project() ? ("("+r->project()->constraintEndTime().toString()+")" ) : QString() ) )
        <<r->units()<<"%";
    qDebug()<<"Type:"<<r->typeToString();
    if ( r->type() == Resource::Type_Team ) {
        qDebug()<<"Team members:"<<r->teamMembers().count();
        foreach ( Resource *tm, r->teamMembers() ) {
            qDebug()<<"   "<<tm->name()<<"Available:"
                    <<( r->availableFrom().isValid()
                            ? r->availableFrom().toString()
                            : ( r->project() ? ("("+r->project()->constraintStartTime().toString()+")" ) : QString() ) )
                    <<( r->availableUntil().isValid()
                            ? r->availableUntil().toString()
                            : ( r->project() ? ("("+r->project()->constraintEndTime().toString()+")" ) : QString() ) )
                    <<r->units()<<"%";
        }
    } else {
        Calendar *cal = r->calendar( true );
        QString s;
        if ( cal ) {
            s = cal->name();
        } else {
            cal = r->calendar( false );
            if ( cal ) {
                s = cal->name() + " (Default)";
            } else {
                s = "No calendar";
            }
        }
        qDebug()<<"Calendar:"<<s;
        if ( cal ) {
            print( cal, "Resource calendar" );
        }
    }
    if ( ! full ) return;
    qDebug()<<"External appointments:"<<r->numExternalAppointments();
    foreach ( Appointment *a, r->externalAppointmentList() ) {
        qDebug()<<"   appointment:"<<a->startTime().toString()<<a->endTime().toString();
    }
}
static
void print( Project *p, const QString &str, bool all = false ) {
    qDebug()<<"Debug info: Project"<<p->name()<<str;
    qDebug()<<"project target start:"<<p->constraintStartTime().toString();
    qDebug()<<"  project target end:"<<p->constraintEndTime().toString();
    qDebug()<<"  project start time:"<<p->startTime().toString();
    qDebug()<<"    project end time:"<<p->endTime().toString();
    
    if ( ! all ) {
        return;
    }
    foreach ( Task *t, p->allTasks() ) {
        qDebug();
        print( t );
    }
}
static
void print( Project *p, Task *t, const QString &str, bool full = true ) {
    Q_UNUSED(full);
    print( p, str );
    print( t );
}
static
void print( Task *t, const QString &str, bool full = true ) {
    qDebug()<<"Debug info: Task"<<t->name()<<str;
    print( t, full );
}
static
void print( Task *t, bool full = true ) {
    Q_UNUSED(full);
    qDebug()<<"Task"<<t->name()<<t->typeToString()<<t->constraintToString();
    qDebug()<<"     earlyStart:"<<t->earlyStart().toString();
    qDebug()<<"      lateStart:"<<t->lateStart().toString();
    qDebug()<<"    earlyFinish:"<<t->earlyFinish().toString();
    qDebug()<<"     lateFinish:"<<t->lateFinish().toString();
    qDebug()<<"      startTime:"<<t->startTime().toString();
    qDebug()<<"        endTime:"<<t->endTime().toString();
    switch ( t->constraint() ) {
        case Node::MustStartOn:
        case Node::StartNotEarlier:
            qDebug()<<"startConstraint:"<<t->constraintStartTime().toString();
            break;
        case Node::FixedInterval:
            qDebug()<<"startConstraint:"<<t->constraintStartTime().toString();
        case Node::MustFinishOn:
        case Node::FinishNotLater:
            qDebug()<<"  endConstraint:"<<t->constraintEndTime().toString();
            break;
        default: break;
    }
    foreach ( ResourceGroupRequest *gr, t->requests().requests() ) {
        qDebug()<<"Group request:"<<gr->group()->name()<<gr->units();
        foreach ( ResourceRequest *rr, gr->resourceRequests() ) {
            qDebug()<<printAvailable( rr->resource(), "   " + rr->resource()->name() );
        }
    }
    Schedule *s = t->currentSchedule();
    if ( s == 0 ) {
        return;
    }
    qDebug();
    qDebug()<<"Estimate   :"<<t->estimate()->expectedEstimate()<<Duration::unitToString(t->estimate()->unit())
			<<t->estimate()->typeToString()<<(t->estimate()->calendar()?t->estimate()->calendar()->name():"Fixed");
    foreach ( Appointment *a, s->appointments() ) {
        qDebug()<<"Resource:"<<a->resource()->resource()->name()<<a->startTime().toString()<<a->endTime().toString();
        if ( ! full ) { continue; }
        foreach( const AppointmentInterval &i, a->intervals() ) {
            qDebug()<<"  "<<i.startTime().toString()<<i.endTime().toString()<<i.load();
        }
    }
    if ( t->runningAccount() ) {
        qDebug()<<"Running account :"<<t->runningAccount()->name();
    }
    if ( t->startupAccount() ) {
        qDebug()<<"Startup account :"<<t->startupAccount()->name()<<" cost:"<<t->startupCost();
    }
    if ( t->shutdownAccount() ) {
        qDebug()<<"Shutdown account:"<<t->shutdownAccount()->name()<<" cost:"<<t->shutdownCost();
    }
}
static
void printSchedulingLog( const ScheduleManager &sm, const QString &s )
{
    qDebug()<<"Scheduling log"<<s;
    qDebug()<<"Scheduling:"<<sm.name()<<(sm.recalculate()?QString("recalculate from %1").arg(sm.recalculateFrom().toString()):"");
    foreach ( const QString &s, sm.expected()->logMessages() ) {
        qDebug()<<s;
    }
}
static
void print( Account *a, long id = -1, const QString &s = QString() )
{
    qDebug()<<"Debug info Account:"<<a->name()<<s;
    qDebug()<<"Account:"<<a->name()<<(a->isDefaultAccount() ? "Default" : "");
    EffortCostMap ec = a->plannedCost( id );
    qDebug()<<"Planned cost:"<<ec.totalCost()<<"effort:"<<ec.totalEffort().toString();
    if ( ! a->isElement() ) {
        foreach ( Account *c, a->accountList() ) {
            print( c );
        }
        return;
    }
    qDebug()<<"Cost places:";
    foreach ( Account::CostPlace *cp, a->costPlaces() ) {
        qDebug()<<"     Node:"<<(cp->node() ? cp->node()->name() : "");
        qDebug()<<"  running:"<<cp->running();
        qDebug()<<"  startup:"<<cp->startup();
        qDebug()<<" shutdown:"<<cp->shutdown();
    }
}

static
void print( const AppointmentInterval &i, const QString &indent = QString() )
{
    QString s = indent + "Interval:";
    if ( ! i.isValid() ) {
        qDebug()<<s<<"Not valid";
    } else {
        qDebug()<<s<<i.startTime().toString()<<i.endTime().toString()<<i.load()<<"%";
    }
}

static
void print( const AppointmentIntervalList &lst, const QString &s = QString() )
{
    qDebug()<<"Interval list:"<<lst.count()<<s;
    foreach ( const AppointmentInterval &i, lst ) {
        print( i, "  " );
    }
}

};

}
