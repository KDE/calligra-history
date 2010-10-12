/* This file is part of the KDE project
  Copyright (C) 2006 - 2010 Dag Andersen <danders@get2net.dk>

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

#include "kpttaskeditor.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptnodeitemmodel.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptitemviewsettup.h"
#include "kptworkpackagesenddialog.h"
#include "kptworkpackagesendpanel.h"

#include <KoDocument.h>

#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenu>
#include <QDragMoveEvent>

#include <kicon.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <kaccelgen.h>
#include <kactioncollection.h>

namespace KPlato
{

//--------------------
TaskEditorItemModel::TaskEditorItemModel( QObject *parent )
: NodeItemModel( parent )
{
}

Qt::ItemFlags TaskEditorItemModel::flags( const QModelIndex &index ) const
{
    if ( index.column() == NodeModel::NodeType ) {
        if ( ! m_readWrite || isColumnReadOnly( index.column() ) ) {
            return QAbstractItemModel::flags( index );
        }
        Node *n = node( index );
        if ( n && ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
            return QAbstractItemModel::flags( index ) | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
        }
        return QAbstractItemModel::flags( index ) | Qt::ItemIsDropEnabled;
    }
    return NodeItemModel::flags( index );
}

QVariant TaskEditorItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && section == NodeModel::NodeType ) {
        if ( role == Qt::ToolTipRole ) {
            return i18nc( "@info:tooltip", "The type of task or the estimate type of the task" );
        } else if ( role == Qt::WhatsThisRole ) {
            return i18nc( "@info:whatsthis",
                          "<p>Indicates the type of task or the estimate type of the task.</p>"
                          "The type can be set to <emphasis>Milestone</emphasis>, <emphasis>Effort</emphasis> or <emphasis>Duration</emphasis>.<nl/>"
                          "<note>If the type is <emphasis>Summary</emphasis> or <emphasis>Project</emphasis> the type is not editable.</note>");
        }
    }
    return NodeItemModel::headerData(section, orientation, role);
}

QVariant TaskEditorItemModel::data( const QModelIndex &index, int role ) const
{
    Node *n = node( index );
    if ( n != 0 && index.column() == NodeModel::NodeType ) {
        return type( n, role );
    }
    return NodeItemModel::data( index, role );
}

bool TaskEditorItemModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    Node *n = node( index );
    if ( n != 0 && role == Qt::EditRole && index.column() == NodeModel::NodeType ) {
        return setType( n, value, role );
    }
    return NodeItemModel::setData( index, value, role );
}

QVariant TaskEditorItemModel::type( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->typeToString( true );
            }
            return node->typeToString( true );
        }
        case Qt::EditRole:
            return node->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole: {
            if ( node->type() == Node::Type_Task ) {
                return i18nc( "@info:tooltip", "Task with estimate type: %1", node->estimate()->typeToString( true ) );
            }
            return i18nc( "@info:tooltip", "Task type: %1", node->typeToString( true ) );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::EnumListValue: {
            if ( node->type() == Node::Type_Milestone ) {
                return 0;
            }
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->type() + 1;
            }
            return -1;
        }
        case Role::EnumList: {
            QStringList lst;
            lst << Node::typeToString( Node::Type_Milestone, true );
            lst += Estimate::typeToStringList( true );
            return lst;
        }
    }
    return QVariant();
}

bool TaskEditorItemModel::setType( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            if ( node->type() == Node::Type_Summarytask ) {
                return false;
            }
            int v = value.toInt();
            switch ( v ) {
                case 0: { // Milestone
                    ModifyEstimateCmd *cmd =  new ModifyEstimateCmd( *node, node->estimate()->expectedEstimate(), 0.0, i18n( "Set type to Milestone" ) );
                    emit executeCommand( cmd );
                    return true;
                }
                default: { // Estimate
                    --v;
                    MacroCommand *m = new MacroCommand( i18n( "Set type to %1", Estimate::typeToString( (Estimate::Type)v, true ) ) );
                    m->addCommand( new ModifyEstimateTypeCmd( *node, node->estimate()->type(), v ) );
                    if ( node->type() == Node::Type_Milestone ) {
                        m->addCommand( new ModifyEstimateUnitCmd( *node, node->estimate()->unit(), Duration::Unit_d ) );
                        m->addCommand( new ModifyEstimateCmd( *node, node->estimate()->expectedEstimate(), 1.0 ) );
                    }
                    emit executeCommand( m );
                    return true;
                }
            }
            break;
        }
        default: break;
    }
    return false;
}

//--------------------
TaskEditorTreeView::TaskEditorTreeView( QWidget *parent )
    : DoubleTreeViewBase( parent )
{
    TaskEditorItemModel *m = new TaskEditorItemModel( this );
    setModel( m );
    //setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );

    createItemDelegates( m );
    setItemDelegateForColumn( NodeModel::NodeType, new EnumDelegate( this ) );

    connect( this, SIGNAL( dropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ), SLOT(slotDropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ) );
}

NodeItemModel *TaskEditorTreeView::baseModel() const
{
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        return static_cast<NodeItemModel*>( pr->sourceModel() );
    }
    return static_cast<NodeItemModel*>( model() );
}

void TaskEditorTreeView::slotDropAllowed( const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event )
{
    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        idx = pr->mapToSource( index );
    }
    if ( baseModel()->dropAllowed( idx, dropIndicatorPosition, event->mimeData() ) ) {
        event->accept();
    }
}

//--------------------
NodeTreeView::NodeTreeView( QWidget *parent )
    : DoubleTreeViewBase( parent )
{
    NodeItemModel *m = new NodeItemModel( this );
    setModel( m );
    //setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );

    createItemDelegates( m );

    connect( this, SIGNAL( dropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ), SLOT(slotDropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ) );
}

NodeItemModel *NodeTreeView::baseModel() const
{
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        return static_cast<NodeItemModel*>( pr->sourceModel() );
    }
    return static_cast<NodeItemModel*>( model() );
}

void NodeTreeView::slotDropAllowed( const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event )
{
    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        idx = pr->mapToSource( index );
    }
    if ( baseModel()->dropAllowed( idx, dropIndicatorPosition, event->mimeData() ) ) {
        event->accept();
    }
}


//-----------------------------------
TaskEditor::TaskEditor( KoDocument *part, QWidget *parent )
    : ViewBase( part, parent )
{
    kDebug()<<"----------------- Create TaskEditor ----------------------";
    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new TaskEditorTreeView( this );
    l->addWidget( m_view );
    kDebug()<<m_view->actionSplitView();
    setupGui();

    m_view->setEditTriggers( m_view->editTriggers() | QAbstractItemView::EditKeyPressed );

    m_view->setDragDropMode( QAbstractItemView::InternalMove );
    m_view->setDropIndicatorShown( true );
    m_view->setDragEnabled ( true );
    m_view->setAcceptDrops( true );
    m_view->setAcceptDropsOnView( true );

    QList<int> lst1; lst1 << 1 << -1; // only display column 0 (NodeName) in left view
    QList<int> show;
    show << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeType
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;

    QList<int> lst2;
    for ( int i = 0; i < model()->columnCount(); ++i ) {
        if ( ! show.contains( i ) ) {
            lst2 << i;
        }
    }
    for ( int i = 0; i < show.count(); ++i ) {
        int sec = m_view->slaveView()->header()->visualIndex( show[ i ] );
        //kDebug()<<"move section:"<<i<<show[i]<<sec;
        if ( i != sec ) {
            m_view->slaveView()->header()->moveSection( sec, i );
        }
    }
    m_view->hideColumns( lst1, lst2 );
    m_view->masterView()->setDefaultColumns( QList<int>() << NodeModel::NodeName );
    m_view->slaveView()->setDefaultColumns( show );

    connect( model(), SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );

    connect( m_view, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT ( slotCurrentChanged( const QModelIndex &, const QModelIndex & ) ) );

    connect( m_view, SIGNAL( selectionChanged( const QModelIndexList ) ), this, SLOT ( slotSelectionChanged( const QModelIndexList ) ) );

    connect( m_view, SIGNAL( contextMenuRequested( const QModelIndex&, const QPoint& ) ), SLOT( slotContextMenuRequested( const QModelIndex&, const QPoint& ) ) );

    connect( m_view, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );

}

void TaskEditor::updateReadWrite( bool rw )
{
    m_view->setReadWrite( rw );
}

void TaskEditor::draw( Project &project )
{
    m_view->setProject( &project );
}

void TaskEditor::draw()
{
}

void TaskEditor::setGuiActive( bool activate )
{
    kDebug()<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate && !m_view->selectionModel()->currentIndex().isValid() ) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
    }
}

void TaskEditor::slotCurrentChanged(  const QModelIndex &curr, const QModelIndex & )
{
    kDebug()<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskEditor::slotSelectionChanged( const QModelIndexList list)
{
    kDebug()<<list.count();
    slotEnableActions();
}

QModelIndexList TaskEditor::selectedRows() const
{
#if 0
// Qt bug?
    return m_view->selectionModel()->selectedRows();
#else
    QModelIndexList lst;
    foreach ( QModelIndex i, m_view->selectionModel()->selectedIndexes() ) {
        if ( i.column() == 0 ) {
            lst << i;
        }
    }
    return lst;
#endif
}

int TaskEditor::selectedRowCount() const
{
    return selectedRows().count();
}

QList<Node*> TaskEditor::selectedNodes() const {
    QList<Node*> lst;
    foreach ( const QModelIndex &i, selectedRows() ) {
        Node * n = m_view->baseModel()->node( i );
        if ( n != 0 && n->type() != Node::Type_Project ) {
            lst.append( n );
        }
    }
    return lst;
}

Node *TaskEditor::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if ( lst.count() != 1 ) {
        return 0;
    }
    return lst.first();
}

Node *TaskEditor::currentNode() const {
    Node * n = m_view->baseModel()->node( m_view->selectionModel()->currentIndex() );
    if ( n == 0 || n->type() == Node::Type_Project ) {
        return 0;
    }
    return n;
}

void TaskEditor::slotContextMenuRequested( const QModelIndex& index, const QPoint& pos )
{
    Node *node = m_view->baseModel()->node( index );
    if ( node == 0 ) {
        return;
    }
    kDebug()<<node->name()<<" :"<<pos;
    QString name;
    switch ( node->type() ) {
        case Node::Type_Project:
            name = "task_edit_popup";
            break;
        case Node::Type_Task:
            name = node->isScheduled( baseModel()->id() ) ? "task_popup" : "task_edit_popup";
            break;
        case Node::Type_Milestone:
            name = node->isScheduled( baseModel()->id() ) ? "taskeditor_milestone_popup" : "task_edit_popup";
            break;
        case Node::Type_Summarytask:
            name = "summarytask_popup";
            break;
        default:
            name = "node_popup";
            break;
    }
    if ( name.isEmpty() ) {
        slotHeaderContextMenuRequested( pos );
        return;
    }
    kDebug()<<name;
    emit requestPopupMenu( name, pos );
}

void TaskEditor::setScheduleManager( ScheduleManager *sm )
{
    //kDebug()<<endl;
    m_view->baseModel()->setScheduleManager( sm );
}

void TaskEditor::slotEnableActions()
{
    updateActionsEnabled( true );
}

void TaskEditor::updateActionsEnabled( bool on )
{
    Project *p = m_view->project();
    int selCount = selectedRowCount();
    Node *n = selectedNode(); // 0 if not task or milestone
    bool o = ( on && p && selCount <= 1 );
    menuAddTask->setEnabled( o && n != p );
    actionAddTask->setEnabled( o && n != p );
    actionAddMilestone->setEnabled( o && n != p );

    int projSelected = selCount == 1 && n == 0;
    actionDeleteTask->setEnabled( on && p && ! projSelected && selCount > 0 );

    o = ( on && p && selCount == 1 );

    menuAddSubTask->setEnabled( o );
    actionAddSubtask->setEnabled( o );
    actionAddSubMilestone->setEnabled( o );
    actionMoveTaskUp->setEnabled( o && p->canMoveTaskUp( n ) );
    actionMoveTaskDown->setEnabled( o && p->canMoveTaskDown( n ) );
    actionIndentTask->setEnabled( o && p->canIndentTask( n ) );
    actionUnindentTask->setEnabled( o && p->canUnindentTask( n ) );
}

void TaskEditor::setupGui()
{
    QString name = "taskeditor_add_list";

    menuAddTask = new KActionMenu( KIcon( "view-task-add" ), i18n( "Add Task" ), this );
    actionCollection()->addAction("add_task", menuAddTask );
    connect( menuAddTask, SIGNAL( triggered( bool ) ), SLOT( slotAddTask() ) );
    addAction( name, menuAddTask );

    actionAddTask  = new KAction( i18n( "Add Task" ), this);
    actionAddTask->setShortcut( KShortcut( Qt::CTRL + Qt::Key_I ) );
    connect( actionAddTask, SIGNAL( triggered( bool ) ), SLOT( slotAddTask() ) );
    menuAddTask->addAction( actionAddTask );

    actionAddMilestone  = new KAction( i18n( "Add Milestone" ), this );
    actionAddMilestone->setShortcut( KShortcut( Qt::CTRL + Qt::ALT + Qt::Key_I ) );
    connect( actionAddMilestone, SIGNAL( triggered( bool ) ), SLOT( slotAddMilestone() ) );
    menuAddTask->addAction( actionAddMilestone );


    menuAddSubTask = new KActionMenu( KIcon( "view-task-child-add" ), i18n( "Add Sub-Task" ), this );
    actionCollection()->addAction("add_subtask", menuAddTask );
    connect( menuAddSubTask, SIGNAL( triggered( bool ) ), SLOT( slotAddSubtask() ) );
    addAction( name, menuAddSubTask );

    actionAddSubtask  = new KAction( i18n( "Add Sub-Task" ), this );
    actionAddSubtask->setShortcut( KShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_I ) );
    connect( actionAddSubtask, SIGNAL( triggered( bool ) ), SLOT( slotAddSubtask() ) );
    menuAddSubTask->addAction( actionAddSubtask );

    actionAddSubMilestone = new KAction( i18n( "Add Sub-Milestone" ), this );
    actionAddSubMilestone->setShortcut( KShortcut( Qt::CTRL + Qt::SHIFT + Qt::ALT + Qt::Key_I ) );
    connect( actionAddSubMilestone, SIGNAL( triggered( bool ) ), SLOT( slotAddSubMilestone() ) );
    menuAddSubTask->addAction( actionAddSubMilestone );

    actionDeleteTask  = new KAction(KIcon( "edit-delete" ), i18nc( "@action", "Delete" ), this);
    actionDeleteTask->setShortcut( KShortcut( Qt::Key_Delete ) );
    actionCollection()->addAction("delete_task", actionDeleteTask );
    connect( actionDeleteTask, SIGNAL( triggered( bool ) ), SLOT( slotDeleteTask() ) );
    addAction( name, actionDeleteTask );


    name = "taskeditor_move_list";
    actionIndentTask  = new KAction(KIcon("format-indent-more"), i18n("Indent Task"), this);
    actionCollection()->addAction("indent_task", actionIndentTask );
    connect(actionIndentTask, SIGNAL(triggered(bool) ), SLOT(slotIndentTask()));
    addAction( name, actionIndentTask );

    actionUnindentTask  = new KAction(KIcon("format-indent-less"), i18n("Unindent Task"), this);
    actionCollection()->addAction("unindent_task", actionUnindentTask );
    connect(actionUnindentTask, SIGNAL(triggered(bool) ), SLOT(slotUnindentTask()));
    addAction( name, actionUnindentTask );

    actionMoveTaskUp  = new KAction(KIcon("arrow-up"), i18n("Move Up"), this);
    actionCollection()->addAction("move_task_up", actionMoveTaskUp );
    connect(actionMoveTaskUp, SIGNAL(triggered(bool) ), SLOT(slotMoveTaskUp()));
    addAction( name, actionMoveTaskUp );

    actionMoveTaskDown  = new KAction(KIcon("arrow-down"), i18n("Move Down"), this);
    actionCollection()->addAction("move_task_down", actionMoveTaskDown );
    connect(actionMoveTaskDown, SIGNAL(triggered(bool) ), SLOT(slotMoveTaskDown()));
    addAction( name, actionMoveTaskDown );

    // Add the context menu actions for the view options
    actionShowProject = new KToggleAction( i18n( "Show Project" ), this );
    connect(actionShowProject, SIGNAL(triggered(bool) ), baseModel(), SLOT(setShowProject(bool)));
    addContextAction( actionShowProject );

    connect(m_view->actionSplitView(), SIGNAL(triggered(bool) ), SLOT(slotSplitView()));
    addContextAction( m_view->actionSplitView() );

    createOptionAction();
}

void TaskEditor::slotSplitView()
{
    kDebug();
    m_view->setViewSplitMode( ! m_view->isViewSplit() );
    emit optionsModified();
}


void TaskEditor::slotOptions()
{
    kDebug();
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog( m_view, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void TaskEditor::slotAddTask()
{
    kDebug();
    if ( selectedRowCount() == 0 || ( selectedRowCount() == 1 && selectedNode() == 0 ) ) {
        Task *t = m_view->project()->createTask( m_view->project()->taskDefaults(),  m_view->project() );
        QModelIndex idx = m_view->baseModel()->insertSubtask( t, t->parentNode() );
        Q_ASSERT( idx.isValid() );
        m_view->setParentsExpanded( idx, true ); // rightview is not automatically expanded
        edit( idx );
        return;
    }
    Node *sib = selectedNode();
    if ( sib == 0 ) {
        return;
    }
    Task *t = m_view->project()->createTask( m_view->project()->taskDefaults(), sib->parentNode() );
    QModelIndex idx = m_view->baseModel()->insertTask( t, sib );
    Q_ASSERT( idx.isValid() );
    edit( idx );
}

void TaskEditor::slotAddMilestone()
{
    kDebug();
    if ( selectedRowCount() == 0  || ( selectedRowCount() == 1 && selectedNode() == 0 ) ) {
        // None selected or only project selected: insert under main project
        Task *t = m_view->project()->createTask( m_view->project() );
        t->estimate()->clear();
        QModelIndex idx = m_view->baseModel()->insertSubtask( t, t->parentNode() );
        Q_ASSERT( idx.isValid() );
        m_view->setParentsExpanded( idx, true ); // rightview is not automatically expanded
        edit( idx );
        return;
    }
    Node *sib = selectedNode(); // sibling
    if ( sib == 0 ) {
        return;
    }
    Task *t = m_view->project()->createTask( sib->parentNode() );
    t->estimate()->clear();
    QModelIndex idx = m_view->baseModel()->insertTask( t, sib );
    Q_ASSERT( idx.isValid() );
    m_view->setParentsExpanded( idx, true ); // rightview is not automatically expanded
    edit( idx );
}

void TaskEditor::slotAddSubMilestone()
{
    kDebug();
    Node *parent = selectedNode();
    if ( parent == 0 && selectedRowCount() == 1 ) {
        // project selected
        parent = m_view->project();
    }
    if ( parent == 0 ) {
        return;
    }
    Task *t = m_view->project()->createTask( m_view->project()->taskDefaults(), parent );
    t->estimate()->clear();
    QModelIndex idx = m_view->baseModel()->insertSubtask( t, parent );
    Q_ASSERT( idx.isValid() );
    m_view->setParentsExpanded( idx, true ); // rightview is not automatically expanded
    edit( idx );
}

void TaskEditor::slotAddSubtask()
{
    kDebug();
    Node *parent = selectedNode();
    if ( parent == 0 && selectedRowCount() == 1 ) {
        // project selected
        parent = m_view->project();
    }
    if ( parent == 0 ) {
        return;
    }
    Task *t = m_view->project()->createTask( m_view->project()->taskDefaults(), parent );
    QModelIndex idx = m_view->baseModel()->insertSubtask( t, parent );
    Q_ASSERT( idx.isValid() );
    m_view->setParentsExpanded( idx, true ); // rightview is not automatically expanded
    edit( idx );
}

void TaskEditor::edit( QModelIndex i )
{
    if ( i.isValid() ) {
        if ( m_view->slaveView()->hasFocus() ) {
            m_view->masterView()->setFocus();
        }

        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
        QModelIndex p = m_view->model()->parent( i );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->edit( i );
    }
}

void TaskEditor::slotDeleteTask()
{
    //kDebug();
    QList<Node*> lst = selectedNodes();
    while ( true ) {
        // remove children of selected tasks, as parents delete their children
        Node *ch = 0;
        foreach ( Node *n1, lst ) {
            foreach ( Node *n2, lst ) {
                if ( n2->isChildOf( n1 ) ) {
                    ch = n2;
                    break;
                }
            }
            if ( ch != 0 ) {
                break;
            }
        }
        if ( ch == 0 ) {
            break;
        }
        lst.removeAt( lst.indexOf( ch ) );
    }
    //foreach ( Node* n, lst ) { kDebug()<<n->name(); }
    emit deleteTaskList( lst );
    QModelIndex i = m_view->selectionModel()->currentIndex();
    if ( i.isValid() ) {
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
    }
}

void TaskEditor::slotIndentTask()
{
    kDebug();
    Node *n = selectedNode();
    if ( n ) {
        emit indentTask();
        QModelIndex i = baseModel()->index( n );
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->setParentsExpanded( i, true );
    }
}

void TaskEditor::slotUnindentTask()
{
    kDebug();
    Node *n = selectedNode();
    if ( n ) {
        emit unindentTask();
        QModelIndex i = baseModel()->index( n );
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
    }
}

void TaskEditor::slotMoveTaskUp()
{
    kDebug();
    Node *n = selectedNode();
    if ( n ) {
        emit moveTaskUp();
        QModelIndex i = baseModel()->index( n );
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
    }
}

void TaskEditor::slotMoveTaskDown()
{
    kDebug();
    Node *n = selectedNode();
    if ( n ) {
        emit moveTaskDown();
        QModelIndex i = baseModel()->index( n );
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect );
         m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
   }
}

bool TaskEditor::loadContext( const KoXmlElement &context )
{
    kDebug();
    bool show = (bool)(context.attribute( "show-project", "0" ).toInt() );
    actionShowProject->setChecked( show );
    baseModel()->setShowProject( show ); // why is this not called by the action?
    return m_view->loadContext( baseModel()->columnMap(), context );
}

void TaskEditor::saveContext( QDomElement &context ) const
{
    context.setAttribute( "show-project", baseModel()->projectShown() );
    m_view->saveContext( baseModel()->columnMap(), context );
}

KoPrintJob *TaskEditor::createPrintJob()
{
    return m_view->createPrintJob( this );
}


//-----------------------------------
TaskView::TaskView( KoDocument *part, QWidget *parent )
    : ViewBase( part, parent )
{
    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new NodeTreeView( this );
    NodeSortFilterProxyModel *p = new NodeSortFilterProxyModel( m_view->baseModel(), m_view );
    m_view->setModel( p );
    l->addWidget( m_view );
    setupGui();

    //m_view->setEditTriggers( m_view->editTriggers() | QAbstractItemView::EditKeyPressed );
    m_view->setDragDropMode( QAbstractItemView::InternalMove );
    m_view->setDropIndicatorShown( false );
    m_view->setDragEnabled ( true );
    m_view->setAcceptDrops( false );
    m_view->setAcceptDropsOnView( false );

    QList<int> readonly;
    readonly << NodeModel::NodeName
            << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeEstimateType
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;
    foreach ( int c, readonly ) {
        m_view->baseModel()->setReadOnly( c, true );
    }

    QList<int> lst1; lst1 << 1 << -1;
    QList<int> show;
    show << NodeModel::NodeStatus
            << NodeModel::NodeCompleted
            << NodeModel::NodeResponsible
            << NodeModel::NodeAssignments
            << NodeModel::NodePerformanceIndex
            << NodeModel::NodeBCWS
            << NodeModel::NodeBCWP
            << NodeModel::NodeACWP
            << NodeModel::NodeDescription;

    for ( int s = 0; s < show.count(); ++s ) {
        m_view->slaveView()->mapToSection( show[s], s );
    }
    QList<int> lst2;
    for ( int i = 0; i < m_view->model()->columnCount(); ++i ) {
        if ( ! show.contains( i ) ) {
            lst2 << i;
        }
    }
    m_view->hideColumns( lst1, lst2 );
    m_view->masterView()->setDefaultColumns( QList<int>() << 0 );
    m_view->slaveView()->setDefaultColumns( show );

    connect( m_view->baseModel(), SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );

    connect( m_view, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT ( slotCurrentChanged( const QModelIndex &, const QModelIndex & ) ) );

    connect( m_view, SIGNAL( selectionChanged( const QModelIndexList ) ), this, SLOT ( slotSelectionChanged( const QModelIndexList ) ) );

    connect( m_view, SIGNAL( contextMenuRequested( const QModelIndex&, const QPoint& ) ), SLOT( slotContextMenuRequested( const QModelIndex&, const QPoint& ) ) );

    connect( m_view, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
}

void TaskView::updateReadWrite( bool rw )
{
    m_view->setReadWrite( rw );
}

void TaskView::draw( Project &project )
{
    m_view->setProject( &project );
}

void TaskView::draw()
{
}

void TaskView::setGuiActive( bool activate )
{
    kDebug()<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate && !m_view->selectionModel()->currentIndex().isValid() ) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
    }
}

void TaskView::slotCurrentChanged(  const QModelIndex &curr, const QModelIndex & )
{
    kDebug()<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskView::slotSelectionChanged( const QModelIndexList list)
{
    kDebug()<<list.count();
    slotEnableActions();
}

int TaskView::selectedNodeCount() const
{
    QItemSelectionModel* sm = m_view->selectionModel();
    return sm->selectedRows().count();
}

QList<Node*> TaskView::selectedNodes() const {
    QList<Node*> lst;
    QItemSelectionModel* sm = m_view->selectionModel();
    if ( sm == 0 ) {
        return lst;
    }
    foreach ( const QModelIndex &i, sm->selectedRows() ) {
        Node * n = m_view->baseModel()->node( proxyModel()->mapToSource( i ) );
        if ( n != 0 && n->type() != Node::Type_Project ) {
            lst.append( n );
        }
    }
    return lst;
}

Node *TaskView::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if ( lst.count() != 1 ) {
        return 0;
    }
    return lst.first();
}

Node *TaskView::currentNode() const {
    Node * n = m_view->baseModel()->node( proxyModel()->mapToSource( m_view->selectionModel()->currentIndex() ) );
    if ( n == 0 || n->type() == Node::Type_Project ) {
        return 0;
    }
    return n;
}

void TaskView::slotContextMenuRequested( const QModelIndex& index, const QPoint& pos )
{
    QString name;
    Node *node = m_view->baseModel()->node( proxyModel()->mapToSource( index ) );
    if ( node ) {
        switch ( node->type() ) {
            case Node::Type_Task:
                name = "taskview_popup";
                break;
            case Node::Type_Milestone:
                name = "taskview_milestone_popup";
                break;
            case Node::Type_Summarytask:
                name = "taskview_summary_popup";
                break;
            default:
                break;
        }
    } else kDebug()<<"No node: "<<index;
    if ( name.isEmpty() ) {
        kDebug()<<"No menu";
        slotHeaderContextMenuRequested( pos );
        return;
    }
    emit requestPopupMenu( name, pos );
}

void TaskView::setScheduleManager( ScheduleManager *sm )
{
    //kDebug()<<endl;
    m_view->baseModel()->setScheduleManager( sm );
}

void TaskView::slotEnableActions()
{
    updateActionsEnabled( true );
}

void TaskView::updateActionsEnabled( bool /*on*/ )
{

}

