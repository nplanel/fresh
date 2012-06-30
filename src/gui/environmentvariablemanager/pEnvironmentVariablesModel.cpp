/****************************************************************************
**
** 		Created using Monkey Studio IDE v1.8.4.0 (1.8.4.0)
** Authors   : Filipe AZEVEDO aka Nox P@sNox <pasnox@gmail.com>
** Project   : Fresh Library
** FileName  : pEnvironmentVariablesModel.cpp
** Date      : 2011-02-20T00:41:09
** License   : LGPL v3
** Home Page : http://bettercodes.org/projects/fresh
** Comment   : Fresh Library is a Qt 4 extension library providing set of new core & gui classes.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Leser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This package is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "pEnvironmentVariablesModel.h"

#include <QFont>
#include <QProcess>

pEnvironmentVariablesModel::Variable::Variable( const QString& _name, const QString& _value, bool _enabled )
{
	name = _name;
	value = _value;
	enabled = _enabled;
}

pEnvironmentVariablesModel::pEnvironmentVariablesModel( QObject* parent )
	: QAbstractItemModel( parent )
{
	mRowCount = 0;
}

int pEnvironmentVariablesModel::columnCount( const QModelIndex& parent ) const
{
	return parent.isValid() ? 0 : 2;
}

QVariant pEnvironmentVariablesModel::data( const QModelIndex& index, int role ) const
{
	if ( !index.isValid() || index.row() < 0 || index.row() >= mRowCount || index.column() < 0 || index.column() >= 2 ) {
		return QVariant();
	}

	pEnvironmentVariablesModel::Variable* variable = static_cast<pEnvironmentVariablesModel::Variable*>( index.internalPointer() );

	switch ( role ) {
		case Qt::DisplayRole: {
			switch ( index.column() ) {
				case 0:
					return variable->name;
				case 1:
					return variable->value;
			}
		}
		case Qt::ToolTipRole: {
			const QStringList entries = QStringList()
				<< tr( "Name: %1" ).arg( variable->name )
				<< tr( "Value: %1" ).arg( variable->value )
				<< tr( "Enabled: %1" ).arg( variable->enabled ? tr( "true" ) : tr( "false" ) );
			return entries.join( "\n" );
		}
		case Qt::FontRole: {
			QFont font;
			font.setStrikeOut( !variable->enabled );
			return font;
		}
		case Qt::CheckStateRole: {
			if ( index.column() == 0 ) {
				return variable->enabled ? Qt::Checked : Qt::Unchecked;
			}

			break;
		}
	}

	return QVariant();
}

QModelIndex pEnvironmentVariablesModel::index( int row, int column, const QModelIndex& parent ) const
{
	if ( parent.isValid() || column < 0 || column >= 2 || row < 0 || row >= mRowCount ) {
		return QModelIndex();
	}

	pEnvironmentVariablesModel::Variable* variable = mOrder[ row ];
	return createIndex( row, column, variable );
}

QModelIndex pEnvironmentVariablesModel::parent( const QModelIndex& index ) const
{
	Q_UNUSED( index );
	return QModelIndex();
}

int pEnvironmentVariablesModel::rowCount( const QModelIndex& parent ) const
{
	return parent.isValid() ? 0 : mRowCount;
}

QVariant pEnvironmentVariablesModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( orientation == Qt::Vertical || section < 0 || section >= 2 ) {
		return QAbstractItemModel::headerData( section, orientation, role );
	}

	switch ( role ) {
		case Qt::DisplayRole: {
			switch ( section ) {
				case 0:
					return tr( "Name" );
				case 1:
					return tr( "Value" );
			}
		}
	}

	return QAbstractItemModel::headerData( section, orientation, role );
}

bool pEnvironmentVariablesModel::hasChildren( const QModelIndex& parent ) const
{
	return parent.isValid() ? false : !mVariables.isEmpty();
}

Qt::ItemFlags pEnvironmentVariablesModel::flags( const QModelIndex& index ) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags( index );

	if ( !index.isValid() || index.row() < 0 || index.row() >= mRowCount || index.column() < 0 || index.column() >= 2 ) {
		return flags;
	}

	if ( index.column() == 0 ) {
		flags |= Qt::ItemIsUserCheckable;
	}

	return flags;
}

bool pEnvironmentVariablesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	if ( !index.isValid() || index.column() != 0 || index.row() < 0 || index.row() >= mRowCount ) {
		return false;
	}

	pEnvironmentVariablesModel::Variable* variable = static_cast<pEnvironmentVariablesModel::Variable*>( index.internalPointer() );

	switch ( role ) {
		case Qt::CheckStateRole: {
			variable->enabled = value.toInt() == Qt::Checked;
			emit dataChanged( index, index.sibling( index.row(), 1 ) );
		}
	}

	return false;
}

QModelIndex pEnvironmentVariablesModel::index( const QString& name, int column ) const
{
	if ( !mVariables.contains( name ) || column < 0 || column >= 2 ) {
		return QModelIndex();
	}

	pEnvironmentVariablesModel::Variable* variable = &const_cast<pEnvironmentVariablesModel*>( this )->mVariables[ name ];
	return createIndex( mOrder.indexOf( variable ), column, variable );
}

pEnvironmentVariablesModel::Variable pEnvironmentVariablesModel::variable( const QModelIndex& index ) const
{
	pEnvironmentVariablesModel::Variable variable;

	if ( index.isValid() && index.row() >= 0 && index.row() < mRowCount && index.column() >= 0 && index.column() < 2 ) {
		variable = *static_cast<pEnvironmentVariablesModel::Variable*>( index.internalPointer() );
	}

	return variable;
}

pEnvironmentVariablesModel::Variables pEnvironmentVariablesModel::variables() const
{
	return mVariables;
}

pEnvironmentVariablesModel::Variables pEnvironmentVariablesModel::defaultVariables() const
{
	return mDefaultVariables;
}

QStringList pEnvironmentVariablesModel::variables( bool keepDisabled ) const
{
	return variablesToStringList( mVariables, keepDisabled );
}

pEnvironmentVariablesModel::Variable pEnvironmentVariablesModel::variable( const QString& name ) const
{
	return mVariables.value( name );
}

bool pEnvironmentVariablesModel::contains( const QString& variable ) const
{
	return mVariables.contains( variable );
}

bool pEnvironmentVariablesModel::isEmpty() const
{
	return mVariables.isEmpty();
}

pEnvironmentVariablesModel::Variables pEnvironmentVariablesModel::stringListToVariables( const QStringList& variables )
{
	pEnvironmentVariablesModel::Variables items;

	foreach ( const QString& v, variables ) {
		const QString name = v.section( '=', 0, 0 );
		const QString value = v.section( '=', 1 );

		pEnvironmentVariablesModel::Variable variable;
		variable.name = name;
		variable.value = value;
		variable.enabled = true;

		items[ name ] = variable;
	}
	
	return items;
}

QStringList pEnvironmentVariablesModel::variablesToStringList( const pEnvironmentVariablesModel::Variables& variables, bool keepDisabled )
{
	QStringList items;
	
	foreach ( const pEnvironmentVariablesModel::Variable& variable, variables.values() ) {
		if ( !variable.enabled && !keepDisabled ) {
			continue;
		}
		
		items << QString( "%1=%2" ).arg( variable.name ).arg( variable.value );
	}
	
	return items;
}

void pEnvironmentVariablesModel::setVariables( const pEnvironmentVariablesModel::Variables& variables, bool setDefault )
{
	emit layoutAboutToBeChanged();

	int count = rowCount();

	if ( count > 0 ) {
		beginRemoveRows( QModelIndex(), 0, count -1 );
	}

	mRowCount = 0;
	mOrder.clear();
	mVariables.clear();

	if ( setDefault ) {
		mDefaultVariables.clear();
	}

	if ( count > 0 ) {
		endRemoveRows();
	}

	count = variables.count();

	if ( count > 0 ) {
		beginInsertRows( QModelIndex(), 0, count -1 );
	}

	mVariables = variables;
	mRowCount = count;

	if ( setDefault ) {
		setDefaultVariables( mVariables );
	}

	QStringList keys = mVariables.keys();
	qSort( keys );

	foreach ( const QString& key, keys ) {
		mOrder << &mVariables[ key ];
	}

	if ( count > 0 ) {
		endInsertRows();
	}

	emit layoutChanged();
}

void pEnvironmentVariablesModel::setDefaultVariables( const pEnvironmentVariablesModel::Variables& variables )
{
	mDefaultVariables = variables;
	emit defaultVariablesChanged();
}

void pEnvironmentVariablesModel::setVariables( const QStringList& variables, bool setDefault )
{
	setVariables( stringListToVariables( variables ), setDefault );
}

void pEnvironmentVariablesModel::setDefaultVariables( const QStringList& variables )
{
	setDefaultVariables( stringListToVariables( variables ) );
}

void pEnvironmentVariablesModel::setVariable( const QString& name, const pEnvironmentVariablesModel::Variable& variable )
{
	const bool hasVariable = mVariables.contains( name );
	QStringList keys = mVariables.keys();
	int row = -1;

	if ( !hasVariable ) {
		keys << name;
		qSort( keys );
		row = keys.indexOf( name );

		beginInsertRows( QModelIndex(), row, row );
	}

	mVariables[ name ] = variable;
	
	if ( hasVariable ) {
		const QModelIndex index = this->index( variable.name, 0 );
		emit dataChanged( index, index.sibling( index.row(), 1 ) );
	}

	if ( !hasVariable ) {
		mOrder.insert( row, &mVariables[ name ] );
		mRowCount++;

		endInsertRows();
	}
}

void pEnvironmentVariablesModel::removeVariable( const QString& name )
{
	if ( !mVariables.contains( name ) ) {
		return;
	}

	pEnvironmentVariablesModel::Variable& variable = mVariables[ name ];
	const int row = mOrder.indexOf( &variable );

	beginRemoveRows( QModelIndex(), row, row );
	mRowCount--;
	mOrder.removeAt( row );
	mVariables.remove( name );
	endRemoveRows();
}

void pEnvironmentVariablesModel::clearVariables()
{
	setVariables( pEnvironmentVariablesModel::Variables(), false );
}

void pEnvironmentVariablesModel::resetVariablesToDefault()
{
	setVariables( pEnvironmentVariablesModel::Variables( mDefaultVariables ), false );
}

void pEnvironmentVariablesModel::resetVariablesToSystem( bool setDefault )
{
	setVariables( QProcess::systemEnvironment(), setDefault );
}
