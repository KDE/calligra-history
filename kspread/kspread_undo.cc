/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include "kspread_global.h"
#include "kspread_undo.h"
#include "kspread_doc.h"
#include "kspread_map.h"
#include "kspread_util.h"

/****************************************************************************
 *
 * KSpreadUndo
 *
 ***************************************************************************/

KSpreadUndo::KSpreadUndo( KSpreadDoc *_doc )
{
    m_pDoc = _doc;

    m_stckUndo.setAutoDelete( FALSE );
    m_stckRedo.setAutoDelete( FALSE );

    m_bLocked = FALSE;
}

KSpreadUndo::~KSpreadUndo()
{
    clear();
}

void KSpreadUndo::appendUndo( KSpreadUndoAction *_action )
{
    if ( m_bLocked )
	return;

    m_stckRedo.setAutoDelete( TRUE );
    m_stckRedo.clear();
    m_stckRedo.setAutoDelete( FALSE );

    m_stckUndo.push( _action );

    if ( m_pDoc )
    {
	m_pDoc->enableUndo( hasUndoActions() );
	m_pDoc->enableRedo( hasRedoActions() );
        m_pDoc->setModified( true );
    }
}

void KSpreadUndo::clear()
{
    if ( m_bLocked )
	return;

    m_stckUndo.setAutoDelete( TRUE );
    m_stckRedo.setAutoDelete( TRUE );

    m_stckUndo.clear();
    m_stckRedo.clear();

    m_stckUndo.setAutoDelete( FALSE );
    m_stckRedo.setAutoDelete( FALSE );
}

void KSpreadUndo::undo()
{
    if ( m_stckUndo.isEmpty() )
	return;
    KSpreadUndoAction *a = m_stckUndo.pop();
    a->undo();
    m_stckRedo.push( a );

    if ( m_pDoc )
    {
	m_pDoc->enableUndo( hasUndoActions() );
	m_pDoc->enableRedo( hasRedoActions() );
    }
}

void KSpreadUndo::redo()
{
    if ( m_stckRedo.isEmpty() )
	return;
    KSpreadUndoAction *a = m_stckRedo.pop();
    a->redo();
    m_stckUndo.push( a );

    if ( m_pDoc )
    {
	m_pDoc->enableUndo( hasUndoActions() );
	m_pDoc->enableRedo( hasRedoActions() );
    }
}

QString KSpreadUndo::getRedoName()
{
    if ( m_stckRedo.isEmpty() )
	return QString("");
    return  m_stckRedo.current()->getName();

}

QString KSpreadUndo::getUndoName()
{
    if ( m_stckUndo.isEmpty() )
	return QString("");
    return  m_stckUndo.current()->getName();
}

/****************************************************************************
 *
 * KSpreadMacroUndoAction
 *
 ***************************************************************************/
KSpreadMacroUndoAction::KSpreadMacroUndoAction( KSpreadDoc *_doc, const QString& _name ):
 KSpreadUndoAction( _doc )
{
    name=_name;
}

KSpreadMacroUndoAction::~KSpreadMacroUndoAction()
{
    m_commands.setAutoDelete( true );
}

void KSpreadMacroUndoAction::addCommand(KSpreadUndoAction *command)
{
    m_commands.append(command);
}

void KSpreadMacroUndoAction::undo()
{
    QPtrListIterator<KSpreadUndoAction> it(m_commands);
    for ( ; it.current() ; ++it )
        it.current()->undo();
}

void KSpreadMacroUndoAction::redo()
{
    QPtrListIterator<KSpreadUndoAction> it(m_commands);
    for ( ; it.current() ; ++it )
        it.current()->redo();
}

/****************************************************************************
 *
 * KSpreadUndoRemoveColumn
 *
 ***************************************************************************/

KSpreadUndoRemoveColumn::KSpreadUndoRemoveColumn( KSpreadDoc *_doc, KSpreadTable *_table, int _column,int _nbCol ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Remove Columns");
    m_tableName = _table->tableName();
    m_iColumn= _column;
    m_iNbCol=_nbCol;
    m_printRange=_table->printRange();
    m_printRepeatColumns = _table->printRepeatColumns();
    QRect selection;
    selection.setCoords( _column, 1, _column+m_iNbCol, KS_rowMax );
    QDomDocument doc = _table->saveCellRect( selection );

    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    m_data = buffer.utf8();
    int len = m_data.length();
    char tmp = m_data[ len - 1 ];
    m_data.resize( len );
    *( m_data.data() + len - 1 ) = tmp;
}

KSpreadUndoRemoveColumn::~KSpreadUndoRemoveColumn()
{
}

void KSpreadUndoRemoveColumn::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    table->insertColumn( m_iColumn,m_iNbCol);

    QPoint pastePoint( m_iColumn, 1 );
    table->paste( m_data, QRect( pastePoint, pastePoint ) );
    if(table->getAutoCalc()) table->recalc();

    table->setPrintRange( m_printRange );
    table->setPrintRepeatColumns( m_printRepeatColumns );

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoRemoveColumn::redo()
{
    doc()->undoBuffer()->lock();

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    table->removeColumn( m_iColumn,m_iNbCol );

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoInsertColumn
 *
 ***************************************************************************/

KSpreadUndoInsertColumn::KSpreadUndoInsertColumn( KSpreadDoc *_doc, KSpreadTable *_table, int _column, int _nbCol ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Insert Columns");
    m_tableName = _table->tableName();
    m_iColumn= _column;
    m_iNbCol=_nbCol;
}

KSpreadUndoInsertColumn::~KSpreadUndoInsertColumn()
{
}

void KSpreadUndoInsertColumn::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->removeColumn( m_iColumn,m_iNbCol );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoInsertColumn::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->insertColumn( m_iColumn,m_iNbCol);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoRemoveRow
 *
 ***************************************************************************/

KSpreadUndoRemoveRow::KSpreadUndoRemoveRow( KSpreadDoc *_doc, KSpreadTable *_table, int _row,int _nbRow) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Remove Rows");

    m_tableName = _table->tableName();
    m_iRow = _row;
    m_iNbRow=  _nbRow;
    m_printRange=_table->printRange();
    m_printRepeatRows = _table->printRepeatRows();

    QRect selection;
    selection.setCoords( 1, _row, KS_colMax, _row+m_iNbRow );
    QDomDocument doc = _table->saveCellRect( selection );

    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    m_data = buffer.utf8();
    int len = m_data.length();
    char tmp = m_data[ len - 1 ];
    m_data.resize( len );
    *( m_data.data() + len - 1 ) = tmp;

    // printf("UNDO {{{%s}}}\n", buffer.latin1() );
    // printf("UNDO2 %i bytes, length %i {{{%s}}}\n", m_data.length(), m_data.size(), (const char*)m_data );
    // printf("length=%i, size=%i", m_data.length(), m_data.size() );
    // printf("Last characters are %i %i %i\n", (int)m_data[ m_data.size() - 3 ],
    // (int)m_data[ m_data.size() - 2 ], (int)m_data[ m_data.size() - 1 ] );
}

KSpreadUndoRemoveRow::~KSpreadUndoRemoveRow()
{
}

void KSpreadUndoRemoveRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    table->insertRow( m_iRow,m_iNbRow );

    QPoint pastePoint( 1, m_iRow );
    table->paste( m_data, QRect(pastePoint, pastePoint) );

    table->setPrintRange( m_printRange );
    table->setPrintRepeatRows( m_printRepeatRows );

    if(table->getAutoCalc()) table->recalc();

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoRemoveRow::redo()
{
    doc()->undoBuffer()->lock();

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    table->removeRow( m_iRow,m_iNbRow );

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoInsertRow
 *
 ***************************************************************************/

KSpreadUndoInsertRow::KSpreadUndoInsertRow( KSpreadDoc *_doc, KSpreadTable *_table, int _row,int _nbRow ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Insert Rows");
    m_tableName = _table->tableName();
    m_iRow = _row;
    m_iNbRow=_nbRow;
}

KSpreadUndoInsertRow::~KSpreadUndoInsertRow()
{
}

void KSpreadUndoInsertRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->removeRow( m_iRow,m_iNbRow );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoInsertRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->insertRow( m_iRow,m_iNbRow );
    doc()->undoBuffer()->unlock();
}


/****************************************************************************
 *
 * KSpreadUndoHideRow
 *
 ***************************************************************************/

KSpreadUndoHideRow::KSpreadUndoHideRow( KSpreadDoc *_doc, KSpreadTable *_table, int _row, int _nbRow , QValueList<int>_listRow) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Hide Rows");
    m_tableName = _table->tableName();
    m_iRow= _row;
    m_iNbRow=_nbRow;
    if(m_iNbRow!=-1)
      createList( listRow ,_table );
    else
      listRow=QValueList<int>(_listRow);
}