void TaskView::setupGui()
{
//    KActionCollection *coll = actionCollection();

    // Add the context menu actions for the view options
    actionShowProject = new KToggleAction( i18n( "Show Project" ), this );
    connect(actionShowProject, SIGNAL(triggered(bool) ), baseModel(), SLOT(setShowProject(bool)));
    addContextAction( actionShowProject );

    connect(m_view->actionSplitView(), SIGNAL(triggered(bool) ), SLOT(slotSplitView()));
    addContextAction( m_view->actionSplitView() );

    createOptionAction();
}

void TaskView::slotSplitView()
{
    kDebug();
    m_view->setViewSplitMode( ! m_view->isViewSplit() );
    emit optionsModified();
}

void TaskView::slotOptions()
{
    kDebug();
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog( m_view, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

bool TaskView::loadContext( const KoXmlElement &context )
{
    bool show = (bool)(context.attribute( "show-project", "0" ).toInt() );
    actionShowProject->setChecked( show );
    baseModel()->setShowProject( show ); // why is this not called by the action?
    return m_view->loadContext( m_view->baseModel()->columnMap(), context );
}

void TaskView::saveContext( QDomElement &context ) const
{
    context.setAttribute( "show-project", baseModel()->projectShown() );
    m_view->saveContext( m_view->baseModel()->columnMap(), context );
}

KoPrintJob *TaskView::createPrintJob()
{
    return m_view->createPrintJob( this );
}

//---------------------------------
GeneralNodeTreeView::GeneralNodeTreeView( QWidget *parent )
    : DoubleTreeViewBase( parent )
{
    GeneralNodeItemModel *m = new GeneralNodeItemModel( this );
    setModel( m );
    //setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );

    createItemDelegates( m );

    connect( this, SIGNAL( dropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ), SLOT(slotDropAllowed( const QModelIndex&, int, QDragMoveEvent* ) ) );
}

void GeneralNodeTreeView::setModus( int mode )
{
    baseModel()->setModus( mode );
}

GeneralNodeItemModel *GeneralNodeTreeView::baseModel() const
{
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        return static_cast<GeneralNodeItemModel*>( pr->sourceModel() );
    }
    return static_cast<GeneralNodeItemModel*>( model() );
}

void GeneralNodeTreeView::slotDropAllowed( const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event )
{
    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if ( pr ) {
        idx = pr->mapToSource( index );
    }
    if ( baseModel()->dropAllowed( idx, dropIndicatorPosition, event->mimeData() ) ) {
        event->accept();
    }
}

//--------------------------------
TaskWorkPackageView::TaskWorkPackageView( KoDocument *part, QWidget *parent )
    : ViewBase( part, parent ),
    m_cmd( 0 )
{
    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new GeneralNodeTreeView( this );
    m_view->setModus( GeneralNodeItemModel::Flat | GeneralNodeItemModel::WorkPackage );
    NodeSortFilterProxyModel *p = new NodeSortFilterProxyModel( m_view->baseModel(), m_view, false );
    m_view->setModel( p );

    m_view->setSortingEnabled( true );
    m_view->sortByColumn( NodeModel::NodeWBSCode, Qt::AscendingOrder );

    // match empty string or Type_Task
    p->setFilterRegExp( QRegExp( QString( "^$|%1").arg( Node::Type_Task ) ) );
    p->setFilterRole( Qt::EditRole );
    p->setFilterKeyColumn( NodeModel::NodeType );

    l->addWidget( m_view );
    setupGui();

    //m_view->setEditTriggers( m_view->editTriggers() | QAbstractItemView::EditKeyPressed );
    m_view->setDragDropMode( QAbstractItemView::InternalMove );
    m_view->setDropIndicatorShown( false );
    m_view->setDragEnabled ( true );
    m_view->setAcceptDrops( false );
    m_view->setAcceptDropsOnView( false );

    QList<int> readonly;
    readonly << NodeModel::NodeName
            << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeEstimateType
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;
    foreach ( int c, readonly ) {
        m_view->baseModel()->setReadOnly( c, true );
    }

    QList<int> lst1; lst1 << 1 << -1;
    QList<int> show;
    show << NodeModel::NodeStatus
            << NodeModel::NodeCompleted
            << NodeModel::NodeResponsible
            << NodeModel::NodeAssignments
            << NodeModel::NodeDescription;

    for ( int s = 0; s < show.count(); ++s ) {
        m_view->slaveView()->mapToSection( show[s], s );
    }
    QList<int> lst2;
    for ( int i = 0; i < m_view->model()->columnCount(); ++i ) {
        if ( ! show.contains( i ) ) {
            lst2 << i;
        }
    }
    m_view->hideColumns( lst1, lst2 );
    m_view->masterView()->setDefaultColumns( QList<int>() << 0 );
    m_view->slaveView()->setDefaultColumns( show );

    connect( m_view->baseModel(), SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );

    connect( m_view, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT ( slotCurrentChanged( const QModelIndex &, const QModelIndex & ) ) );

    connect( m_view, SIGNAL( selectionChanged( const QModelIndexList ) ), this, SLOT ( slotSelectionChanged( const QModelIndexList ) ) );

    connect( m_view, SIGNAL( contextMenuRequested( const QModelIndex&, const QPoint& ) ), SLOT( slotContextMenuRequested( const QModelIndex&, const QPoint& ) ) );

    connect( m_view, SIGNAL( headerContextMenuRequested( const QPoint& ) ), SLOT( slotHeaderContextMenuRequested( const QPoint& ) ) );
}

Project *TaskWorkPackageView::project() const
{
    return m_view->project();
}

void TaskWorkPackageView::setProject( Project *project )
{
    m_view->setProject( project );
}

NodeSortFilterProxyModel *TaskWorkPackageView::proxyModel() const
{
    return m_view->proxyModel();
}

void TaskWorkPackageView::updateReadWrite( bool rw )
{
    m_view->setReadWrite( rw );
}

void TaskWorkPackageView::setGuiActive( bool activate )
{
    kDebug()<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate && !m_view->selectionModel()->currentIndex().isValid() ) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
    }
}

