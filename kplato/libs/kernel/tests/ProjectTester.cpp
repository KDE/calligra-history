/* This file is part of the KDE project
   Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

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
#include "ProjectTester.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"


#include <qtest_kde.h>
#include <kdebug.h>

#include "debug.cpp"

namespace KPlato
{

void ProjectTester::initTestCase()
{
    m_project = new Project();
    // standard worktime defines 8 hour day as default
    QVERIFY( m_project->standardWorktime() );
    QCOMPARE( m_project->standardWorktime()->day(), 8.0 );
    m_calendar = new Calendar();
    m_calendar->setDefault( true );
    QTime t1( 9, 0, 0 );
    QTime t2 ( 17, 0, 0 );
    int length = t1.msecsTo( t2 );
    for ( int i=1; i <= 7; ++i ) {
        CalendarDay *d = m_calendar->weekday( i );
        d->setState( CalendarDay::Working );
        d->addInterval( t1, length );
    }
    m_project->addCalendar( m_calendar );

    m_task = 0;
}

void ProjectTester::cleanupTestCase()
{
    delete m_project;
}

void ProjectTester::testAddTask()
{
    m_task = m_project->createTask( m_project );
    QVERIFY( m_project->addTask( m_task, m_project ) );
    QVERIFY( m_task->parentNode() == m_project );
    QCOMPARE( m_project->findNode( m_task->id() ), m_task );

    m_project->takeTask( m_task );
    delete m_task; m_task = 0;
}

void ProjectTester::testTakeTask()
{
    m_task = m_project->createTask( m_project );
    m_project->addTask( m_task, m_project );
    QCOMPARE( m_project->findNode( m_task->id() ), m_task );

    m_project->takeTask( m_task );
    QVERIFY( m_project->findNode( m_task->id() ) == 0 );

    delete ( m_task ); m_task = 0;
}

void ProjectTester::testTaskAddCmd()
{
    m_task = m_project->createTask( m_project );
    SubtaskAddCmd *cmd = new SubtaskAddCmd( m_project, m_task, m_project );
    cmd->execute();
    QVERIFY( m_task->parentNode() == m_project );
    QCOMPARE( m_project->findNode( m_task->id() ), m_task );
    cmd->unexecute();
    QVERIFY( m_project->findNode( m_task->id() ) == 0 );
    delete cmd;
    m_task = 0;
}

void ProjectTester::testTaskDeleteCmd()
{
    m_task = m_project->createTask( m_project );
    QVERIFY( m_project->addTask( m_task, m_project ) );
    QVERIFY( m_task->parentNode() == m_project );

    NodeDeleteCmd *cmd = new NodeDeleteCmd( m_task );
    cmd->execute();
    QVERIFY( m_project->findNode( m_task->id() ) == 0 );

    cmd->unexecute();
    QCOMPARE( m_project->findNode( m_task->id() ), m_task );

    cmd->execute();
    delete cmd;
    m_task = 0;
}

void ProjectTester::schedule()
{
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays( 1 );
    QDate yesterday = today.addDays( -1 );
    QDate nextweek = today.addDays( 7 );
    QTime t1( 9, 0, 0 );
    QTime t2 ( 17, 0, 0 );
    int length = t1.msecsTo( t2 );

    Task *t = m_project->createTask( m_project );
    t->setName( "T1" );
    m_project->addTask( t, m_project );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Duration );

    ScheduleManager *sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->startTime(), m_project->startTime() );
    QCOMPARE( t->endTime(), DateTime(t->startTime().addDays( 1 )) );

    t->estimate()->setCalendar( m_calendar );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->startTime(), m_calendar->firstAvailableAfter( m_project->startTime(), m_project->endTime() ) );
    QCOMPARE( t->endTime(), DateTime( t->startTime().addMSecs( length ) ) );

    ResourceGroup *g = new ResourceGroup();
    g->setName( "G1" );
    m_project->addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "R1" );
    r->setAvailableFrom( QDateTime( yesterday, QTime() ) );
    r->setCalendar( m_calendar );
    m_project->addResource( g, r );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    ResourceRequest *rr = new ResourceRequest( r, 100 );
    gr->addResourceRequest( rr );
    t->estimate()->setType( Estimate::Type_Effort );

    QString s = "Calculate forward, Task: ASAP -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//     Debug::print( m_project, t, s );
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->startTime() ) );
    QVERIFY( t->lateStart() >=  t->earlyStart() );
    QVERIFY( t->earlyFinish() <= t->endTime() );
    QVERIFY( t->lateFinish() >= t->endTime() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    s = "Calculate forward, Task: ASAP, Resource 50% available -----------------";
    qDebug()<<endl<<"Testing:"<<s;
    r->setUnits( 50 );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    Debug::print( m_project, t, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->startTime() ) );
    QVERIFY( t->lateStart() >=  t->earlyStart() );
    QVERIFY( t->earlyFinish() <= t->endTime() );
    QVERIFY( t->lateFinish() >= t->endTime() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 1, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    s = "Calculate forward, Task: ASAP, Resource 50% available, Request 50% load ---------";
    qDebug()<<endl<<"Testing:"<<s;
    r->setUnits( 50 );
    rr->setUnits( 50 );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//    Debug::print( m_project, t, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->startTime() ) );
    QVERIFY( t->lateStart() >=  t->earlyStart() );
    QVERIFY( t->earlyFinish() <= t->endTime() );
    QVERIFY( t->lateFinish() >= t->endTime() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 3, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    s = "Calculate forward, Task: ASAP, Resource available tomorrow --------";
    qDebug()<<endl<<"Testing:"<<s;
    r->setAvailableFrom( QDateTime( tomorrow, QTime() ) );
    qDebug()<<"Tomorrow:"<<QDateTime( tomorrow, QTime() )<<r->availableFrom();
    r->setUnits( 100 );
    rr->setUnits( 100 );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//     Debug::print( m_project, t, s);
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->startTime() ) );
    QVERIFY( t->lateStart() >=  t->earlyStart() );
    QVERIFY( t->earlyFinish() <= t->endTime() );
    QVERIFY( t->lateFinish() >= t->endTime() );

    QCOMPARE( t->startTime(), DateTime( r->availableFrom().date(), t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    s = "Calculate forward, Task: ALAP -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime(0,0,0) ) );
    t->setConstraint( Node::ALAP );
    r->setAvailableFrom( QDateTime( yesterday, QTime() ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->startTime() ) );
    QVERIFY( t->lateStart() >=  t->earlyStart() );
    QVERIFY( t->earlyFinish() <= t->endTime() );
    QVERIFY( t->lateFinish() >= t->endTime() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    s = "Calculate forward, Task: MustStartOn -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    r->setAvailableFrom( QDateTime( yesterday, QTime() ) );
    t->setConstraint( Node::MustStartOn );
    t->setConstraintStartTime( DateTime( nextweek, t1 ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->startTime(), DateTime( t->constraintStartTime().date(), t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    // Calculate backwards
    s = "Calculate backward, Task: MustStartOn -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( nextweek.addDays( 1 ), QTime() ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    Debug::print( m_project, t, s );
    Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( t->startTime(), DateTime( t->constraintStartTime().date(), t1 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    // Calculate backword
    s = "Calculate backwards, Task: MustFinishOn -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime(0,0,0) ) );
    m_project->setConstraintEndTime( DateTime( nextweek.addDays( 1 ), QTime() ) );
    t->setConstraint( Node::MustFinishOn );
    t->setConstraintEndTime( DateTime( nextweek.addDays( -2 ), t2 ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->endTime(), t->constraintEndTime() );
    QCOMPARE( t->startTime(), t->endTime() - Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: MustFinishOn -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::MustFinishOn );
    t->setConstraintEndTime( DateTime( tomorrow, t2 ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    Debug::print( m_project, t, s );
    Debug::printSchedulingLog( *sm, s );
    QCOMPARE( t->endTime(), t->constraintEndTime() );
    QCOMPARE( t->startTime(), t->endTime() - Duration( 0, 8, 0 ) );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: StartNotEarlier -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::StartNotEarlier );
    t->setConstraintStartTime( DateTime( today, t2 ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//     Debug::print( m_project, t, s );
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->startTime(), DateTime( tomorrow, t1 ));
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 )  );
    QVERIFY( t->schedulingError() == false );

    // Calculate backward
    s = "Calculate backwards, Task: StartNotEarlier -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( nextweek, QTime() ) );
    t->setConstraint( Node::StartNotEarlier );
    t->setConstraintStartTime( DateTime( today, t2 ) );
    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    Debug::print( m_project, t, s );
    Debug::print( m_project->resourceList().first(), s );
    Debug::printSchedulingLog( *sm, s );

    QVERIFY( t->lateStart() >=  t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->endTime() );
    QVERIFY( t->lateFinish() <= m_project->constraintEndTime() );

    QVERIFY( t->endTime() <= t->lateFinish() );
    QCOMPARE( t->startTime(), t->endTime() - Duration( 0, 8, 0 )  );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: FinishNotLater -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::FinishNotLater );
    t->setConstraintEndTime( DateTime( tomorrow.addDays( 1 ), t2 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->startTime() );
    QVERIFY( t->startTime() >= t->earlyStart() );
    QVERIFY( t->startTime() <= t->lateStart() );
    QVERIFY( t->startTime() >= m_project->startTime() );
    QVERIFY( t->endTime() >= t->earlyFinish() );
    QVERIFY( t->endTime() <= t->lateFinish() );
    QVERIFY( t->endTime() <= m_project->endTime() );
    QVERIFY( t->earlyFinish() <= t->constraintEndTime() );
    QVERIFY( t->lateFinish() <= m_project->constraintEndTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate backward
    s = "Calculate backwards, Task: FinishNotLater -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( nextweek, QTime() ) );
    t->setConstraint( Node::FinishNotLater );
    t->setConstraintEndTime( DateTime( tomorrow, t2 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    //Debug::print( m_project, t, s );

    QCOMPARE( t->earlyStart(), m_project->startTime() );
    QCOMPARE( t->lateStart(),  t->startTime() );
    QCOMPARE( t->earlyFinish(), t->constraintEndTime() );
    QVERIFY( t->lateFinish() <= m_project->constraintEndTime() );

    QCOMPARE( t->startTime(), m_project->startTime() );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 )  );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forward, Task: FixedInterval -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::FixedInterval );
    t->setConstraintStartTime( DateTime( tomorrow, t1 ) );
    t->setConstraintEndTime( DateTime( tomorrow, t2 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    //Debug::print( m_project, t, s );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->constraintEndTime() );
    QCOMPARE( t->lateFinish(), t->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->constraintEndTime()  );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: FixedInterval -----------------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::FixedInterval );
    t->setConstraintStartTime( DateTime( tomorrow, QTime() ) ); // outside working hours
    t->setConstraintEndTime( DateTime( tomorrow, t2 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->constraintEndTime() );
    QCOMPARE( t->lateFinish(), t->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->constraintEndTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, ASAP-------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::ASAP );
    t->estimate()->clear();

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    //Debug::print( m_project, t, s );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->earlyStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->earlyStart() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, ASAP-------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::ASAP );
    t->estimate()->clear();

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->earlyStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->earlyStart() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, ALAP-------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::ALAP );
    t->estimate()->clear();

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->earlyStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->earlyStart() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, ALAP-------------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::ALAP );
    t->estimate()->clear();

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->earlyStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->earlyStart() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, MustStartOn ------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::MustStartOn );
    t->setConstraintStartTime( DateTime( tomorrow, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, MustStartOn ------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( tomorrow, QTime() ) );
    t->setConstraint( Node::MustStartOn );
    t->setConstraintStartTime( DateTime( today, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), t->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->earlyStart() );
    QCOMPARE( t->lateFinish(), m_project->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, MustFinishOn ------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::MustFinishOn );
    t->setConstraintEndTime( DateTime( tomorrow, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintEndTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->constraintEndTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->endTime(), t->endTime() );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, MustFinishOn ------------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( tomorrow, QTime() ) );
    t->setConstraint( Node::MustFinishOn );
    t->setConstraintEndTime( DateTime( today, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), t->constraintEndTime() );
    QCOMPARE( t->lateStart(), t->constraintEndTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), m_project->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintEndTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->startTime(), t->startTime() );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, StartNotEarlier ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::StartNotEarlier );
    t->setConstraintEndTime( DateTime( tomorrow, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->endTime(), t->endTime() );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, StartNotEarlier ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( tomorrow, QTime() ) );
    t->setConstraint( Node::StartNotEarlier );
    t->setConstraintStartTime( DateTime( today, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), t->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintStartTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), m_project->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintStartTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->startTime(), t->startTime() );

    // Calculate forward
    s = "Calculate forwards, Task: Milestone, FinishNotLater ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::FinishNotLater );
    t->setConstraintEndTime( DateTime( tomorrow, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), m_project->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->constraintEndTime() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), t->earlyFinish() );

    QCOMPARE( t->startTime(), t->constraintEndTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->endTime(), t->endTime() );

    // Calculate backward
    s = "Calculate backwards, Task: Milestone, FinishNotLater ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintEndTime( DateTime( tomorrow, QTime() ) );
    t->setConstraint( Node::FinishNotLater );
    t->setConstraintEndTime( DateTime( today, t1 ) );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setSchedulingDirection( true );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

    QCOMPARE( t->earlyStart(), t->constraintStartTime() );
    QCOMPARE( t->lateStart(), t->earlyStart() );
    QCOMPARE( t->earlyFinish(), t->lateStart() );
    QCOMPARE( t->lateFinish(), m_project->constraintEndTime() );

    QCOMPARE( t->startTime(), t->constraintEndTime() );
    QCOMPARE( t->endTime(), t->startTime() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( m_project->startTime(), t->startTime() );

    // Calculate forward
    s = "Calculate forward, 2 Tasks, no overbooking ----------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    m_project->setConstraintEndTime( DateTime( today, QTime() ).addDays( 4 ) );
    t->setConstraint( Node::ASAP );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 2.0 );

    Task *tsk2 = m_project->createTask( *t, m_project );
    tsk2->setName( "T2" );
    m_project->addTask( tsk2, m_project );

    gr = new ResourceGroupRequest( g );
    tsk2->addRequest( gr );
    rr = new ResourceRequest( r, 100 );
    gr->addResourceRequest( rr );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setAllowOverbooking( false );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//     Debug::print( m_project, t, s );
//     Debug::print( m_project, tsk2, s );
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->constraintStartTime() ) );
    QCOMPARE( t->lateStart(), tsk2->startTime() );
    QCOMPARE( t->earlyFinish(), DateTime( tomorrow, t2 ) );
    QCOMPARE( t->lateFinish(), t->lateFinish() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->earlyFinish() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( tsk2->earlyStart(), t->earlyStart() );
    QCOMPARE( tsk2->lateStart(), t->earlyFinish() + Duration( 0, 16, 0 ) );
    QCOMPARE( tsk2->earlyFinish(), DateTime( tomorrow, t2 ) );
    QCOMPARE( tsk2->lateFinish(), t->lateFinish() );

    QCOMPARE( tsk2->startTime(), DateTime( tomorrow.addDays( 1 ), t1 ) );
    QCOMPARE( tsk2->endTime(), tsk2->lateFinish() );
    QVERIFY( tsk2->schedulingError() == false );

    QCOMPARE( m_project->endTime(), tsk2->endTime() );

    // Calculate forward
    s = "Calculate forward, 2 Tasks, relation ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    m_project->setConstraintStartTime( DateTime( today, QTime() ) );
    t->setConstraint( Node::ASAP );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 2.0 );

    Relation *rel = new Relation( t, tsk2 );
    bool relationAdded = m_project->addRelation( rel );
    QVERIFY( relationAdded );

    sm = m_project->createScheduleManager( "Test Plan" );
    sm->setAllowOverbooking( true );
    sm->setSchedulingDirection( false );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//    Debug::print( m_project, t, s );
//    Debug::print( m_project, tsk2, s );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->earlyStart(), t->requests().workTimeAfter( m_project->constraintStartTime() ) );
    QCOMPARE( t->lateStart(), DateTime( today, t1 ) );
    QCOMPARE( t->earlyFinish(), DateTime( tomorrow, t2 ) );
    QCOMPARE( t->lateFinish(), t->lateFinish() );

    QCOMPARE( t->startTime(), DateTime( today, t1 ) );
    QCOMPARE( t->endTime(), t->earlyFinish() );
    QVERIFY( t->schedulingError() == false );

    QCOMPARE( tsk2->earlyStart(), tsk2->requests().workTimeAfter( t->earlyFinish() ) );
    QCOMPARE( tsk2->lateStart(), DateTime( tomorrow.addDays( 1 ), t1 ) );
    QCOMPARE( tsk2->earlyFinish(), DateTime( tomorrow.addDays( 2 ), t2 ) );
    QCOMPARE( tsk2->lateFinish(), tsk2->earlyFinish() );

    QCOMPARE( tsk2->startTime(), DateTime( tomorrow.addDays( 1 ), t1 ) );
    QCOMPARE( tsk2->endTime(), tsk2->earlyFinish() );
    QVERIFY( tsk2->schedulingError() == false );

    QCOMPARE( m_project->endTime(), tsk2->endTime() );

}

void ProjectTester::scheduleFullday()
{
    QString s = "Full day, 1 resource works 24 hours a day -------------";
    qDebug()<<endl<<"Testing:"<<s;
    Calendar *c = new Calendar("Test");
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    m_project->addCalendar( c );

    ResourceGroup *g = new ResourceGroup();
    g->setName( "G1" );
    m_project->addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "R1" );
    r->setCalendar( c );
    r->setAvailableFrom( m_project->constraintStartTime() );
    r->setAvailableUntil( r->availableFrom().addDays( 21 ) );
    m_project->addResource( g, r );

    Task *t = m_project->createTask( m_project );
    t->setName( "T1" );
    m_project->addTask( t, m_project );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 3 * 14.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );
    t->addRequest( gr );

    ScheduleManager *sm = m_project->createScheduleManager( "Test Plan" );
    m_project->addScheduleManager( sm );
    sm->createSchedules();
    m_project->calculate( *sm );

//      Debug::print( c, s );
//      Debug::print( m_project, t, s );
//      Debug::print( r, s, true );
//      Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->startTime(), m_project->startTime() );
    QCOMPARE( t->endTime(), DateTime(t->startTime().addDays( 14 )) );

    s = "Full day, 8 hour shifts, 3 resources ---------------";
    qDebug()<<endl<<"Testing:"<<s;
    int hour = 60*60*1000;
    Calendar *c1 = new Calendar("Test 1");
    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c1->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime( 6, 0, 0 ), 8*hour));
    }
    m_project->addCalendar( c1 );
    Calendar *c2 = new Calendar("Test 2");
    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c2->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime( 14, 0, 0 ), 8*hour));
    }
    m_project->addCalendar( c2 );
    Calendar *c3 = new Calendar("Test 3");
    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c3->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime( 0, 0, 0 ), 6*hour));
        wd1->addInterval(TimeInterval(QTime( 22, 0, 0 ), 2*hour));
    }
    m_project->addCalendar( c3 );

    r->setCalendar( c1 );

    r = new Resource();
    r->setName( "R2" );
    r->setCalendar( c2 );
    r->setAvailableFrom( m_project->constraintStartTime() );
    r->setAvailableUntil( r->availableFrom().addDays( 21 ) );
    m_project->addResource( g, r );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );

    r = new Resource();
    r->setName( "R3" );
    r->setCalendar( c3 );
    r->setAvailableFrom( m_project->constraintStartTime() );
    r->setAvailableUntil( r->availableFrom().addDays( 21 ) );
    m_project->addResource( g, r );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );


    sm->createSchedules();
    m_project->calculate( *sm );

//    Debug::print( m_project, t, s );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( t->startTime(), m_project->startTime() );
    QCOMPARE( t->endTime(), DateTime(t->startTime().addDays( 14 )) );
}

void ProjectTester::scheduleWithExternalAppointments()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate::currentDate(), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 3 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar c("Test");
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c.weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    ResourceGroup *g = new ResourceGroup();
    g->setName( "G1" );
    project.addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "R1" );
    r->setCalendar( &c );
    project.addResource( g, r );

    r->addExternalAppointment( "Ext-1", "External project 1", targetstart, targetstart.addDays( 1 ), 100 );
    r->addExternalAppointment( "Ext-1", "External project 1", targetend.addDays( -1 ), targetend, 100 );

    Task *t = project.createTask( &project );
    t->setName( "T1" );
    project.addTask( t, &project );
    t->estimate()->setUnit( Duration::Unit_h );
    t->estimate()->setExpectedEstimate( 8.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );
    t->addRequest( gr );

    ScheduleManager *sm = project.createScheduleManager( "Test Plan" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

    QString s = "Schedule with external appointments ----------";
    qDebug()<<endl<<"Testing:"<<s;

//     Debug::print( r, s );
//     Debug::print( &project, t, s );
//     Debug::printSchedulingLog( *sm );

    QCOMPARE( t->startTime(), targetstart + Duration( 1, 0, 0 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );

    sm->setAllowOverbooking( true );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::printSchedulingLog( *sm );

    QCOMPARE( t->startTime(), targetstart );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );

    sm->setAllowOverbooking( false );
    sm->setSchedulingDirection( true ); // backwards
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::printSchedulingLog( *sm );

    QCOMPARE( t->startTime(), targetend - Duration( 1, 8, 0 ) );
    QCOMPARE( t->endTime(), t->startTime() + Duration( 0, 8, 0 ) );

    sm->setAllowOverbooking( true );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::printSchedulingLog( *sm );

    QCOMPARE( t->startTime(), targetend - Duration( 0, 8, 0 ) );
    QCOMPARE( t->endTime(), targetend  );

    sm->setAllowOverbooking( false );
    r->clearExternalAppointments();
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::printSchedulingLog( *sm );

    QCOMPARE( t->endTime(), targetend );
    QCOMPARE( t->startTime(),  t->endTime() - Duration( 0, 8, 0 )  );

}

void ProjectTester::reschedule()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate::currentDate(), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );
    ResourceGroup *g = new ResourceGroup();
    g->setName( "G1" );
    project.addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "R1" );
    r->setCalendar( c );
    project.addResource( g, r );

    QString s = "Re-schedule; schedule tasks T1, T2, T3 ---------------";
    qDebug()<<endl<<"Testing:"<<s;

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_h );
    task1->estimate()->setExpectedEstimate( 8.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );
    task1->addRequest( gr );

    Task *task2 = project.createTask( &project );
    task2->setName( "T2" );
    project.addTask( task2, &project );
    task2->estimate()->setUnit( Duration::Unit_h );
    task2->estimate()->setExpectedEstimate( 8.0 );
    task2->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );
    task2->addRequest( gr );

    Task *task3 = project.createTask( &project );
    task3->setName( "T3" );
    project.addTask( task3, &project );
    task3->estimate()->setUnit( Duration::Unit_h );
    task3->estimate()->setExpectedEstimate( 8.0 );
    task3->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    gr->addResourceRequest( new ResourceRequest( r, 100 ) );
    task3->addRequest( gr );

    Relation *rel = new Relation( task1, task2 );
    project.addRelation( rel );
    rel = new Relation( task1, task3 );
    project.addRelation( rel );

    ScheduleManager *sm = project.createScheduleManager( "Plan" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( &project, task1, s, true );
//     Debug::print( &project, task2, s, true );
//     Debug::print( &project, task3, s, true );
//     Debug::print( r, s );
//     Debug::printSchedulingLog( *sm );

    QVERIFY( task1->startTime() >= c->firstAvailableAfter( targetstart, targetend ) );
    QVERIFY( task1->startTime() <= c->firstAvailableBefore( targetend, targetstart ) );
    QCOMPARE( task1->endTime(), task1->startTime() + Duration( 0, 8, 0 ) );

    QVERIFY( task2->startTime() >= c->firstAvailableAfter( targetstart, targetend ) );
    QVERIFY( task2->startTime() <= c->firstAvailableBefore( targetend, targetstart ) );
    QCOMPARE( task2->endTime(), task2->startTime() + Duration( 0, 8, 0 ) );

    QVERIFY( task3->startTime() >= c->firstAvailableAfter( targetstart, targetend ) );
    QVERIFY( task3->startTime() <= c->firstAvailableBefore( targetend, targetstart ) );
    QCOMPARE( task3->endTime(), task3->startTime() + Duration( 0, 8, 0 ) );


    DateTime restart = task1->endTime();
    s = QString( "Re-schedule; re-schedule from %1 - tasks T1 (finished), T2, T3 ------" ).arg( restart.toString() );
    qDebug()<<endl<<"Testing:"<<s;

    task1->completion().setStarted( true );
    task1->completion().setPercentFinished( task1->endTime().date(), 100 );
    task1->completion().setFinished( true );

    ScheduleManager *child = project.createScheduleManager( "Plan.1" );
    project.addScheduleManager( child, sm );
    child->setRecalculate( true );
    child->setRecalculateFrom( restart );
    child->createSchedules();
    project.calculate( *child );

//     Debug::print( &project, task1, s, true );
//     Debug::print( &project, task2, s, true );
//     Debug::print( &project, task3, s, true );
//     Debug::printSchedulingLog( *child, s );

    QCOMPARE( task1->startTime(), c->firstAvailableAfter( targetstart, targetend ) );
    QCOMPARE( task1->endTime(), task1->startTime() + Duration( 0, 8, 0 ) );

    // either task2 or task3 may be scheduled first
    if ( task2->startTime() < task3->startTime() ) {
        QCOMPARE( task2->startTime(), c->firstAvailableAfter( qMax(task1->endTime(), restart ), targetend ) );
        QCOMPARE( task2->endTime(), task2->startTime() + Duration( 0, 8, 0 ) );

        QCOMPARE( task3->startTime(), c->firstAvailableAfter( task2->endTime(), targetend ) );
        QCOMPARE( task3->endTime(), task3->startTime() + Duration( 0, 8, 0 ) );
    } else {
        QCOMPARE( task3->startTime(), c->firstAvailableAfter( qMax(task1->endTime(), restart ), targetend ) );
        QCOMPARE( task3->endTime(), task3->startTime() + Duration( 0, 8, 0 ) );

        QCOMPARE( task2->startTime(), c->firstAvailableAfter( task3->endTime(), targetend ) );
        QCOMPARE( task2->endTime(), task2->startTime() + Duration( 0, 8, 0 ) );
    }
}

void ProjectTester::materialResource()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate::currentDate(), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_h );
    task1->estimate()->setExpectedEstimate( 8.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    QString s = "Calculate forward, Task: ASAP, Working + material resource --------";
    qDebug()<<endl<<"Testing:"<<s;
    qDebug()<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "Work" );
    r->setAvailableFrom( targetstart );
    r->setCalendar( c );
    project.addResource( g, r );

    ResourceGroup *mg = new ResourceGroup();
    mg->setType( ResourceGroup::Type_Material );
    project.addResourceGroup( mg );
    Resource *mr = new Resource();
    mr->setType( Resource::Type_Material );
    mr->setName( "Material" );
    mr->setAvailableFrom( targetstart );
    mr->setCalendar( c );
    project.addResource( mg, mr );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    ResourceRequest *rr = new ResourceRequest( r, 100 );
    gr->addResourceRequest( rr );

    ResourceGroupRequest *mgr = new ResourceGroupRequest( mg );
    task1->addRequest( mgr );
    ResourceRequest *mrr = new ResourceRequest( mr, 100 );
    mgr->addResourceRequest( mrr );

    ScheduleManager *sm = project.createScheduleManager( "Test Plan" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r, s);
//     Debug::print( mr, s);
//     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->earlyStart(), task1->requests().workTimeAfter( targetstart ) );
    QVERIFY( task1->lateStart() >=  task1->earlyStart() );
    QVERIFY( task1->earlyFinish() <= task1->endTime() );
    QVERIFY( task1->lateFinish() >= task1->endTime() );

    QCOMPARE( task1->startTime(), DateTime( r->availableFrom().date(), t1 ) );
    QCOMPARE( task1->endTime(), task1->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( task1->schedulingError() == false );
}

void ProjectTester::requiredResource()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate::currentDate(), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_h );
    task1->estimate()->setExpectedEstimate( 8.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    QString s = "Required resource: Working + required material resource --------";
    qDebug()<<endl<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup( g );
    Resource *r = new Resource();
    r->setName( "Work" );
    r->setAvailableFrom( targetstart );
    r->setCalendar( c );
    project.addResource( g, r );

    ResourceGroup *mg = new ResourceGroup();
    mg->setType( ResourceGroup::Type_Material );
    mg->setName( "MG" );
    project.addResourceGroup( mg );
    Resource *mr = new Resource();
    mr->setType( Resource::Type_Material );
    mr->setName( "Material" );
    mr->setAvailableFrom( targetstart );
    mr->setCalendar( c );
    project.addResource( mg, mr );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    ResourceRequest *rr = new ResourceRequest( r, 100 );
    gr->addResourceRequest( rr );

    QList<Resource*> lst; lst << mr;
    rr->setRequiredResources( lst );

    ScheduleManager *sm = project.createScheduleManager( "Test Plan" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r, s);
//     Debug::print( mr, s);
//     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->earlyStart(), task1->requests().workTimeAfter( targetstart ) );
    QVERIFY( task1->lateStart() >=  task1->earlyStart() );
    QVERIFY( task1->earlyFinish() <= task1->endTime() );
    QVERIFY( task1->lateFinish() >= task1->endTime() );

    QCOMPARE( task1->startTime(), DateTime( r->availableFrom().date(), t1 ) );
    QCOMPARE( task1->endTime(), task1->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( task1->schedulingError() == false );

    QList<Appointment*> apps = r->appointments( sm->scheduleId() );
    QVERIFY( apps.count() == 1 );
    QCOMPARE( task1->startTime(), apps.first()->startTime() );
    QCOMPARE( task1->endTime(), apps.last()->endTime() );

    apps = mr->appointments( sm->scheduleId() );
    QVERIFY( apps.count() == 1 );
    QCOMPARE( task1->startTime(), apps.first()->startTime() );
    QCOMPARE( task1->endTime(), apps.last()->endTime() );

    s = "Required resource limits availability --------";
    qDebug()<<endl<<"Testing:"<<s;
    DateTime tomorrow = targetstart.addDays( 1 );
    mr->setAvailableFrom( tomorrow );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r, s);
//     Debug::print( mr, s);
//     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->earlyStart(), task1->requests().workTimeAfter( targetstart ) );
    QVERIFY( task1->lateStart() >=  task1->earlyStart() );
    QVERIFY( task1->earlyFinish() <= task1->endTime() );
    QVERIFY( task1->lateFinish() >= task1->endTime() );

    QCOMPARE( task1->startTime(), DateTime( mr->availableFrom().date(), t1 ) );
    QCOMPARE( task1->endTime(), task1->startTime() + Duration( 0, 8, 0 ) );
    QVERIFY( task1->schedulingError() == false );

    apps = r->appointments( sm->scheduleId() );
    QVERIFY( apps.count() == 1 );
    QCOMPARE( task1->startTime(), apps.first()->startTime() );
    QCOMPARE( task1->endTime(), apps.last()->endTime() );

    apps = mr->appointments( sm->scheduleId() );
    QVERIFY( apps.count() == 1 );
    QCOMPARE( task1->startTime(), apps.first()->startTime() );
    QCOMPARE( task1->endTime(), apps.last()->endTime() );
}

void ProjectTester::resourceWithLimitedAvailability()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate( 2010, 5, 1 ), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    DateTime expectedEndTime( QDate( 2010, 5, 3 ), QTime( 16, 0, 0 ) );

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_d );
    task1->estimate()->setExpectedEstimate( 4.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    QString s = "Two resources: One with available until < resulting task length --------";
    qDebug()<<endl<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    r1->setAvailableFrom( targetstart );
    r1->setCalendar( c );
    project.addResource( g, r1 );

    Resource *r2 = new Resource();
    r2->setName( "R2" );
    r2->setAvailableFrom( targetstart );
    r2->setAvailableUntil( targetstart.addDays( 1 ) );
    r2->setCalendar( c );
    project.addResource( g, r2 );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    ResourceRequest *rr1 = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( rr1 );
    ResourceRequest *rr2 = new ResourceRequest( r2, 100 );
    gr->addResourceRequest( rr2 );

    ScheduleManager *sm = project.createScheduleManager( "Test Plan" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r1, s);
//     Debug::print( r2, s);
//     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), expectedEndTime );
}

void ProjectTester::unavailableResource()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate( 2010, 5, 1 ), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_d );
    task1->estimate()->setExpectedEstimate( 2.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    QString s = "One available resource --------";
    qDebug()<<endl<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    r1->setCalendar( c );
    project.addResource( g, r1 );

    Resource *r2 = new Resource();
    r2->setName( "Unavailable" );
    r2->setCalendar( c );
    project.addResource( g, r2 );

    
    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    gr->addResourceRequest( new ResourceRequest( r1, 100 ) );

    ScheduleManager *sm = project.createScheduleManager( "Plan R1" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r1, s);
//     Debug::print( r2, s);
     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    DateTime expectedEndTime = targetstart + Duration( 1, 16, 0 );

    QCOMPARE( task1->endTime(), expectedEndTime );

    s = "One available resource + one unavailable resource --------";
    qDebug()<<endl<<"Testing:"<<s;

    r2->setAvailableFrom( targetend );
    r2->setAvailableUntil( targetend.addDays( 1 ) );
    gr->addResourceRequest( new ResourceRequest( r2, 100 ) );

    sm = project.createScheduleManager( "Team + Resource" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r1, s);
//     Debug::print( r2, s);
     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );
    QCOMPARE( task1->endTime(), expectedEndTime );

}


void ProjectTester::team()
{
    Project project;
    project.setName( "P1" );
    DateTime targetstart = DateTime( QDate( 2010, 5, 1 ), QTime(0,0,0) );
    DateTime targetend = DateTime( targetstart.addDays( 7 ) );
    project.setConstraintStartTime( targetstart );
    project.setConstraintEndTime( targetend);

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar( c );

    Task *task1 = project.createTask( &project );
    task1->setName( "T1" );
    project.addTask( task1, &project );
    task1->estimate()->setUnit( Duration::Unit_d );
    task1->estimate()->setExpectedEstimate( 2.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    QString s = "One team with one resource --------";
    qDebug()<<endl<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    r1->setCalendar( c );
    project.addResource( g, r1 );

    Resource *r2 = new Resource();
    r2->setName( "Team member" );
    r2->setCalendar( c );
    project.addResource( g, r2 );

    Resource *team = new Resource();
    team->setType( Resource::Type_Team );
    team->setName( "Team" );
    team->addTeamMember( r2 );
    project.addResource( g, team );
    
    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    ResourceRequest *tr = new ResourceRequest( team, 100 );
    gr->addResourceRequest( tr );

    ScheduleManager *sm = project.createScheduleManager( "Team" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r1, s);
//     Debug::print( r2, s);
     Debug::print( team, s, false);
     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );

    DateTime expectedEndTime = targetstart + Duration( 1, 16, 0 );
    QCOMPARE( task1->endTime(), expectedEndTime );

    s = "One team with one resource + one resource --------";
    qDebug()<<endl<<"Testing:"<<s;

    ResourceRequest *rr1 = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( rr1 );

    sm = project.createScheduleManager( "Team + Resource" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//     Debug::print( r1, s);
//     Debug::print( r2, s);
     Debug::print( team, s, false);
     Debug::print( &project, task1, s);
//     Debug::printSchedulingLog( *sm, s );
    expectedEndTime = targetstart + Duration( 0, 16, 0 );
    QCOMPARE( task1->endTime(), expectedEndTime );

    s = "One team with one resource + one resource, resource available too late --------";
    qDebug()<<endl<<"Testing:"<<s;
    
    r1->setAvailableFrom( targetend );
    r1->setAvailableUntil( targetend.addDays( 7 ) );

    sm = project.createScheduleManager( "Team + Resource not available" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

    Debug::print( r1, s);
//    Debug::print( r2, s);
    Debug::print( team, s, false);
    Debug::print( &project, task1, s);
    Debug::printSchedulingLog( *sm, s );
    expectedEndTime = targetstart + Duration( 1, 16, 0 );
    QCOMPARE( task1->endTime(), expectedEndTime );

    s = "One team with two resources --------";
    qDebug()<<endl<<"Testing:"<<s;
    
    r1->removeRequests();
    team->addTeamMember( r1 );
    r1->setAvailableFrom( targetstart );
    r1->setAvailableUntil( targetend );

    sm = project.createScheduleManager( "Team with 2 resources" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

//    Debug::print( r1, s);
//    Debug::print( r2, s);
    Debug::print( team, s, false);
    Debug::print( &project, task1, s);
    Debug::printSchedulingLog( *sm, s );
    expectedEndTime = targetstart + Duration( 0, 16, 0 );
    QCOMPARE( task1->endTime(), expectedEndTime );

    s = "One team with two resources, one resource unavailable --------";
    qDebug()<<endl<<"Testing:"<<s;
    
    r1->setAvailableFrom( targetend );
    r1->setAvailableUntil( targetend.addDays( 2 ) );

    sm = project.createScheduleManager( "Team, one unavailable resource" );
    project.addScheduleManager( sm );
    sm->createSchedules();
    project.calculate( *sm );

    Debug::print( r1, s);
//    Debug::print( r2, s);
    Debug::print( team, s, false);
    Debug::print( &project, task1, s);
    Debug::printSchedulingLog( *sm, s );
    expectedEndTime = targetstart + Duration( 1, 16, 0 );
    QCOMPARE( task1->endTime(), expectedEndTime );

}

void ProjectTester::inWBSOrder()
{
    Project p;
    p.setName( "WBS Order" );
    DateTime st = p.constraintStartTime();
    st = DateTime( st.addDays( 1 ) );
    st.setTime( QTime ( 0, 0, 0 ) );
    p.setConstraintStartTime( st );
    p.setConstraintEndTime( st.addDays( 5 ) );

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar( c );
    p.setDefaultCalendar( c );
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    p.addResource( g, r1 );

    Task *t = p.createTask( &p );
    t->setName( "T1" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    ResourceRequest *tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T2" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T3" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T4" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );
    
    ScheduleManager *sm = p.createScheduleManager( "WBS Order, forward" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    
    QString s = "Schedule 4 tasks forward in wbs order -------";
    // NOTE: It's not *mandatory* to schedule in wbs order but users expect it, so we'll try
    //       This test can be removed if for some important reason this isn't possible.

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 3, 8, 0 ) );

}

void ProjectTester::resourceConflictALAP()
{
    Project p;
    p.setName( "resourceConflictALAP" );
    DateTime st = p.constraintStartTime();
    st = DateTime( st.addDays( 1 ) );
    st.setTime( QTime ( 0, 0, 0 ) );
    p.setConstraintStartTime( st );
    p.setConstraintEndTime( st.addDays( 5 ) );

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar( c );
    p.setDefaultCalendar( c );
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    p.addResource( g, r1 );

    Task *t = p.createTask( &p );
    t->setName( "T1" );
    t->setConstraint( Node::ALAP );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    ResourceRequest *tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T2" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T3" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T4" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );
    
    ScheduleManager *sm = p.createScheduleManager( "T1 ALAP" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    
    QString s = "Schedule T1 ALAP -------";

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );

    s = "Schedule T1, T2 ALAP -------";
    p.allTasks().at( 1 )->setConstraint( Node::ALAP );
    
    sm = p.createScheduleManager( "T1, T2 ALAP" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );

    s = "Schedule T1, T2, T3 ALAP -------";
    p.allTasks().at( 2 )->setConstraint( Node::ALAP );
    
    sm = p.createScheduleManager( "T1, T2, T3 ALAP" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );

    s = "Schedule T1, T2, T3, T4 ALAP -------";
    p.allTasks().at( 3 )->setConstraint( Node::ALAP );
    
    sm = p.createScheduleManager( "T1, T2, T3, T4 ALAP" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );

}

void ProjectTester::resourceConflictMustStartOn()
{
    Project p;
    p.setName( "resourceConflictMustStartOn" );
    DateTime st = p.constraintStartTime();
    st = DateTime( st.addDays( 1 ) );
    st.setTime( QTime ( 0, 0, 0 ) );
    p.setConstraintStartTime( st );
    p.setConstraintEndTime( st.addDays( 5 ) );

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar( c );
    p.setDefaultCalendar( c );
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    p.addResource( g, r1 );

    Task *t = p.createTask( &p );
    t->setName( "T1" );
    t->setConstraint( Node::MustStartOn );
    t->setConstraintStartTime( st + Duration( 1, 8, 0 ) );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    ResourceRequest *tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T2" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T3" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    t = p.createTask( &p );
    t->setName( "T4" );
    p.addSubTask( t, &p );
    t->estimate()->setUnit( Duration::Unit_d );
    t->estimate()->setExpectedEstimate( 1.0 );
    t->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    t->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );
    
    ScheduleManager *sm = p.createScheduleManager( "T1 MustStartOn" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    
    QString s = "Schedule T1 MustStartOn -------";

//     Debug::print ( c, s );
//     Debug::print( r1, s );
     Debug::print( &p, s, true );
     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );

    s = "Schedule T1, T2 MustStartOn -------";
    p.allTasks().at( 1 )->setConstraint( Node::MustStartOn );
    p.allTasks().at( 1 )->setConstraintStartTime( st + Duration( 2, 8, 0 ) );
    
    sm = p.createScheduleManager( "T1, T2 MustStartOn" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    

//     Debug::print ( c, s );
//     Debug::print( r1, s );
     Debug::print( &p, s, true );
     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );

    s = "Schedule T1, T2, T3 MustStartOn -------";
    p.allTasks().at( 2 )->setConstraint( Node::MustStartOn );
    p.allTasks().at( 2 )->setConstraintStartTime( st + Duration( 3, 8, 0 ) );
    
    sm = p.createScheduleManager( "T1, T2, T3 MustStartOn" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );
    

//     Debug::print ( c, s );
//     Debug::print( r1, s );
//     Debug::print( &p, s, true );
//     Debug::printSchedulingLog( *sm, s );
    
    QCOMPARE( p.allTasks().count(), 4 );
    QCOMPARE( p.allTasks().at( 0 )->startTime(), st + Duration( 1, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 0 )->endTime(), st + Duration( 1, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->startTime(), st + Duration( 2, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 1 )->endTime(), st + Duration( 2, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->startTime(), st + Duration( 3, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 2 )->endTime(), st + Duration( 3, 8, 0 ) + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->startTime(), st + Duration( 0, 8, 0 ) );
    QCOMPARE( p.allTasks().at( 3 )->endTime(), st + Duration( 0, 8, 0 ) + Duration( 0, 8, 0 ) );


    s = "Schedule backwards, T1 MustStartOn, T2 ASAP -------";

    p.takeTask( p.allTasks().at( 3 ), false );
    p.takeTask( p.allTasks().at( 2 ), false );

    Task *task1 = p.allTasks().at( 0 );
    Task *task2 = p.allTasks().at( 1 );
    DateTime et = p.constraintEndTime();
    task1->setConstraint( Node::MustStartOn );
    task1->setConstraintStartTime( et - Duration( 1, 16, 0 ) );
    task2->setConstraint( Node::ASAP );

    sm = p.createScheduleManager( "T1 MustStartOn, T2 ASAP" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    sm->setSchedulingDirection( true );
    p.calculate( *sm );

    Debug::print( &p, s, true );
    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->startTime(), task1->mustStartOn() );
    QCOMPARE( task2->startTime(), et - Duration( 0, 16, 0 ) );

    s = "Schedule backwards, T1 MustStartOn, T2 StartNotEarlier -------";

    task2->setConstraint( Node::StartNotEarlier );
    task2->setConstraintStartTime( task1->mustStartOn().addDays( -1 ) );

    sm = p.createScheduleManager( "T1 MustStartOn, T2 StartNotEarlier" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    sm->setSchedulingDirection( true );
    p.calculate( *sm );

    Debug::print( &p, s, true );
    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->startTime(), task1->mustStartOn() );
    QCOMPARE( task2->startTime(), et - Duration( 0, 16, 0 ) );

    task2->setConstraintStartTime( task1->mustStartOn() );
    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->startTime(), task1->mustStartOn() );
    QCOMPARE( task2->startTime(), et - Duration( 0, 16, 0 ) );

    task2->setConstraintStartTime( task1->mustStartOn().addDays( 1 ) );
    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->startTime(), task1->mustStartOn() );
    QCOMPARE( task2->startTime(), et - Duration( 0, 16, 0 ) );

}

void ProjectTester::resourceConflictMustFinishOn()
{
    Project p;
    p.setName( "P1" );
    DateTime st = p.constraintStartTime();
    st = DateTime( st.addDays( 1 ) );
    st.setTime( QTime ( 0, 0, 0 ) );
    p.setConstraintStartTime( st );
    p.setConstraintEndTime( st.addDays( 5 ) );

    Calendar *c = new Calendar("Test");
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for ( int i = 1; i <= 7; ++i ) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar( c );
    p.setDefaultCalendar( c );
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup( g );
    Resource *r1 = new Resource();
    r1->setName( "R1" );
    p.addResource( g, r1 );

    Task *task1 = p.createTask( &p );
    task1->setName( "T1" );
    task1->setConstraint( Node::MustFinishOn );
    task1->setConstraintEndTime( st + Duration( 1, 16, 0 ) );
    p.addSubTask( task1, &p );
    task1->estimate()->setUnit( Duration::Unit_d );
    task1->estimate()->setExpectedEstimate( 1.0 );
    task1->estimate()->setType( Estimate::Type_Effort );

    ResourceGroupRequest *gr = new ResourceGroupRequest( g );
    task1->addRequest( gr );
    ResourceRequest *tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    Task *task2 = p.createTask( &p );
    task2->setName( "T2" );
    p.addSubTask( task2, &p );
    task2->estimate()->setUnit( Duration::Unit_d );
    task2->estimate()->setExpectedEstimate( 1.0 );
    task2->estimate()->setType( Estimate::Type_Effort );

    gr = new ResourceGroupRequest( g );
    task2->addRequest( gr );
    tr = new ResourceRequest( r1, 100 );
    gr->addResourceRequest( tr );

    QString s = "Schedule T1 MustFinishOn, T2 ASAP -------";
    qDebug()<<s;

    ScheduleManager *sm = p.createScheduleManager( "T1 MustFinishOn" );
    p.addScheduleManager( sm );
    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), p.constraintStartTime() + Duration( 0, 8, 0 ) );

    s = "Schedule T1 MustFinishOn, T2 ALAP -------";
    qDebug()<<s;

    task2->setConstraint( Node::ALAP );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), p.constraintStartTime() + Duration( 0, 8, 0 ) );

    s = "Schedule T1 MustFinishOn, T2 StartNotEarlier -------";
    qDebug()<<s;

    task2->setConstraint( Node::StartNotEarlier );
    task2->setConstraintStartTime( p.constraintStartTime() );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), p.constraintStartTime() + Duration( 0, 8, 0 ) );

    s = "Schedule T1 MustFinishOn, T2 StartNotEarlier -------";
    qDebug()<<s;

    task2->setConstraintStartTime( task1->mustFinishOn() - Duration( 0, 8, 0 ) );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), task2->constraintStartTime() + Duration( 1, 0, 0 ) );

    s = "Schedule T1 MustFinishOn, T2 StartNotEarlier -------";
    qDebug()<<s;

    task2->setConstraintStartTime( task1->mustFinishOn() );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), task2->constraintStartTime() + Duration( 0, 16, 0 ) );

    s = "Schedule backwards, T1 MustFinishOn, T2 ASAP -------";
    qDebug()<<s;

    task2->setConstraint( Node::ASAP );

    sm->createSchedules();
    sm->setSchedulingDirection( true );
    p.calculate( *sm );

    Debug::print( &p, s, true );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->startTime(), task1->mustFinishOn() + Duration( 0, 16, 0 ) );

    s = "Schedule backwards, T1 MustFinishOn, T2 ALAP -------";
    qDebug()<<s;

    DateTime et = p.constraintEndTime();

    task2->setConstraint( Node::ALAP );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->endTime(), et - Duration( 0, 8, 0 ) );

    s = "Schedule backwards, T1 MustFinishOn, T2 StartNotEarlier -------";
    qDebug()<<s;

    task2->setConstraint( Node::StartNotEarlier );
    task2->setConstraintStartTime( task1->mustFinishOn() );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->endTime(), et - Duration( 0, 8, 0 ) );

    task2->setConstraintStartTime( p.constraintStartTime() );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->endTime(), et - Duration( 0, 8, 0 ) );

    s = "Schedule backwards, T1 MustFinishOn, T2 FinishNotLater -------";
    qDebug()<<s;

    task2->setConstraint( Node::FinishNotLater );
    task2->setConstraintEndTime( task1->mustFinishOn() );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->endTime(), task1->startTime() - Duration( 0, 16, 0 ) );

    task2->setConstraintEndTime( task1->mustFinishOn().addDays( 2 ) );

    sm->createSchedules();
    p.calculate( *sm );

    Debug::print( &p, s, true );
//    Debug::printSchedulingLog( *sm, s );

    QCOMPARE( task1->endTime(), task1->mustFinishOn() );
    QCOMPARE( task2->endTime(), task2->finishNotLater() );
}

} //namespace KPlato

QTEST_KDEMAIN_CORE( KPlato::ProjectTester )

#include "ProjectTester.moc"