KSpreadUndoHideRow::~KSpreadUndoHideRow()
{
}

void KSpreadUndoHideRow::createList( QValueList<int>&list,KSpreadTable *tab )
{
RowLayout *rl;
for(int i=m_iRow;i<=(m_iRow+m_iNbRow);i++)
        {
        rl= tab->nonDefaultRowLayout( i );
        if(!rl->isHide())
                list.append(rl->row());
        }
}

void KSpreadUndoHideRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->showRow( 0,-1,listRow );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoHideRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideRow(0,-1, listRow );
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoHideColumn
 *
 ***************************************************************************/

KSpreadUndoHideColumn::KSpreadUndoHideColumn( KSpreadDoc *_doc, KSpreadTable *_table, int _column, int _nbCol, QValueList<int>_listCol ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Hide Columns");

    m_tableName = _table->tableName();
    m_iColumn= _column;
    m_iNbCol=_nbCol;
    if(m_iNbCol!=-1)
      createList( listCol ,_table );
    else
      listCol=QValueList<int>(_listCol);
}

KSpreadUndoHideColumn::~KSpreadUndoHideColumn()
{
}

void KSpreadUndoHideColumn::createList( QValueList<int>&list,KSpreadTable *tab )
{
ColumnLayout *cl;
for(int i=m_iColumn;i<=(m_iColumn+m_iNbCol);i++)
  {
    cl= tab->nonDefaultColumnLayout( i );
    if(!cl->isHide())
      list.append(cl->column());
  }
}

void KSpreadUndoHideColumn::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->showColumn(0,-1,listCol);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoHideColumn::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideColumn(0,-1,listCol);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoShowRow
 *
 ***************************************************************************/

KSpreadUndoShowRow::KSpreadUndoShowRow( KSpreadDoc *_doc, KSpreadTable *_table, int _row, int _nbRow, QValueList<int>_listRow ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Show Rows");

    m_tableName = _table->tableName();
    m_iRow= _row;
    m_iNbRow=_nbRow;
    if(m_iNbRow!=-1)
      createList( listRow ,_table );
    else
      listRow=QValueList<int>(_listRow);
}

KSpreadUndoShowRow::~KSpreadUndoShowRow()
{
}

void KSpreadUndoShowRow::createList( QValueList<int>&list,KSpreadTable *tab )
{
RowLayout *rl;
for(int i=m_iRow;i<=(m_iRow+m_iNbRow);i++)
        {
        rl= tab->nonDefaultRowLayout( i );
        if(rl->isHide())
                list.append(rl->row());
        }
}

void KSpreadUndoShowRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideRow(0,-1,listRow);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoShowRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->showRow(0,-1,listRow);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoShowColumn
 *
 ***************************************************************************/

KSpreadUndoShowColumn::KSpreadUndoShowColumn( KSpreadDoc *_doc, KSpreadTable *_table, int _column, int _nbCol,QValueList<int>_listCol ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Show Columns");

    m_tableName = _table->tableName();
    m_iColumn= _column;
    m_iNbCol=_nbCol;
    if(m_iNbCol!=-1)
      createList( listCol ,_table );
    else
      listCol=QValueList<int>(_listCol);
}

KSpreadUndoShowColumn::~KSpreadUndoShowColumn()
{
}

void KSpreadUndoShowColumn::createList( QValueList<int>&list,KSpreadTable *tab )
{
ColumnLayout *cl;
for(int i=m_iColumn;i<=(m_iColumn+m_iNbCol);i++)
  {
    cl= tab->nonDefaultColumnLayout( i );
    if(cl->isHide())
      list.append(cl->column());
  }

}

void KSpreadUndoShowColumn::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideColumn( 0,-1,listCol );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoShowColumn::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->showColumn(0,-1,listCol);
    doc()->undoBuffer()->unlock();
}


/****************************************************************************
 *
 * KSpreadUndoPaperLayout
 *
 ***************************************************************************/

KSpreadUndoPaperLayout::KSpreadUndoPaperLayout( KSpreadDoc *_doc, KSpreadTable *_table )
    : KSpreadUndoAction( _doc )
{
    name=i18n("Set Paper Layout");
    m_tableName = _table->tableName();

    m_pl = _table->getPaperLayout();
    m_hf = _table->getHeadFootLine();
    m_unit = doc()->getUnit();
    m_printGrid = _table->getPrintGrid();
    m_printCommentIndicator = _table->getPrintCommentIndicator();
    m_printFormulaIndicator = _table->getPrintFormulaIndicator();
    m_printRange = _table->printRange();
    m_printRepeatColumns = _table->printRepeatColumns();
    m_printRepeatRows = _table->printRepeatRows();
}

KSpreadUndoPaperLayout::~KSpreadUndoPaperLayout()
{
}

void KSpreadUndoPaperLayout::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    m_plRedo = table->getPaperLayout();
    table->setPaperLayout( m_pl.ptLeft,  m_pl.ptTop,
                           m_pl.ptRight, m_pl.ptBottom,
                           m_pl.format,  m_pl.orientation );

    m_hfRedo = table->getHeadFootLine();
    table->setHeadFootLine( m_hf.headLeft, m_hf.headMid, m_hf.headRight,
                            m_hf.footLeft, m_hf.footMid, m_hf.footRight );

    m_unitRedo = doc()->getUnit();
    doc()->setUnit( m_unit );

    m_printGridRedo = table->getPrintGrid();
    table->setPrintGrid( m_printGrid );

    m_printCommentIndicatorRedo = table->getPrintCommentIndicator();
    table->setPrintCommentIndicator( m_printCommentIndicator );

    m_printFormulaIndicatorRedo = table->getPrintFormulaIndicator();
    table->setPrintFormulaIndicator( m_printFormulaIndicator );

    m_printRangeRedo = table->printRange();
    table->setPrintRange( m_printRange );

    m_printRepeatColumnsRedo = table->printRepeatColumns();
    table->setPrintRepeatColumns( m_printRepeatColumns );

    m_printRepeatRowsRedo = table->printRepeatRows();
    table->setPrintRepeatRows( m_printRepeatRows );

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoPaperLayout::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->setPaperLayout( m_plRedo.ptLeft,  m_plRedo.ptTop,
                           m_plRedo.ptRight, m_plRedo.ptBottom,
                           m_plRedo.format, m_plRedo.orientation );

    table->setHeadFootLine( m_hfRedo.headLeft, m_hfRedo.headMid, m_hfRedo.headRight,
                            m_hfRedo.footLeft, m_hfRedo.footMid, m_hfRedo.footRight );

    doc()->setUnit( m_unitRedo );

    table->setPrintGrid( m_printGridRedo );
    table->setPrintCommentIndicator( m_printCommentIndicatorRedo );
    table->setPrintFormulaIndicator( m_printFormulaIndicatorRedo );

    table->setPrintRange( m_printRangeRedo );
    table->setPrintRepeatColumns( m_printRepeatColumnsRedo );
    table->setPrintRepeatRows( m_printRepeatRowsRedo );

    doc()->undoBuffer()->unlock();
}


/****************************************************************************
 *
 * KSpreadUndoDefinePrintRange
 *
 ***************************************************************************/

KSpreadUndoDefinePrintRange::KSpreadUndoDefinePrintRange( KSpreadDoc *_doc, KSpreadTable *_table )
    : KSpreadUndoAction( _doc )
{
    name=i18n("Set Paper Layout");
    m_tableName = _table->tableName();

    m_printRange = _table->printRange();
}

KSpreadUndoDefinePrintRange::~KSpreadUndoDefinePrintRange()
{
}

void KSpreadUndoDefinePrintRange::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    m_printRangeRedo = table->printRange();
    table->setPrintRange( m_printRange );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoDefinePrintRange::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->setPrintRange( m_printRangeRedo );
    doc()->undoBuffer()->unlock();
}


/****************************************************************************
 *
 * KSpreadUndoSetText
 *
 ***************************************************************************/

KSpreadUndoSetText::KSpreadUndoSetText( KSpreadDoc *_doc, KSpreadTable *_table, const QString& _text, int _column, int _row,KSpreadCell::FormatType _formatType ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Change Text");

    m_strText = _text;
    m_iColumn= _column;
    m_iRow = _row;
    m_tableName = _table->tableName();
    m_eFormatType=_formatType;
}

KSpreadUndoSetText::~KSpreadUndoSetText()
{
}