void TaskWorkPackageView::slotCurrentChanged(  const QModelIndex &curr, const QModelIndex & )
{
    kDebug()<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskWorkPackageView::slotSelectionChanged( const QModelIndexList list)
{
    kDebug()<<list.count();
    slotEnableActions();
}

int TaskWorkPackageView::selectedNodeCount() const
{
    QItemSelectionModel* sm = m_view->selectionModel();
    return sm->selectedRows().count();
}

QList<Node*> TaskWorkPackageView::selectedNodes() const {
    QList<Node*> lst;
    QItemSelectionModel* sm = m_view->selectionModel();
    if ( sm == 0 ) {
        return lst;
    }
    foreach ( const QModelIndex &i, sm->selectedRows() ) {
        Node * n = m_view->baseModel()->node( proxyModel()->mapToSource( i ) );
        if ( n != 0 && n->type() != Node::Type_Project ) {
            lst.append( n );
        }
    }
    return lst;
}

Node *TaskWorkPackageView::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if ( lst.count() != 1 ) {
        return 0;
    }
    return lst.first();
}

Node *TaskWorkPackageView::currentNode() const {
    Node * n = m_view->baseModel()->node( proxyModel()->mapToSource( m_view->selectionModel()->currentIndex() ) );
    if ( n == 0 || n->type() == Node::Type_Project ) {
        return 0;
    }
    return n;
}

