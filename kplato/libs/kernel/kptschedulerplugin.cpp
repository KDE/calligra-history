/* This file is part of the KDE project
  Copyright (C) 2009, 2010 Dag Andersen <danders@get2net.dk>

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

#include "kptschedulerplugin.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"

#include "KoXmlReader.h"

#include <QDomDocument>

namespace KPlato
{

class SchedulerPlugin::Private
{
public:
    Private() {}

    QString name;
    QString comment;
};

SchedulerPlugin::SchedulerPlugin(QObject *parent)
    : QObject(parent),
    d( new SchedulerPlugin::Private() )
{
    m_synctimer.setInterval( 500 );
    connect(&m_synctimer, SIGNAL(timeout()), SLOT(slotSyncData()));
}
 
SchedulerPlugin::~SchedulerPlugin()
{
    foreach ( SchedulerThread *s, m_jobs ) {
        s->haltScheduling();
    }
    delete d;
}

void SchedulerPlugin::setName( const QString &name )
{
    d->name = name;
}

QString SchedulerPlugin::name() const
{
    return d->name;
}

void SchedulerPlugin::setComment( const QString &comment )
{
    d->comment = comment;
}

QString SchedulerPlugin::comment() const
{
    return d->comment;
}

void SchedulerPlugin::stopCalculation( ScheduleManager *sm )
{
    foreach ( SchedulerThread *j, m_jobs ) {
        if ( sm == j->mainManager() ) {
            j->stopScheduling();
        }
    }
}

void SchedulerPlugin::haltCalculation( ScheduleManager *sm )
{
    qDebug()<<"SchedulerPlugin::haltCalculation:"<<sm;
    foreach ( SchedulerThread *j, m_jobs ) {
        if ( sm == j->mainManager() ) {
            haltCalculation( j );
            break;
        }
    }
    sm->setCalculationResult( ScheduleManager::CalculationCanceled );
    sm->setScheduling( false );
}

void SchedulerPlugin::stopCalculation( SchedulerThread *job )
{
    job->stopScheduling();
}

void SchedulerPlugin::haltCalculation( SchedulerThread *job )
{
    qDebug()<<"SchedulerPlugin::haltCalculation:"<<job<<m_jobs.contains( job );
    disconnect(this, 0, job, 0 );
    job->haltScheduling();
    if ( m_jobs.contains( job ) ) {
        qDebug()<<"SchedulerPlugin::haltCalculation: remove"<<job;
        m_jobs.removeAt( m_jobs.indexOf( job ) );
    }
}

void SchedulerPlugin::slotSyncData()
{
    updateProgress();
    updateLog();
}

void SchedulerPlugin::updateProgress()
{
    foreach ( SchedulerThread *j, m_jobs ) {
        ScheduleManager *sm = j->mainManager();
        if ( sm->maxProgress() != j->maxProgress() ) {
            sm->setMaxProgress( j->maxProgress() );
        }
        sm->setProgress( j->progress() );
    }
}

void SchedulerPlugin::updateLog()
{
    foreach ( SchedulerThread *j, m_jobs ) {
        updateLog( j );
    }
}

void SchedulerPlugin::updateLog( SchedulerThread *j )
{
    ScheduleManager *sm = j->mainManager();
    Project *p = j->mainProject();
    Q_ASSERT( p == &(sm->project()) );
    QList<Schedule::Log> logs;
    //qDebug()<<"SchedulerPlugin::updateLog:"<<j<<logs.count();
    foreach ( const Schedule::Log log, j->log() ) {
        // map log from temporary project to real project
        Schedule::Log l = log;
        if ( l.resource ) {
            const Resource *r = l.resource;
            l.resource = sm->project().findResource( l.resource->id() );
//            qDebug()<<"SchedulerPlugin::updateLog: mapped"<<r<<l.resource;
            Q_ASSERT( r != l.resource );
            Q_ASSERT( l.resource->project() == p );
        }
        if ( l.node ) {
            const Node *n = l.node;
            if ( l.node->type() == Node::Type_Project ) {
                l.node = &( sm->project() );
            } else {
                l.node = sm->project().findNode( l.node->id() );
            }
//            qDebug()<<"SchedulerPlugin::updateLog: mapped"<<n<<l.node;
            Q_ASSERT( n != l.node );
            Q_ASSERT( l.node->projectNode() == p );
        }
        logs << l;
    }
    sm->slotAddLog( logs );
}

void SchedulerPlugin::updateProject( const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm ) const
{
    Q_CHECK_PTR( tp );
    Q_CHECK_PTR( tm );
    Q_CHECK_PTR( mp );
    Q_CHECK_PTR( sm );
    //qDebug()<<"SchedulerPlugin::updateProject:"<<tp<<tp->name()<<"->"<<mp<<mp->name()<<sm;
    Q_ASSERT( tp != mp && tm != sm );
    long sid = tm->scheduleId();
    Q_ASSERT( sid == sm->scheduleId() );

    XMLLoaderObject status;
    status.setVersion( KPLATO_FILE_SYNTAX_VERSION );
    status.setProject( mp );
    status.setProjectSpec( mp->timeSpec() );

    foreach ( const Node *tn, tp->allNodes() ) {
        Node *mn = mp->findNode( tn->id() );
        Q_ASSERT( mn );
        if ( mn ) {
            updateNode( tn, mn, sid, status );
        }
    }
    // update main schedule and appointments
    updateAppointments( tp, tm, mp, sm, status );
}

void SchedulerPlugin::updateNode( const Node *tn, Node *mn, long sid, XMLLoaderObject &status ) const
{
    //qDebug()<<"SchedulerPlugin::updateNode:"<<tn<<tn->name()<<"->"<<mn<<mn->name();
    NodeSchedule *s = static_cast<NodeSchedule*>( tn->schedule( sid ) );
    if ( s == 0 ) {
        kWarning()<<"SchedulerPlugin::updateNode:"<<"Task:"<<tn->name()<<"could not find schedule with id:"<<sid;
        return;
    }
    QDomDocument doc( "tmp" );
    QDomElement e = doc.createElement( "schedules" );
    doc.appendChild( e );
    s->saveXML( e );

    Q_ASSERT( ! mn->findSchedule( sid ) );
    s = static_cast<NodeSchedule*>( mn->schedule( sid ) );
    Q_ASSERT( s == 0 );
    s = new NodeSchedule();

    KoXmlDocument xd;
    xd.setContent( doc.toString() );
    KoXmlElement se = xd.documentElement().namedItem( "schedule" ).toElement();
    Q_ASSERT( ! se.isNull() );

    s->loadXML( se, status );
    s->setDeleted( false );
    s->setNode( mn );
    mn->addSchedule( s );
}

void SchedulerPlugin::updateAppointments( const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm, XMLLoaderObject &status ) const
{
    MainSchedule *sch = tm->expected();
    Q_ASSERT( sch );
    Q_ASSERT( sch != sm->expected() );
    Q_ASSERT( sch->id() == sm->expected()->id() );

    QDomDocument doc( "tmp" );
    QDomElement e = doc.createElement( "schedule" );
    doc.appendChild( e );
    sch->saveXML( e );
    tp->saveAppointments( e, sch->id() );

    KoXmlDocument xd;
    xd.setContent( doc.toString() );
    KoXmlElement se = xd.namedItem( "schedule" ).toElement();
    Q_ASSERT( ! se.isNull() );

    bool ret = sm->loadMainSchedule( sm->expected(), se, status ); // also loads appointments
    Q_ASSERT( ret );
    mp->setCurrentSchedule( sch->id() );
    sm->expected()->setPhaseNames( sch->phaseNames() );
    mp->changed( sm );
}

//----------------------
SchedulerThread::SchedulerThread( Project *project, ScheduleManager *manager, QObject *parent )
    : QThread( parent ),
    m_mainproject( project ),
    m_mainmanager( manager ),
    m_mainmanagerId( manager->managerId() ),
    m_project( 0 ),
    m_manager( 0 ),
    m_stopScheduling(false ),
    m_haltScheduling( false ),
    m_progress( 0 )
{
    KGlobal::ref(); // keep locale around

    manager->createSchedules(); // creates expected() to get log messages during calculation

    QDomDocument document( "kplato" );
    saveProject( project, document );

    m_pdoc.setContent( document.toString() );

    
    connect( this, SIGNAL(started()), this, SLOT(slotStarted()));
    connect( this, SIGNAL(finished()), this, SLOT(slotFinished()));
}

SchedulerThread::~SchedulerThread()
{
    qDebug()<<"SchedulerThread::~SchedulerThread:"<<QThread::currentThreadId();
    delete m_project;
    m_project = 0;
    KGlobal::deref();
}

void SchedulerThread::setMaxProgress( int value )
{
    m_maxprogressMutex.lock();
    m_maxprogress = value;
    m_maxprogressMutex.unlock();
    emit maxProgressChanged( value, m_mainmanager );
}

int SchedulerThread::maxProgress() const
{
    QMutexLocker m( &m_maxprogressMutex );
    return m_maxprogress;
}

void SchedulerThread::setProgress( int value )
{
    m_progressMutex.lock();
    m_progress = value;
    m_progressMutex.unlock();
    emit progressChanged( value, m_mainmanager );
}

int SchedulerThread::progress() const
{
    QMutexLocker m( &m_progressMutex );
    return m_progress;
}

void SchedulerThread::slotAddLog( KPlato::Schedule::Log log )
{
    //qDebug()<<"SchedulerThread::slotAddLog:"<<log;
    QMutexLocker m( &m_logMutex );
    m_logs << log;
}

QList<Schedule::Log> SchedulerThread::log()
{
    QMutexLocker m( &m_logMutex );
    QList<KPlato::Schedule::Log> l = m_logs;
    m_logs.clear();
    return l;
}


void SchedulerThread::slotStarted()
{
    emit jobStarted( this );
}

void SchedulerThread::slotFinished()
{
    if ( m_haltScheduling ) {
        deleteLater();
    } else {
        emit jobFinished( this );
    }
}

void SchedulerThread::doRun()
{
    slotStarted();
    run();
    slotFinished();
}

ScheduleManager *SchedulerThread::manager() const
{
    QMutexLocker m( &m_managerMutex );
    return m_manager;
}

Project *SchedulerThread::project() const
{
    QMutexLocker m( &m_projectMutex );
    return m_project;
}

void SchedulerThread::stopScheduling()
{
    qDebug()<<"SchedulerThread::stopScheduling:";
    m_stopScheduling = true;
}

void SchedulerThread::haltScheduling()
{
    qDebug()<<"SchedulerThread::haltScheduling:";
    m_haltScheduling = true;
}

//static
void SchedulerThread::saveProject( Project *project, QDomDocument &document )
{
    document.appendChild( document.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );

    QDomElement doc = document.createElement( "kplato" );
    doc.setAttribute( "editor", "KPlato" );
    doc.setAttribute( "mime", "application/x-vnd.kde.kplato" );
    doc.setAttribute( "version", KPLATO_FILE_SYNTAX_VERSION );
    document.appendChild( doc );
    project->save( doc );
}

//static
bool SchedulerThread::loadProject( Project *project, const KoXmlDocument &doc )
{
    KoXmlElement pel = doc.documentElement().namedItem( "project" ).toElement();
    if ( pel.isNull() ) {
        return false;
    }
    XMLLoaderObject status;
    status.setVersion( KPLATO_FILE_SYNTAX_VERSION );
    status.setProject( project );
    return project->load( pel, status );
}


} //namespace KPlato