void KSpreadUndoSetText::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();
    KSpreadCell *cell = table->nonDefaultCell( m_iColumn, m_iRow );
    m_strRedoText = cell->text();
    m_eFormatTypeRedo=cell->getFormatType( m_iColumn, m_iRow );
    cell->setFormatType(m_eFormatType);

    if ( m_strText.isNull() )
	cell->setCellText( "" );
    else
	cell->setCellText( m_strText );
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoSetText::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();
    KSpreadCell *cell = table->nonDefaultCell( m_iColumn, m_iRow );
    m_strText = cell->text();
    m_eFormatType=cell->getFormatType( m_iColumn, m_iRow );
    if ( m_strRedoText.isNull() )
	cell->setCellText( "" );
    else
	cell->setCellText( m_strRedoText );
    cell->setFormatType(m_eFormatTypeRedo);
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoSetTableName
 *
 ***************************************************************************/

KSpreadUndoSetTableName::KSpreadUndoSetTableName( KSpreadDoc *doc, KSpreadTable *table, const QString& _name ) :
    KSpreadUndoAction( doc )
{
    name=i18n("Change Table Name");

    m_name = _name;
    m_tableName = table->tableName();
}

KSpreadUndoSetTableName::~KSpreadUndoSetTableName()
{
}

void KSpreadUndoSetTableName::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    m_redoName = table->tableName();

    table->setTableName( m_name,false,false );

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoSetTableName::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_name );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    table->setTableName( m_redoName,false,false );

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoCellLayout
 *
 ***************************************************************************/

KSpreadUndoCellLayout::KSpreadUndoCellLayout( KSpreadDoc * _doc,
                                              KSpreadTable * _table,
                                              const QRect & _selection,
                                              const QString & _name ) :
  KSpreadUndoAction( _doc )
{
  if ( _name.isEmpty())
    name = i18n("Change Layout");
  else
    name = _name;

  m_rctRect   = _selection;
  m_tableName = _table->tableName();
  copyLayout( m_lstLayouts, m_lstColLayouts, m_lstRowLayouts, _table );
}

void KSpreadUndoCellLayout::copyLayout(QValueList<layoutCell> & list,
                                       QValueList<layoutColumn> & listCol,
                                       QValueList<layoutRow> & listRow,
                                       KSpreadTable * table )
{
    QValueList<layoutCell>::Iterator it2;
  for ( it2 = list.begin(); it2 != list.end(); ++it2 )
  {
      delete (*it2).l;
  }
  list.clear();

  KSpreadCell * cell;
  int bottom = m_rctRect.bottom();
  int right  = m_rctRect.right();

  if ( util_isColumnSelected( m_rctRect ) )
  {
    /* Don't need to go through the loop twice...
      for (int i = m_rctRect.left(); i <= right; ++i)
      {
      layoutColumn tmplayout;
      tmplayout.col = i;
      tmplayout.l = new ColumnLayout( table, i );
      tmplayout.l->copy( *(table->columnLayout( i )) );
      listCol.append(tmplayout);
      }
    */
    for ( int c = m_rctRect.left(); c <= right; ++c )
    {
      layoutColumn tmplayout;
      tmplayout.col = c;
      tmplayout.l = new ColumnLayout( table, c );
      tmplayout.l->copy( *(table->columnLayout( c )) );
      listCol.append(tmplayout);

      cell = table->getFirstCellColumn( c );
      while ( cell )
      {
        if ( cell->isObscuringForced() )
        {
          cell = table->getNextCellDown( c, cell->row() );
          continue;
        }

        layoutCell tmplayout;
        tmplayout.col = c;
        tmplayout.row = cell->row();
        tmplayout.l = new KSpreadLayout( table );
        tmplayout.l->copy( *(table->cellAt( tmplayout.col, tmplayout.row )) );
        list.append(tmplayout);

        cell = table->getNextCellDown( c, cell->row() );
      }
    }
    /*
      KSpreadCell * c = table->firstCell();
      for( ; c; c = c->nextCell() )
      {
      int col = c->column();
      if ( m_rctRect.left() <= col && right >= col
          && !c->isObscuringForced())
      {
        layoutCell tmplayout;
        tmplayout.col = c->column();
        tmplayout.row = c->row();
        tmplayout.l = new KSpreadLayout( table );
        tmplayout.l->copy( *(table->cellAt( tmplayout.col, tmplayout.row )) );
        list.append(tmplayout);
      }
      }
    */
  }
  else if (util_isRowSelected( m_rctRect ) )
  {
    for ( int row = m_rctRect.top(); row <= bottom; ++row )
    {
      layoutRow tmplayout;
      tmplayout.row = row;
      tmplayout.l = new RowLayout( table, row );
      tmplayout.l->copy( *(table->rowLayout( row )) );
      listRow.append(tmplayout);

      cell = table->getFirstCellRow( row );
      while ( cell )
      {
        if ( cell->isObscuringForced() )
        {
          cell = table->getNextCellRight( cell->column(), row );
          continue;
        }
        layoutCell tmplayout;
        tmplayout.col = cell->column();
        tmplayout.row = row;
        tmplayout.l = new KSpreadLayout( table );
        tmplayout.l->copy( *(table->cellAt( cell->column(), row )) );
        list.append(tmplayout);

        cell = table->getNextCellRight( cell->column(), row );
      }
    }
    /*
      KSpreadCell * c = table->firstCell();
      for( ; c; c = c->nextCell() )
      {
      int row = c->row();
      if ( m_rctRect.top() <= row && bottom >= row
           && !c->isObscuringForced())
      {
        layoutCell tmplayout;
        tmplayout.col = c->column();
        tmplayout.row = c->row();
        tmplayout.l = new KSpreadLayout( table );
        tmplayout.l->copy( *(table->cellAt( tmplayout.col, tmplayout.row )) );
        list.append(tmplayout);
      }
      }
    */
  }
  else
  {
    for ( int y = m_rctRect.top(); y <= bottom; ++y )
      for ( int x = m_rctRect.left(); x <= right; ++x )
      {
        KSpreadCell * cell = table->nonDefaultCell( x, y );
        if ( !cell->isObscuringForced() )
        {
          layoutCell tmplayout;
          tmplayout.col = x;
          tmplayout.row = y;
          tmplayout.l = new KSpreadLayout( table );
          tmplayout.l->copy( *(table->cellAt( x, y )) );
          list.append(tmplayout);
        }
      }
  }
}

KSpreadUndoCellLayout::~KSpreadUndoCellLayout()
{
    QValueList<layoutCell>::Iterator it2;
    for ( it2 = m_lstLayouts.begin(); it2 != m_lstLayouts.end(); ++it2 )
    {
        delete (*it2).l;
    }
    m_lstLayouts.clear();

    for ( it2 = m_lstRedoLayouts.begin(); it2 != m_lstRedoLayouts.end(); ++it2 )
    {
        delete (*it2).l;
    }
    m_lstRedoLayouts.clear();

    QValueList<layoutColumn>::Iterator it3;
    for ( it3 = m_lstColLayouts.begin(); it3 != m_lstColLayouts.end(); ++it3 )
    {
        delete (*it3).l;
    }
    m_lstColLayouts.clear();

    for ( it3 = m_lstRedoColLayouts.begin(); it3 != m_lstRedoColLayouts.end(); ++it3 )
    {
        delete (*it3).l;
    }
    m_lstRedoColLayouts.clear();

    QValueList<layoutRow>::Iterator it4;
    for ( it4 = m_lstRowLayouts.begin(); it4 != m_lstRowLayouts.end(); ++it4 )
    {
        delete (*it4).l;
    }
    m_lstRowLayouts.clear();

    for ( it4 = m_lstRedoRowLayouts.begin(); it4 != m_lstRedoRowLayouts.end(); ++it4 )
    {
        delete (*it4).l;
    }
    m_lstRedoRowLayouts.clear();


}

void KSpreadUndoCellLayout::undo()
{
  KSpreadTable * table = doc()->map()->findTable( m_tableName );
  if ( !table )
    return;

  doc()->undoBuffer()->lock();
  doc()->emitBeginOperation();
  copyLayout( m_lstRedoLayouts, m_lstRedoColLayouts, m_lstRedoRowLayouts, table );
  if( util_isColumnSelected( m_rctRect ) )
  {
    QValueList<layoutColumn>::Iterator it2;
    for ( it2 = m_lstColLayouts.begin(); it2 != m_lstColLayouts.end(); ++it2 )
    {
      ColumnLayout * col = table->nonDefaultColumnLayout( (*it2).col );
      col->copy( *(*it2).l );
    }
  }
  else if( util_isRowSelected( m_rctRect ) )
  {
    QValueList<layoutRow>::Iterator it2;
    for ( it2 = m_lstRowLayouts.begin(); it2 != m_lstRowLayouts.end(); ++it2 )
    {
      RowLayout * row = table->nonDefaultRowLayout( (*it2).row );
      row->copy( *(*it2).l );
    }
  }

  QValueList<layoutCell>::Iterator it2;
  for ( it2 = m_lstLayouts.begin(); it2 != m_lstLayouts.end(); ++it2 )
  {
    KSpreadCell *cell = table->nonDefaultCell( (*it2).col,(*it2).row );
    cell->copy( *(*it2).l );
    cell->setLayoutDirtyFlag();
    cell->setDisplayDirtyFlag();
    table->updateCell( cell, (*it2).col, (*it2).row );
  }

  table->updateView(m_rctRect);
  doc()->emitEndOperation();

  doc()->undoBuffer()->unlock();
}