void TaskWorkPackageView::slotContextMenuRequested( const QModelIndex& index, const QPoint& pos )
{
    QString name;
    Node *node = m_view->baseModel()->node( proxyModel()->mapToSource( index ) );
    if ( node ) {
        switch ( node->type() ) {
            case Node::Type_Task:
                name = "workpackage_popup";
                break;
            case Node::Type_Milestone:
                name = "taskview_milestone_popup";
                break;
            case Node::Type_Summarytask:
                name = "taskview_summary_popup";
                break;
            default:
                break;
        }
    } else kDebug()<<"No node: "<<index;
    if ( name.isEmpty() ) {
        kDebug()<<"No menu";
        slotHeaderContextMenuRequested( pos );
        return;
    }
    emit requestPopupMenu( name, pos );
}

void TaskWorkPackageView::setScheduleManager( ScheduleManager *sm )
{
    //kDebug()<<endl;
    m_view->baseModel()->setScheduleManager( sm );
}

void TaskWorkPackageView::slotEnableActions()
{
    updateActionsEnabled( true );
}

void TaskWorkPackageView::updateActionsEnabled( bool on )
{
    bool o = ! selectedNodes().isEmpty();
    actionMailWorkpackage->setEnabled( o && on );
}

void TaskWorkPackageView::setupGui()
{
//    KActionCollection *coll = actionCollection();

    QString name = "workpackage_list";
    actionMailWorkpackage  = new KAction(KIcon( "mail-send" ), i18n("Send..."), this);
    actionMailWorkpackage->setShortcut( KShortcut( Qt::CTRL + Qt::Key_M ) );
    actionCollection()->addAction("send_workpackage", actionMailWorkpackage );
    connect( actionMailWorkpackage, SIGNAL( triggered( bool ) ), SLOT( slotMailWorkpackage() ) );
    addAction( name, actionMailWorkpackage );

    // Add the context menu actions for the view options
    connect(m_view->actionSplitView(), SIGNAL(triggered(bool) ), SLOT(slotSplitView()));
    addContextAction( m_view->actionSplitView() );

    createOptionAction();
}

