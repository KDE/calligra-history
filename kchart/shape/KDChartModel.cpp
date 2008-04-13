/* This file is part of the KDE project

   Copyright 2008 Johannes Simon <johannes.simon@gmail.com>

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

// Local
#include "KDChartModel.h"
#include "DataSet.h"

// KDE
#include <KDebug>

using namespace KChart;

class KDChartModel::Private {
public:
    Private();

    int dataDimensions;
    QList<DataSet*> dataSets;
};

KDChartModel::Private::Private()
{
    dataDimensions = 1;
}

KDChartModel::KDChartModel( QObject *parent /* = 0 */ )
    : QAbstractItemModel( parent ),
      d( new Private )
{
}

KDChartModel::~KDChartModel()
{
}

QVariant KDChartModel::data( const QModelIndex &index,
                             int role /* = Qt::DisplayRole */ ) const
{
    if ( role != Qt::DisplayRole )
        return QVariant();
    
    int row = index.row();
    int column = index.column();
    int dataSetNumber = column / d->dataDimensions;
    DataSet *dataSet = d->dataSets[ dataSetNumber ];
    
    if ( d->dataDimensions == 1 )
        return dataSet->yData( row );
    else if ( d->dataDimensions == 2 )
    {
        if ( column % 2 == 0 )
            return dataSet->xData( row );
        else
            return dataSet->yData( row );
    }
    // TODO (Johannes): Support for third data dimension
    
    // Should never happen
    return QVariant();
}

void KDChartModel::dataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    // TODO (Johannes): Emit dataChanged() for all datasets that have references to these rows
    emit QAbstractItemModel::dataChanged( topLeft, bottomRight );
}

void KDChartModel::slotColumnsInserted( const QModelIndex& parent, int start, int end )
{
    beginInsertRows( parent, start, end );
    endInsertRows();
}

QVariant KDChartModel::headerData( int section,
                                   Qt::Orientation orientation,
                                   int role /* = Qt::DisplayRole */ ) const
{
    if ( role != Qt::DisplayRole )
        return QVariant();
    
    if ( orientation == Qt::Vertical )
        return QVariant();
    
    if ( section >= columnCount() )
    {
        qWarning() << "KDChartModel::headerData(): Attempting to request header data for non-existant data set!";
        return QVariant();
    }
    
    int dataSetNumber = section / d->dataDimensions;
    DataSet *dataSet = d->dataSets[ dataSetNumber ];
    
    return dataSet->labelData();
}

QModelIndex KDChartModel::index( int row, int column, const QModelIndex &parent ) const
{
    return createIndex( row, column, 0 );
}

QModelIndex KDChartModel::parent( const QModelIndex &index ) const
{
    return QModelIndex();
}

int KDChartModel::rowCount( const QModelIndex &parent /* = QModelIndex() */ ) const
{
    return d->dataSets.isEmpty() ? 0 : d->dataSets[0]->size();
}

int KDChartModel::columnCount( const QModelIndex &parent /* = QModelIndex() */ ) const
{
    return d->dataSets.size() * d->dataDimensions;
}

void KDChartModel::setDataDimensions( int dataDimensions )
{
    d->dataDimensions = dataDimensions;
}

void KDChartModel::addDataSet( DataSet *dataSet )
{
    if ( d->dataSets.contains( dataSet ) )
    {
        qWarning() << "KDChartModel::addDataSet(): Attempting to insert already-contained data set";
        return;
    }
    if ( !d->dataSets.isEmpty() )
    {
        int columnToBeInserted = columnCount();
        beginInsertColumns( QModelIndex(), columnToBeInserted, columnToBeInserted + d->dataDimensions - 1 );
        d->dataSets.append( dataSet );
        endInsertColumns();
    }
    else
    {
        // If we had no datasets before, we haven't had a valid structure yet
        // Thus, emit the modelReset() signal
        d->dataSets.append( dataSet );
        reset();
    }
    
    dataSet->registerKdChartModel( this );
}

void KDChartModel::removeDataSet( DataSet *dataSet )
{
    if ( !d->dataSets.contains( dataSet ) )
        return;
    
    int columnAboutToBeRemoved = d->dataSets.indexOf( dataSet ) * d->dataDimensions;
    beginRemoveColumns( QModelIndex(), columnAboutToBeRemoved, columnAboutToBeRemoved + d->dataDimensions - 1 );
    d->dataSets.removeAll( dataSet );
    endRemoveColumns();
    
    dataSet->deregisterKdChartModel( this );
}

QList<DataSet*> KDChartModel::dataSets() const
{
    return d->dataSets;
}

#include "KDChartModel.moc"