void KSpreadUndoCellLayout::redo()
{
  KSpreadTable* table = doc()->map()->findTable( m_tableName );
  if ( !table )
    return;

  doc()->undoBuffer()->lock();
  doc()->emitBeginOperation();

  if ( util_isColumnSelected( m_rctRect ) )
  {
    QValueList<layoutColumn>::Iterator it2;
    for ( it2 = m_lstRedoColLayouts.begin(); it2 != m_lstRedoColLayouts.end(); ++it2 )
    {
      ColumnLayout * col = table->nonDefaultColumnLayout( (*it2).col );
      col->copy( *(*it2).l );
    }
  }
  else if( util_isRowSelected( m_rctRect ) )
  {
    QValueList<layoutRow>::Iterator it2;
    for ( it2 = m_lstRedoRowLayouts.begin(); it2 != m_lstRedoRowLayouts.end(); ++it2 )
    {
      RowLayout * row = table->nonDefaultRowLayout( (*it2).row );
      row->copy( *(*it2).l );
    }
  }

  QValueList<layoutCell>::Iterator it2;
  for ( it2 = m_lstRedoLayouts.begin(); it2 != m_lstRedoLayouts.end(); ++it2 )
  {
    KSpreadCell * cell = table->nonDefaultCell( (*it2).col,(*it2).row );
    cell->copy( *(*it2).l );
    cell->setLayoutDirtyFlag();
    cell->setDisplayDirtyFlag();
    table->updateCell( cell, (*it2).col, (*it2).row );
  }

  table->updateView( m_rctRect );
  doc()->emitEndOperation();
  doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoSort
 *
 ***************************************************************************/

KSpreadUndoSort::KSpreadUndoSort( KSpreadDoc * _doc, KSpreadTable * _table, const QRect & _selection ) :
    KSpreadUndoAction( _doc )
{
  name        = i18n("Sort");

  m_rctRect   = _selection;
  m_tableName = _table->tableName();
  copyAll( m_lstLayouts, m_lstColLayouts, m_lstRowLayouts, _table );
}

void KSpreadUndoSort::copyAll(QValueList<layoutTextCell> & list, QValueList<layoutColumn> & listCol,
                              QValueList<layoutRow> & listRow, KSpreadTable * table )
{
  QValueList<layoutTextCell>::Iterator it2;
  for ( it2 = list.begin(); it2 != list.end(); ++it2 )
  {
      delete (*it2).l;
  }
  list.clear();

  if ( util_isColumnSelected( m_rctRect ) )
  {
    KSpreadCell * c;
    for (int col = m_rctRect.left(); col <= m_rctRect.right(); ++col)
    {
      layoutColumn tmplayout;
      tmplayout.col = col;
      tmplayout.l = new ColumnLayout( table, col );
      tmplayout.l->copy( *(table->columnLayout( col )) );
      listCol.append(tmplayout);

      c = table->getFirstCellColumn( col );
      while ( c )
      {
        if ( !c->isObscuringForced() )
        {
          layoutTextCell tmplayout;
          tmplayout.col = col;
          tmplayout.row = c->row();
          tmplayout.l = new KSpreadLayout( table );
          tmplayout.l->copy( *(table->cellAt( tmplayout.col, tmplayout.row )) );
          tmplayout.text = c->text();
          list.append(tmplayout);
        }

        c = table->getNextCellDown( col, c->row() );
      }
    }
  }
  else if ( util_isRowSelected( m_rctRect ) )
  {
    KSpreadCell * c;
    for ( int row = m_rctRect.top(); row <= m_rctRect.bottom(); ++row)
    {
      layoutRow tmplayout;
      tmplayout.row = row;
      tmplayout.l = new RowLayout( table, row );
      tmplayout.l->copy( *(table->rowLayout( row )) );
      listRow.append(tmplayout);

      c = table->getFirstCellRow( row );
      while ( c )
      {
        if ( !c->isObscuringForced() )
        {
          layoutTextCell tmplayout;
          tmplayout.col = c->column();
          tmplayout.row = row;
          tmplayout.l   = new KSpreadLayout( table );
          tmplayout.l->copy( *(table->cellAt( tmplayout.col, tmplayout.row )) );
          tmplayout.text = c->text();
          list.append(tmplayout);
        }
        c = table->getNextCellRight( c->column(), row );
      }
    }
  }
  else
  {
    int bottom = m_rctRect.bottom();
    int right  = m_rctRect.right();
    KSpreadCell * cell;
    for ( int y = m_rctRect.top(); y <= bottom; ++y )
      for ( int x = m_rctRect.left(); x <= right; ++x )
      {
        cell = table->nonDefaultCell( x, y );
        if (!cell->isObscuringForced())
        {
          layoutTextCell tmplayout;
          tmplayout.col = x;
          tmplayout.row = y;
          tmplayout.l   = new KSpreadLayout( table );
          tmplayout.l->copy( *(table->cellAt( x, y )) );
          tmplayout.text = cell->text();
          list.append(tmplayout);
        }
      }
  }
}

KSpreadUndoSort::~KSpreadUndoSort()
{
    QValueList<layoutTextCell>::Iterator it2;
    for ( it2 = m_lstLayouts.begin(); it2 != m_lstLayouts.end(); ++it2 )
    {
        delete (*it2).l;
    }
    m_lstLayouts.clear();

    for ( it2 = m_lstRedoLayouts.begin(); it2 != m_lstRedoLayouts.end(); ++it2 )
    {
        delete (*it2).l;
    }
    m_lstRedoLayouts.clear();

    QValueList<layoutColumn>::Iterator it3;
    for ( it3 = m_lstColLayouts.begin(); it3 != m_lstColLayouts.end(); ++it3 )
    {
        delete (*it3).l;
    }
    m_lstColLayouts.clear();

    for ( it3 = m_lstRedoColLayouts.begin(); it3 != m_lstRedoColLayouts.end(); ++it3 )
    {
        delete (*it3).l;
    }
    m_lstRedoColLayouts.clear();

    QValueList<layoutRow>::Iterator it4;
    for ( it4 = m_lstRowLayouts.begin(); it4 != m_lstRowLayouts.end(); ++it4 )
    {
        delete (*it4).l;
    }
    m_lstRowLayouts.clear();

    for ( it4 = m_lstRedoRowLayouts.begin(); it4 != m_lstRedoRowLayouts.end(); ++it4 )
    {
        delete (*it4).l;
    }
    m_lstRedoRowLayouts.clear();

}

void KSpreadUndoSort::undo()
{
  KSpreadTable * table = doc()->map()->findTable( m_tableName );
  if ( !table )
    return;

  doc()->undoBuffer()->lock();
  doc()->emitBeginOperation();

  copyAll( m_lstRedoLayouts, m_lstRedoColLayouts,
           m_lstRedoRowLayouts, table );

  if ( util_isColumnSelected( m_rctRect ) )
  {
    QValueList<layoutColumn>::Iterator it2;
    for ( it2 = m_lstColLayouts.begin(); it2 != m_lstColLayouts.end(); ++it2 )
    {
      ColumnLayout * col = table->nonDefaultColumnLayout( (*it2).col );
      col->copy( *(*it2).l );
    }
  }
  else if( util_isRowSelected( m_rctRect ) )
  {
    QValueList<layoutRow>::Iterator it2;
    for ( it2 = m_lstRowLayouts.begin(); it2 != m_lstRowLayouts.end(); ++it2 )
    {
      RowLayout *row= table->nonDefaultRowLayout( (*it2).row );
      row->copy( *(*it2).l );
    }
  }

  QValueList<layoutTextCell>::Iterator it2;
  for ( it2 = m_lstLayouts.begin(); it2 != m_lstLayouts.end(); ++it2 )
  {
    KSpreadCell *cell = table->nonDefaultCell( (*it2).col,(*it2).row );
    if ( (*it2).text.isEmpty() )
    {
      if(!cell->text().isEmpty())
        cell->setCellText( "" );
    }
    else
      cell->setCellText( (*it2).text );

    cell->copy( *(*it2).l );
    cell->setLayoutDirtyFlag();
    cell->setDisplayDirtyFlag();
    table->updateCell( cell, (*it2).col, (*it2).row );
  }

  doc()->emitEndOperation();

  table->updateView(m_rctRect);

  doc()->undoBuffer()->unlock();
}

void KSpreadUndoSort::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    if( util_isColumnSelected( m_rctRect ) )
    {
      QValueList<layoutColumn>::Iterator it2;
      for ( it2 = m_lstRedoColLayouts.begin(); it2 != m_lstRedoColLayouts.end(); ++it2 )
      {
        ColumnLayout *col= table->nonDefaultColumnLayout( (*it2).col );
        col->copy( *(*it2).l );
      }
    }
    else if( util_isRowSelected( m_rctRect ) )
    {
      QValueList<layoutRow>::Iterator it2;
      for ( it2 = m_lstRedoRowLayouts.begin(); it2 != m_lstRedoRowLayouts.end(); ++it2 )
      {
        RowLayout *row= table->nonDefaultRowLayout( (*it2).row );
        row->copy( *(*it2).l );
      }
    }

    QValueList<layoutTextCell>::Iterator it2;
    for ( it2 = m_lstRedoLayouts.begin(); it2 != m_lstRedoLayouts.end(); ++it2 )
    {
      KSpreadCell *cell = table->nonDefaultCell( (*it2).col,(*it2).row );

      if ( (*it2).text.isEmpty() )
      {
        if(!cell->text().isEmpty())
          cell->setCellText( "" );
      }
      else
        cell->setCellText( (*it2).text );

      cell->copy( *(*it2).l );
      cell->setLayoutDirtyFlag();
      cell->setDisplayDirtyFlag();
      table->updateCell( cell, (*it2).col, (*it2).row );
    }
    doc()->emitEndOperation();
    table->updateView(m_rctRect);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoDelete
 *
 ***************************************************************************/

KSpreadUndoDelete::KSpreadUndoDelete( KSpreadDoc *_doc, KSpreadTable* table, const QRect & _selection)
    : KSpreadUndoAction( _doc )
{
    name=i18n("Delete");
    m_tableName = table->tableName();
    m_selection = _selection;
    createListCell( m_data, m_lstColumn,m_lstRow,table );

}

KSpreadUndoDelete::~KSpreadUndoDelete()
{
}

void KSpreadUndoDelete::createListCell( QCString &listCell,QValueList<columnSize> &listCol,QValueList<rowSize> &listRow, KSpreadTable* table )
{
    listRow.clear();
    listCol.clear();
    //copy a column(s)
    if( util_isColumnSelected( m_selection ) )
    {
        for( int y =m_selection.left() ; y <=m_selection.right() ; ++y )
        {
           ColumnLayout *cl=table->columnLayout(y);
           if(!cl->isDefault())
                {
                columnSize tmpSize;
                tmpSize.columnNumber=y;
                tmpSize.columnWidth=cl->width();
                listCol.append(tmpSize);
                }
        }
    }
    //copy a row(s)
    else if( util_isRowSelected( m_selection ) )
    {
        //save size of row(s)
        for( int y =m_selection.top() ; y <=m_selection.bottom() ; ++y )
        {
           RowLayout *rw=table->rowLayout(y);
           if(!rw->isDefault())
                {
                rowSize tmpSize;
                tmpSize.rowNumber=y;
                tmpSize.rowHeight=rw->height();
                listRow.append(tmpSize);
                }
        }

    }

    //save all cells in area
    QDomDocument doc = table->saveCellRect( m_selection );
    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    listCell = buffer.utf8();
    int len = listCell.length();
    char tmp = listCell[ len - 1 ];
    listCell.resize( len );
    *( listCell.data() + len - 1 ) = tmp;

}


void KSpreadUndoDelete::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;
    createListCell( m_dataRedo, m_lstRedoColumn,m_lstRedoRow,table );

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    if( util_isColumnSelected( m_selection ) )
    {
        QValueList<columnSize>::Iterator it2;
        for ( it2 = m_lstColumn.begin(); it2 != m_lstColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->nonDefaultColumnLayout((*it2).columnNumber);
           cl->setWidth(int((*it2).columnWidth));
        }
    }
    else if( util_isRowSelected( m_selection ) )
    {
        QValueList<rowSize>::Iterator it2;
        for ( it2 = m_lstRow.begin(); it2 != m_lstRow.end(); ++it2 )
        {
           RowLayout *rw=table->nonDefaultRowLayout((*it2).rowNumber);
           rw->setHeight(int((*it2).rowHeight));
        }
    }

    table->deleteCells( m_selection );
    table->paste( m_data, m_selection );
    doc()->emitEndOperation();

    if(table->getAutoCalc()) table->recalc();

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoDelete::redo()
{

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    if( util_isColumnSelected( m_selection ) )
    {
        QValueList<columnSize>::Iterator it2;
        for ( it2 = m_lstRedoColumn.begin(); it2 != m_lstRedoColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->nonDefaultColumnLayout((*it2).columnNumber);
           cl->setWidth(int((*it2).columnWidth));
        }
    }
    else if( util_isRowSelected( m_selection ) )
    {
        QValueList<rowSize>::Iterator it2;
        for ( it2 = m_lstRedoRow.begin(); it2 != m_lstRedoRow.end(); ++it2 )
        {
           RowLayout *rw=table->nonDefaultRowLayout((*it2).rowNumber);
           rw->setHeight(int((*it2).rowHeight));
        }
    }

    //move next line to refreshView
    //because I must know what is the real rect
    //that I must refresh, when there is cell Merged

    table->paste( m_dataRedo, m_selection );
    //table->deleteCells( m_selection );
    doc()->emitEndOperation();
    table->refreshView( m_selection );
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoResizeColRow
 *
 ***************************************************************************/


KSpreadUndoResizeColRow::KSpreadUndoResizeColRow( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &_selection ) :
    KSpreadUndoAction( _doc )
{
  name=i18n("Resize");
  m_rctRect = _selection;
  m_tableName = _table->tableName();

  createList( m_lstColumn,m_lstRow, _table );
}

void KSpreadUndoResizeColRow::createList( QValueList<columnSize> &listCol,QValueList<rowSize> &listRow, KSpreadTable* table )
{
    listCol.clear();
    listRow.clear();

    if( util_isColumnSelected( m_rctRect ) ) // entire column(s)
    {
    for( int y = m_rctRect.left(); y <= m_rctRect.right(); y++ )
        {
           ColumnLayout *cl=table->columnLayout(y);
	   if(!cl->isHide())
	     {
	       columnSize tmpSize;
	       tmpSize.columnNumber=y;
	       tmpSize.columnWidth=cl->width();
	       listCol.append(tmpSize);
	     }
        }
    }
    else if( util_isRowSelected( m_rctRect ) ) // entire row(s)
    {
    for( int y = m_rctRect.top(); y <= m_rctRect.bottom(); y++ )
        {
           RowLayout *rw=table->rowLayout(y);
	   if(!rw->isHide())
	     {
	       rowSize tmpSize;
	       tmpSize.rowNumber=y;
	       tmpSize.rowHeight=rw->height();
	       listRow.append(tmpSize);
	     }
        }
    }
    else //row and column
    {
    for( int y = m_rctRect.left(); y <= m_rctRect.right(); y++ )
        {
           ColumnLayout *cl=table->columnLayout(y);
	   if(!cl->isHide())
	     {
	       columnSize tmpSize;
	       tmpSize.columnNumber=y;
	       tmpSize.columnWidth=cl->width();
	       listCol.append(tmpSize);
	     }
        }
    for( int y = m_rctRect.top(); y <= m_rctRect.bottom(); y++ )
        {
           RowLayout *rw=table->rowLayout(y);
	   if(!rw->isHide())
	     {
	       rowSize tmpSize;
	       tmpSize.rowNumber=y;
	       tmpSize.rowHeight=rw->height();
	       listRow.append(tmpSize);
	     }
        }

    }
}

KSpreadUndoResizeColRow::~KSpreadUndoResizeColRow()
{
}

void KSpreadUndoResizeColRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    createList( m_lstRedoColumn,m_lstRedoRow, table );

    if( util_isColumnSelected( m_rctRect ) ) // complete column(s)
    {
    QValueList<columnSize>::Iterator it2;
    for ( it2 = m_lstColumn.begin(); it2 != m_lstColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->columnLayout((*it2).columnNumber);
           cl->setWidth((*it2).columnWidth);
        }
    }
    else if( util_isRowSelected( m_rctRect ) ) // complete row(s)
    {
    QValueList<rowSize>::Iterator it2;
    for ( it2 = m_lstRow.begin(); it2 != m_lstRow.end(); ++it2 )
        {
           RowLayout *rw=table->rowLayout((*it2).rowNumber);
           rw->setHeight((*it2).rowHeight);
        }
    }
    else // row and column
    {
    QValueList<columnSize>::Iterator it2;
    for ( it2 = m_lstColumn.begin(); it2 != m_lstColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->columnLayout((*it2).columnNumber);
           cl->setWidth((*it2).columnWidth);
        }
    QValueList<rowSize>::Iterator it1;
    for ( it1 = m_lstRow.begin(); it1 != m_lstRow.end(); ++it1 )
        {
           RowLayout *rw=table->rowLayout((*it1).rowNumber);
           rw->setHeight((*it1).rowHeight);
        }
    }

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoResizeColRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    if( util_isColumnSelected( m_rctRect ) ) // complete column(s)
    {
    QValueList<columnSize>::Iterator it2;
    for ( it2 = m_lstRedoColumn.begin(); it2 != m_lstRedoColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->columnLayout((*it2).columnNumber);
           cl->setWidth((*it2).columnWidth);
        }
    }
    else if( util_isRowSelected( m_rctRect ) ) // complete row(s)
    {
    QValueList<rowSize>::Iterator it2;
    for ( it2 = m_lstRedoRow.begin(); it2 != m_lstRedoRow.end(); ++it2 )
        {
           RowLayout *rw=table->rowLayout((*it2).rowNumber);
           rw->setHeight((*it2).rowHeight);
        }
    }
    else // row and column
    {
    QValueList<columnSize>::Iterator it2;
    for ( it2 = m_lstRedoColumn.begin(); it2 != m_lstRedoColumn.end(); ++it2 )
        {
           ColumnLayout *cl=table->columnLayout((*it2).columnNumber);
           cl->setWidth((*it2).columnWidth);
        }
    QValueList<rowSize>::Iterator it1;
    for ( it1 = m_lstRedoRow.begin(); it1 != m_lstRedoRow.end(); ++it1 )
        {
           RowLayout *rw=table->rowLayout((*it1).rowNumber);
           rw->setHeight((*it1).rowHeight);
        }
    }

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoChangeAreaTextCell
 *
 ***************************************************************************/


KSpreadUndoChangeAreaTextCell::KSpreadUndoChangeAreaTextCell( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &_selection ) :
    KSpreadUndoAction( _doc )
{
  name=i18n("Change Text");

  m_rctRect = _selection;
  m_tableName = _table->tableName();

  createList( m_lstTextCell, _table );
}

void KSpreadUndoChangeAreaTextCell::createList( QValueList<textOfCell> &list, KSpreadTable* table )
{
    int bottom = m_rctRect.bottom();
    int right  = m_rctRect.right();
    list.clear();

    if( util_isColumnSelected( m_rctRect ) )
    {
      KSpreadCell * c;
      for ( int col = m_rctRect.left(); col <= right; ++col )
      {
        c = table->getFirstCellColumn( col );
        while ( c )
        {
          if ( !c->isObscuringForced() )
          {
            textOfCell tmpText;
            tmpText.col = col;
            tmpText.row = c->row();
            tmpText.text = c->text();
            list.append(tmpText);
          }
          c = table->getNextCellDown( col, c->row() );
        }
      }
    }
    else if ( util_isRowSelected( m_rctRect ) )
    {
      KSpreadCell * c;
      for ( int row = m_rctRect.top(); row <= bottom; ++row )
      {
        c = table->getFirstCellRow( row );
        while ( c )
        {
          if ( !c->isObscuringForced() )
          {
            textOfCell tmpText;
            tmpText.col = c->column();
            tmpText.row = row;
            tmpText.text = c->text();
            list.append(tmpText);
          }
          c = table->getNextCellRight( c->column(), row );
        }
      }
    }
    else
    {
      KSpreadCell * cell;
      for ( int y = m_rctRect.top(); y <= bottom; ++y )
        for ( int x = m_rctRect.left(); x <= right; ++x )
        {
          cell = table->nonDefaultCell( x, y );
          if ( !cell->isObscured() )
          {
            textOfCell tmpText;
            tmpText.col = x;
            tmpText.row = y;
            tmpText.text = cell->text();
            list.append(tmpText);
          }
        }
    }
}

KSpreadUndoChangeAreaTextCell::~KSpreadUndoChangeAreaTextCell()
{
}

void KSpreadUndoChangeAreaTextCell::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();
    createList( m_lstRedoTextCell, table );


    QValueList<textOfCell>::Iterator it2;
    for ( it2 = m_lstTextCell.begin(); it2 != m_lstTextCell.end(); ++it2 )
    {
        KSpreadCell *cell = table->nonDefaultCell( (*it2).col, (*it2).row );
        if ( (*it2).text.isEmpty() )
                {
                if(!cell->text().isEmpty())
	                cell->setCellText( "" );
                }
        else
	       cell->setCellText( (*it2).text );
    }
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoChangeAreaTextCell::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();
    QValueList<textOfCell>::Iterator it2;
    for ( it2 = m_lstRedoTextCell.begin(); it2 != m_lstRedoTextCell.end(); ++it2 )
    {
        KSpreadCell *cell = table->nonDefaultCell( (*it2).col, (*it2).row );
        if ( (*it2).text.isEmpty() )
                {
                if(!cell->text().isEmpty())
	                cell->setCellText( "" );
                }
        else
	       cell->setCellText( (*it2).text );
    }
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoMergedCell
 *
 ***************************************************************************/


KSpreadUndoMergedCell::KSpreadUndoMergedCell( KSpreadDoc *_doc, KSpreadTable *_table, int _column, int _row , int _extraX,int _extraY) :
    KSpreadUndoAction( _doc )
{
  name=i18n("Merge Cells");

  m_tableName = _table->tableName();
  m_iRow=_row;
  m_iCol=_column;
  m_iExtraX=_extraX;
  m_iExtraY=_extraY;

}

KSpreadUndoMergedCell::~KSpreadUndoMergedCell()
{
}

void KSpreadUndoMergedCell::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    KSpreadCell *cell = table->nonDefaultCell( m_iCol, m_iRow );
    m_iExtraRedoX=cell->extraXCells();
    m_iExtraRedoY=cell->extraYCells();

    table->changeMergedCell( m_iCol, m_iRow, m_iExtraX,m_iExtraY);

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoMergedCell::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();

    table->changeMergedCell( m_iCol, m_iRow, m_iExtraRedoX,m_iExtraRedoY);

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoAutofill
 *
 ***************************************************************************/

KSpreadUndoAutofill::KSpreadUndoAutofill( KSpreadDoc *_doc, KSpreadTable* table, const QRect & _selection)
    : KSpreadUndoAction( _doc )
{
    name=i18n("Autofill");

    m_tableName = table->tableName();
    m_selection = _selection;
    createListCell( m_data, table );

}

KSpreadUndoAutofill::~KSpreadUndoAutofill()
{
}

void KSpreadUndoAutofill::createListCell( QCString &list, KSpreadTable* table )
{
    QDomDocument doc = table->saveCellRect( m_selection );
    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    list = buffer.utf8();
    int len = list.length();
    char tmp = list[ len - 1 ];
    list.resize( len );
    *( list.data() + len - 1 ) = tmp;
}

void KSpreadUndoAutofill::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    createListCell( m_dataRedo, table );

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    table->deleteCells( m_selection );
    table->paste( m_data, m_selection );
    if(table->getAutoCalc()) table->recalc();

    doc()->emitEndOperation();

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoAutofill::redo()
{
    doc()->undoBuffer()->lock();

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->emitBeginOperation();

    table->deleteCells( m_selection );
    doc()->undoBuffer()->lock();
    table->paste( m_dataRedo, m_selection );
    if(table->getAutoCalc()) table->recalc();
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoInsertCellRow
 *
 ***************************************************************************/

KSpreadUndoInsertCellRow::KSpreadUndoInsertCellRow( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &_rect ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Insert Cell");

    m_tableName = _table->tableName();
    m_rect=_rect;
}

KSpreadUndoInsertCellRow::~KSpreadUndoInsertCellRow()
{
}

void KSpreadUndoInsertCellRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->unshiftRow( m_rect);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoInsertCellRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->shiftRow( m_rect);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoInsertCellCol
 *
 ***************************************************************************/


KSpreadUndoInsertCellCol::KSpreadUndoInsertCellCol( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &_rect ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Insert Cell");

    m_tableName = _table->tableName();
    m_rect=_rect;
}

KSpreadUndoInsertCellCol::~KSpreadUndoInsertCellCol()
{
}

void KSpreadUndoInsertCellCol::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->unshiftColumn( m_rect);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoInsertCellCol::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->shiftColumn( m_rect );
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoRemoveCellRow
 *
 ***************************************************************************/

KSpreadUndoRemoveCellRow::KSpreadUndoRemoveCellRow( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &rect ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Remove Cell");

    m_tableName = _table->tableName();
    m_rect=rect;
    QDomDocument doc = _table->saveCellRect( m_rect );
    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    m_data = buffer.utf8();
    int len = m_data.length();
    char tmp = m_data[ len - 1 ];
    m_data.resize( len );
    *( m_data.data() + len - 1 ) = tmp;
}

KSpreadUndoRemoveCellRow::~KSpreadUndoRemoveCellRow()
{
}

void KSpreadUndoRemoveCellRow::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->shiftRow( m_rect );
    table->paste( m_data, m_rect );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoRemoveCellRow::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->unshiftRow( m_rect);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoRemoveCellCol
 *
 ***************************************************************************/

KSpreadUndoRemoveCellCol::KSpreadUndoRemoveCellCol( KSpreadDoc *_doc, KSpreadTable *_table, const QRect &_rect ) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Remove Cell");

    m_tableName = _table->tableName();
    m_rect=_rect;
    QDomDocument doc = _table->saveCellRect( m_rect );
    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    m_data = buffer.utf8();
    int len = m_data.length();
    char tmp = m_data[ len - 1 ];
    m_data.resize( len );
    *( m_data.data() + len - 1 ) = tmp;
}

KSpreadUndoRemoveCellCol::~KSpreadUndoRemoveCellCol()
{
}

void KSpreadUndoRemoveCellCol::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->shiftColumn( m_rect );
    table->paste( m_data, m_rect );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoRemoveCellCol::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->unshiftColumn( m_rect );
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoConditional
 *
 ***************************************************************************/

KSpreadUndoConditional::KSpreadUndoConditional( KSpreadDoc *_doc, KSpreadTable* table, QRect const & _selection)
    : KSpreadUndoAction( _doc )
{
    name=i18n("Conditional Cell Attribute");

    m_tableName = table->tableName();
    m_selection = _selection;
    createListCell( m_data, table );

}

KSpreadUndoConditional::~KSpreadUndoConditional()
{
}

void KSpreadUndoConditional::createListCell( QCString &list, KSpreadTable* table )
{
    QDomDocument doc = table->saveCellRect( m_selection );
    // Save to buffer
    QString buffer;
    QTextStream str( &buffer, IO_WriteOnly );
    str << doc;

    // This is a terrible hack to store unicode
    // data in a QCString in a way that
    // QCString::length() == QCString().size().
    // This allows us to treat the QCString like a QByteArray later on.
    list = buffer.utf8();
    int len = list.length();
    char tmp = list[ len - 1 ];
    list.resize( len );
    *( list.data() + len - 1 ) = tmp;
}

void KSpreadUndoConditional::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    createListCell( m_dataRedo, table );

    doc()->undoBuffer()->lock();
    table->paste( m_data, m_selection );
    if(table->getAutoCalc()) table->recalc();

    doc()->undoBuffer()->unlock();
}

void KSpreadUndoConditional::redo()
{
    doc()->undoBuffer()->lock();

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->paste( m_dataRedo, m_selection );
    if(table->getAutoCalc()) table->recalc();

    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoHideTable
 *
 ***************************************************************************/

KSpreadUndoHideTable::KSpreadUndoHideTable( KSpreadDoc *_doc, KSpreadTable *_table) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Hide Table");

    m_tableName = _table->tableName();
}

KSpreadUndoHideTable::~KSpreadUndoHideTable()
{
}

void KSpreadUndoHideTable::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideTable(false);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoHideTable::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideTable(true);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoShowTable
 *
 ***************************************************************************/


KSpreadUndoShowTable::KSpreadUndoShowTable( KSpreadDoc *_doc, KSpreadTable *_table) :
    KSpreadUndoAction( _doc )
{
    name=i18n("Show Table");

    m_tableName = _table->tableName();
}

KSpreadUndoShowTable::~KSpreadUndoShowTable()
{
}

void KSpreadUndoShowTable::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideTable(true);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoShowTable::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    table->hideTable(false);
    doc()->undoBuffer()->unlock();
}