void TaskWorkPackageView::slotMailWorkpackage()
{
    QList<Node*> lst = selectedNodes();
    if ( ! lst.isEmpty() ) {
        // TODO find a better way to log to avoid undo/redo
        m_cmd = new MacroCommand( "Log Send Workpackages" );
        WorkPackageSendDialog *dlg = new WorkPackageSendDialog( lst, scheduleManager(), this );
        connect ( dlg->panel(), SIGNAL( sendWorkpackages( QList<Node*>&, Resource* ) ), this, SIGNAL( mailWorkpackages( QList<Node*>&, Resource* ) ) );

        connect ( dlg->panel(), SIGNAL( sendWorkpackages( QList<Node*>&, Resource* ) ), this, SLOT( slotWorkPackageSent( QList<Node*>&, Resource* ) ) );
        dlg->exec();
        delete dlg;
        if ( ! m_cmd->isEmpty() ) {
            part()->addCommand( m_cmd );
            m_cmd = 0;
        }
        delete m_cmd;
        m_cmd = 0;
    }
}

void TaskWorkPackageView::slotWorkPackageSent( QList<Node*> &nodes, Resource *resource )
{
    foreach ( Node *n, nodes ) {
        WorkPackage *wp = new WorkPackage( static_cast<Task*>( n )->workPackage() );
        wp->setOwnerName( resource->name() );
        wp->setOwnerId( resource->id() );
        wp->setTransmitionTime( DateTime::currentLocalDateTime() );
        wp->setTransmitionStatus( WorkPackage::TS_Send );
        m_cmd->addCommand( new WorkPackageAddCmd( static_cast<Project*>( n->projectNode() ), n, wp ) );
    }
}

void TaskWorkPackageView::slotSplitView()
{
    kDebug();
    m_view->setViewSplitMode( ! m_view->isViewSplit() );
    emit optionsModified();
}

void TaskWorkPackageView::slotOptions()
{
    kDebug();
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog( m_view, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

bool TaskWorkPackageView::loadContext( const KoXmlElement &context )
{
    kDebug();
    return m_view->loadContext( m_view->baseModel()->columnMap(), context );
}

void TaskWorkPackageView::saveContext( QDomElement &context ) const
{
    m_view->saveContext( m_view->baseModel()->columnMap(), context );
}

KoPrintJob *TaskWorkPackageView::createPrintJob()
{
    return m_view->createPrintJob( this );
}

} // namespace KPlato

#include "kpttaskeditor.moc"