/****************************************************************************
 *
 * KSpreadUndoCellPaste
 *
 ***************************************************************************/

KSpreadUndoCellPaste::KSpreadUndoCellPaste( KSpreadDoc *_doc, KSpreadTable* table, int _nbCol,int _nbRow, int _xshift,int _yshift, QRect &_selection,bool insert,int _insertTo )
    : KSpreadUndoAction( _doc )
{
    if(!insert)
        name=i18n("Paste");
    else
        name=i18n("Paste and Insert");

    m_tableName = table->tableName();
    m_selection = _selection;
    nbCol=_nbCol;
    nbRow=_nbRow;
    xshift=_xshift;
    yshift=_yshift;
    b_insert=insert;
    m_iInsertTo=_insertTo;
    if( !b_insert)
        createListCell( m_data, m_lstColumn,m_lstRow,table );

}

KSpreadUndoCellPaste::~KSpreadUndoCellPaste()
{
}

void KSpreadUndoCellPaste::createListCell( QCString &listCell,QValueList<columnSize> &listCol,QValueList<rowSize> &listRow, KSpreadTable* table )
{
    listCol.clear();
    listRow.clear();
    //copy a column(s)
    if(nbCol!=0)
    {
        //save all cells
        QRect rect;
        rect.setCoords( xshift, 1, xshift+nbCol, KS_rowMax );
        QDomDocument doc = table->saveCellRect( rect);
        // Save to buffer
        QString buffer;
        QTextStream str( &buffer, IO_WriteOnly );
        str << doc;

        // This is a terrible hack to store unicode
        // data in a QCString in a way that
        // QCString::length() == QCString().size().
        // This allows us to treat the QCString like a QByteArray later on.
        listCell = buffer.utf8();
        int len = listCell.length();
        char tmp = listCell[ len - 1 ];
        listCell.resize( len );
        *( listCell.data() + len - 1 ) = tmp;

        //save size of columns
        for( int y = 1; y <=nbCol ; ++y )
        {
           ColumnLayout *cl=table->columnLayout(y);
           if(!cl->isDefault())
                {
                columnSize tmpSize;
                tmpSize.columnNumber=y;
                tmpSize.columnWidth=cl->width();
                listCol.append(tmpSize);
                }
        }
    }
    //copy a row(s)
    else if(nbRow!=0)
    {
        //save all cells
        QRect rect;
        rect.setCoords( 1, yshift, KS_colMax, yshift+nbRow );
        QDomDocument doc = table->saveCellRect( rect);
        // Save to buffer
        QString buffer;
        QTextStream str( &buffer, IO_WriteOnly );
        str << doc;

        // This is a terrible hack to store unicode
        // data in a QCString in a way that
        // QCString::length() == QCString().size().
        // This allows us to treat the QCString like a QByteArray later on.
        listCell = buffer.utf8();
        int len = listCell.length();
        char tmp = listCell[ len - 1 ];
        listCell.resize( len );
        *( listCell.data() + len - 1 ) = tmp;

        //save size of columns
        for( int y = 1; y <=nbRow ; ++y )
        {
           RowLayout *rw=table->rowLayout(y);
           if(!rw->isDefault())
                {
                rowSize tmpSize;
                tmpSize.rowNumber=y;
                tmpSize.rowHeight=rw->height();
                listRow.append(tmpSize);
                }
        }

    }
    //copy just an area
    else
    {
        //save all cells in area
        QDomDocument doc = table->saveCellRect( m_selection );
        // Save to buffer
        QString buffer;
        QTextStream str( &buffer, IO_WriteOnly );
        str << doc;

        // This is a terrible hack to store unicode
        // data in a QCString in a way that
        // QCString::length() == QCString().size().
        // This allows us to treat the QCString like a QByteArray later on.
        listCell = buffer.utf8();
        int len = listCell.length();
        char tmp = listCell[ len - 1 ];
        listCell.resize( len );
        *( listCell.data() + len - 1 ) = tmp;
    }
}

void KSpreadUndoCellPaste::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    createListCell( m_dataRedo, m_lstRedoColumn,m_lstRedoRow,table );

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    if(nbCol!=0)
    {
        if(!b_insert)
                {
                QRect rect;
                rect.setCoords( xshift, 1, xshift+nbCol, KS_rowMax );
                table->deleteCells( rect );
                QPoint pastePoint(xshift, 1);
                table->paste( m_data, QRect(pastePoint, pastePoint) );
                QValueList<columnSize>::Iterator it2;
                for ( it2 = m_lstColumn.begin(); it2 != m_lstColumn.end(); ++it2 )
                        {
                        ColumnLayout *cl=table->nonDefaultColumnLayout((*it2).columnNumber);
                        cl->setWidth(int((*it2).columnWidth));
                        }
                }
        else
                {
                table->removeColumn( xshift+1,nbCol-1,false);
                }
    }
    else if(nbRow!=0)
    {
        if(!b_insert)
                {
                QRect rect;
                rect.setCoords( 1, yshift, KS_colMax, yshift+nbRow );
                table->deleteCells( rect );

                QPoint pastePoint(1, yshift);
                table->paste( m_data, QRect(pastePoint, pastePoint) );
                QValueList<rowSize>::Iterator it2;
                for ( it2 = m_lstRow.begin(); it2 != m_lstRow.end(); ++it2 )
                        {
                        RowLayout *rw=table->nonDefaultRowLayout((*it2).rowNumber);
                        rw->setHeight(int((*it2).rowHeight));
                        }
                }
        else
                {
                table->removeRow(  yshift+1,nbRow-1);
                }
    }
    else
    {
    if(!b_insert)
        {
        table->deleteCells( m_selection );
        table->paste( m_data, m_selection );
        }
    else
        {
        if(m_iInsertTo==-1)
                table->unshiftRow(m_selection);
        else if(m_iInsertTo==1)
                table->unshiftColumn(m_selection);
        }
    }

    if(table->getAutoCalc())
        table->recalc();
    doc()->emitEndOperation();
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoCellPaste::redo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    if(nbCol!=0)
    {
        if( b_insert)
                {
                table->insertColumn(  xshift+1,nbCol-1,false);
                }
        QRect rect;
        rect.setCoords( xshift, 1, xshift+nbCol, KS_rowMax );
        table->deleteCells( rect );
        QPoint pastePoint(xshift, 1);
        table->paste( m_dataRedo, QRect(pastePoint, pastePoint) );
        QValueList<columnSize>::Iterator it2;
         for ( it2 = m_lstRedoColumn.begin(); it2 != m_lstRedoColumn.end(); ++it2 )
                {
                ColumnLayout *cl=table->nonDefaultColumnLayout((*it2).columnNumber);
                cl->setWidth(int((*it2).columnWidth));
                }

    }
    else if(nbRow!=0)
    {
        if( b_insert)
                {
                table->insertRow(  yshift+1,nbRow-1);
                }

        QRect rect;
        rect.setCoords( 1, yshift, KS_colMax, yshift+nbRow );
        table->deleteCells( rect );

        QPoint pastePoint(1, yshift);
        table->paste( m_dataRedo, QRect(pastePoint, pastePoint) );
        QValueList<rowSize>::Iterator it2;
        for ( it2 = m_lstRedoRow.begin(); it2 != m_lstRedoRow.end(); ++it2 )
                {
                RowLayout *rw=table->nonDefaultRowLayout((*it2).rowNumber);
                 rw->setHeight(int((*it2).rowHeight));
                 }
    }
    else
    {
    if(b_insert)
        {
        if(m_iInsertTo==-1)
                table->shiftRow(m_selection);
        else if(m_iInsertTo==1)
                table->shiftColumn(m_selection);

        }
    table->deleteCells( m_selection );
    table->paste( m_dataRedo,m_selection );
    }
    if(table->getAutoCalc())
        table->recalc();

    doc()->emitEndOperation();

    doc()->undoBuffer()->unlock();
}


/****************************************************************************
 *
 * KSpreadUndoStyleCell
 *
 ***************************************************************************/

KSpreadUndoStyleCell::KSpreadUndoStyleCell( KSpreadDoc *_doc, KSpreadTable* table, const QRect & _selection)
    : KSpreadUndoAction( _doc )
{
    name=i18n("Style of Cell");

    m_tableName = table->tableName();
    m_selection = _selection;
    createListCell( m_lstStyleCell, table );

}

KSpreadUndoStyleCell::~KSpreadUndoStyleCell()
{
}

void KSpreadUndoStyleCell::createListCell( QValueList<styleCell> &listCell, KSpreadTable* table )
{
  int bottom = m_selection.bottom();
  int right  = m_selection.right();
  if ( util_isColumnSelected( m_selection ) )
  {
    KSpreadCell * c;
    for ( int col = m_selection.left(); col <= right; ++ col )
    {
      c = table->getFirstCellColumn( col );
      while ( c )
      {
        if ( !c->isObscuringForced() )
        {
	  styleCell tmpStyleCell;
	  tmpStyleCell.row = c->row();
	  tmpStyleCell.col = col;
	  tmpStyleCell.style = c->style();
	  tmpStyleCell.action = c->action();
	  listCell.append(tmpStyleCell);
        }
        c = table->getNextCellDown( col, c->row() );
      }
    }
  }
  else if ( util_isRowSelected( m_selection ) )
  {
    KSpreadCell * c;
    for ( int row = m_selection.top(); row <= bottom; ++row )
    {
      c = table->getFirstCellRow( row );
      while ( c )
      {
        if ( !c->isObscuringForced() )
        {
	  styleCell tmpStyleCell;
	  tmpStyleCell.row = row;
	  tmpStyleCell.col = c->column();
	  tmpStyleCell.style = c->style();
	  tmpStyleCell.action = c->action();
	  listCell.append(tmpStyleCell);
        }
        c = table->getNextCellRight( c->column(), row );
      }
    }
  }
  else
  {
    KSpreadCell * cell;
    for ( int i = m_selection.top(); i <= bottom; ++i)
	for ( int j = m_selection.left(); j <= right; ++j )
        {
          cell = table->nonDefaultCell( j, i);
          styleCell tmpStyleCell;
          tmpStyleCell.row = i;
          tmpStyleCell.col = j;
          tmpStyleCell.style = cell->style();
          tmpStyleCell.action = cell->action();
          listCell.append(tmpStyleCell);
        }
  }
}

void KSpreadUndoStyleCell::undo()
{
    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    createListCell( m_lstRedoStyleCell, table );

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();


    QValueList<styleCell>::Iterator it2;
    for ( it2 = m_lstStyleCell.begin(); it2 != m_lstStyleCell.end(); ++it2 )
      {
	KSpreadCell *cell = table->nonDefaultCell( (*it2).col, (*it2).row);
	cell->setStyle((*it2).style);
	cell->setAction((*it2).action);
      }
    doc()->emitEndOperation();
    table->updateView(m_selection);
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoStyleCell::redo()
{
    doc()->undoBuffer()->lock();

    KSpreadTable* table = doc()->map()->findTable( m_tableName );
    if ( !table )
	return;

    doc()->undoBuffer()->lock();
    doc()->emitBeginOperation();

    QValueList<styleCell>::Iterator it2;
    for ( it2 = m_lstRedoStyleCell.begin(); it2 != m_lstRedoStyleCell.end(); ++it2 )
      {
	KSpreadCell *cell = table->nonDefaultCell( (*it2).col, (*it2).row);
	cell->setStyle((*it2).style);
	cell->setAction((*it2).action);
      }
    doc()->emitEndOperation();
    table->updateView(m_selection);

    doc()->undoBuffer()->unlock();
}



KSpreadUndoAddTable::KSpreadUndoAddTable(KSpreadDoc *_doc, KSpreadTable* _table)
    : KSpreadUndoAction( _doc ),
      m_table( _table )
{
    name=i18n("Add Table");
}

KSpreadUndoAddTable::~KSpreadUndoAddTable()
{
}

void KSpreadUndoAddTable::undo()
{
    doc()->undoBuffer()->lock();
    m_table->map()->takeTable( m_table );
    doc()->takeTable( m_table );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoAddTable::redo()
{
    doc()->undoBuffer()->lock();
    m_table->map()->insertTable( m_table );
    doc()->insertTable( m_table );
    doc()->undoBuffer()->unlock();
}

KSpreadUndoRemoveTable::KSpreadUndoRemoveTable(KSpreadDoc *_doc, KSpreadTable* _table)
    : KSpreadUndoAction( _doc ),
      m_table( _table )
{
    name=i18n("Remove Table");
}

KSpreadUndoRemoveTable::~KSpreadUndoRemoveTable()
{
}

void KSpreadUndoRemoveTable::undo()
{
    doc()->undoBuffer()->lock();
    m_table->map()->insertTable( m_table );
    doc()->insertTable( m_table );
    doc()->undoBuffer()->unlock();
}

void KSpreadUndoRemoveTable::redo()
{
    doc()->undoBuffer()->lock();
    m_table->map()->takeTable( m_table );
    doc()->takeTable( m_table );
    doc()->undoBuffer()->unlock();
}

KSpreadUndoInsertData::KSpreadUndoInsertData( KSpreadDoc * _doc, KSpreadTable * _table, QRect & _selection )
    : KSpreadUndoChangeAreaTextCell( _doc, _table, _selection )
{
    name = i18n("Insert Data From Database");
}
