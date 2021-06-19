/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2004-2014 George Yunaev, gyunaev@ulduzsoft.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtDebug>
#include <QMimeData>
#include <QAbstractListModel>

#include "toolbarmanager.h"
#include "toolbareditor.h"


static const char* ACTION_MIME_FORMAT = "application/vnd.action.list";


//
// A subclassed list model which supports toolbars
//
class ActionListModel : public QAbstractListModel
{
	public:
		ActionListModel( ToolbarEditor* editor, const QStringList& actions, bool actionSource )
			: QAbstractListModel( editor )
		{
			m_editor = editor;
			m_actions = actions;
			m_actionSource = actionSource;
		}

		int rowCount ( const QModelIndex& ) const
		{
			return m_actions.size();
		}

		QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const
		{
			if ( !index.isValid() || index.row() < 0 || index.row() >= m_actions.size()  )
				return QVariant();

			if ( m_actions[ index.row() ] == ToolbarManager::separatorName() )
			{
				if ( role == Qt::DisplayRole )
					return "--- separator ---";

				return QVariant();
			}

			QAction* action = m_editor->findAction( m_actions[ index.row() ] );

			if ( !action )
				return QVariant();

			switch ( role )
			{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return action->toolTip();

			case Qt::DecorationRole:
				return action->icon();
			}

			return QVariant();
		}

		Qt::ItemFlags flags ( const QModelIndex& index ) const
		{
			if ( index.isValid() )
				return Qt::ItemIsSelectable	| Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
			else
				return Qt::ItemIsDropEnabled;
		}

		Qt::DropActions supportedDropActions() const
		{
			return Qt::MoveAction;
		}

		// Required for drag and drop
		bool insertRows ( int row, int count, const QModelIndex& parent = QModelIndex() )
		{
			int start = row;
			int end = row + count - 1;
			emit beginInsertRows ( parent, start, end );

			for ( ; count > 0; count-- )
				m_actions.insert( row, 0 );

			emit endInsertRows();
			return true;
		}

		// Required for drag and drop
		bool removeRows ( int row, int count, const QModelIndex& parent = QModelIndex() )
		{
			// Do not remove a row with separator
			if ( m_actionSource && m_actions[row ] == ToolbarManager::separatorName() )
				return true;

			int start = row;
			int end = row + count - 1;
			emit beginRemoveRows( parent, start, end );

			for ( ; count > 0; count-- )
				m_actions.removeAt( row );

			emit endRemoveRows();
			return true;
		}

		// Set drag type for drag/drop
		QStringList mimeTypes() const
		{
			QStringList types;
			types << ACTION_MIME_FORMAT;
			return types;
		}

		QMimeData* mimeData( const QModelIndexList& indexes ) const
		{
			if ( indexes.size() != 1 || !indexes[0].isValid() )
				return 0;

			QMimeData* mimeData = new QMimeData();
			mimeData->setData( ACTION_MIME_FORMAT, m_actions[ indexes[0].row() ].toUtf8() );
			return mimeData;
		}

		bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
		{
			if ( action == Qt::IgnoreAction )
				return true;

			if ( !data->hasFormat( ACTION_MIME_FORMAT ) )
				return false;

			// Our list has only one column
			if ( column > 0 )
				return false;

			// We initially examine the row number supplied to see if we can use it to insert items into the model,
			// regardless of whether the parent index is valid or not.
			// If the parent model index is valid, the drop occurred on an item. In this simple list model, we find
			// out the row number of the item and use that value to insert dropped items into the top level of the model.
			int beginRow;

			if ( row != -1 )
				beginRow = row;
			else if ( parent.isValid() )
				beginRow = parent.row();
			else
				beginRow = rowCount( QModelIndex() );

			QByteArray actionName = data->data( ACTION_MIME_FORMAT );

			// Do not add this_is_separator to the source actionlist again
			if ( m_actionSource && actionName == ToolbarManager::separatorName() )
				return true;

			// The strings can then be inserted into the underlying data store. For consistency, this can be done
			// through the model's own interface.
			insertRows( beginRow, 1, QModelIndex() );
			m_actions[ beginRow ] = actionName;

			emit dataChanged( index( beginRow, 0, QModelIndex() ), index( beginRow, 0, QModelIndex() ) );
			return true;
		}

		QStringList	actions() const
		{
			return m_actions;
		}

	private:
		ToolbarEditor*			m_editor;
		QStringList				m_actions;
		bool					m_actionSource;
};



ToolbarEditor::ToolbarEditor( QWidget* parent )
	: QDialog( parent ), Ui::ToolbarEditor()
{
	setupUi( this );

	m_allowActionsWithoutIcons = true;
	m_activeToolbar = 0;

	connect( boxToolbars, SIGNAL( activated( int ) ), this, SLOT( toolbarSelected( int ) ) );
}

ToolbarEditor::~ToolbarEditor()
{
}

void ToolbarEditor::addToolbar( QToolBar* toolbar )
{
	m_toolbars.push_back( toolbar );
}

void ToolbarEditor::addToolbars( QList<QToolBar*> toolbars )
{
	foreach ( QToolBar* t, toolbars )
		m_toolbars.push_back( t );
}

void ToolbarEditor::setAvailableActions( QList<QAction*> availableActions )
{
	m_availableActions = availableActions;
}


int ToolbarEditor::exec()
{
	if ( m_toolbars.size() == 0 )
		qFatal( "ToolbarEditor::exec: no toolbars selected, and editing is disabled. Aborting." );

	// Hide the group allowing to edit toolbar list if we do not allow editing, and there
	// is only one toolbar
	if ( m_toolbars.size() == 1 )
		groupEditToolbars->hide();

	// Init the toolbar(s) actions
	foreach ( QToolBar* toolbar, m_toolbars )
	{
		initToolbarActions( toolbar );

		// Add a toolbar into the combobox
		boxToolbars->addItem( toolbar->windowTitle() );
	}

	m_activeToolbar = m_toolbars.front();
	setupViews( m_activeToolbar );

	return QDialog::exec();
}

QAction* ToolbarEditor::findAction( const QString& objectname ) const
{
	foreach ( QAction* action, m_availableActions )
		if ( ToolbarManager::actionName( action ) == objectname )
			return action;

	return 0;
}


void ToolbarEditor::initToolbarActions( QToolBar* toolbar )
{
	QStringList selected;

	foreach ( QAction* action, toolbar->actions() )
	{
		if ( action->isSeparator() )
			selected.push_back( ToolbarManager::separatorName() );
		else if ( ToolbarManager::hasAction( m_availableActions, action ) )
			selected.push_back( ToolbarManager::actionName( action ) );
	}

	m_selected[ toolbar ] = selected;
}

void ToolbarEditor::setupViews( QToolBar* toolbar )
{
	if ( !m_selected.contains( toolbar ) )
		qFatal( "ToolbarEditor::setupViews: invalid toolbar" );

	QStringList actions = m_selected[ toolbar ];

	// Create the list of available actions
	QStringList available;

	foreach ( QAction* action, m_availableActions )
	{
		if ( !actions.contains( ToolbarManager::actionName( action ) ) )
		{
			if ( m_allowActionsWithoutIcons || !action->icon().isNull() )
				available.push_back( ToolbarManager::actionName( action ) );
		}
	}

	if ( !available.contains( ToolbarManager::separatorName() ) )
		available.push_back( ToolbarManager::separatorName() );

	// Init models for available and selected actions
	ActionListModel* newModelAvailable = new ActionListModel( this, available, true );
	ActionListModel* newModelSelected = new ActionListModel( this, actions, false );

	// Set them, and get the old models
	ActionListModel* oldModelAvailable = ( ActionListModel* ) listAvailable->model();
	ActionListModel* oldModelSelected = ( ActionListModel* ) listActions->model();

	listActions->setModel( newModelSelected );
	listAvailable->setModel( newModelAvailable );

	// Remove old models
	delete oldModelAvailable;
	delete oldModelSelected;

	m_activeToolbar = toolbar;
}

void ToolbarEditor::toolbarSelected( int index )
{
	if ( index == -1 )
		return;

	QToolBar* selected = m_toolbars[ index ];

	if ( selected == m_activeToolbar )
		return;

	// Copy the settings from active toolbar, and switch to new toolbar
	updateToolbarActions( m_activeToolbar );
	setupViews( selected );
}

void ToolbarEditor::updateToolbarActions( QToolBar* toolbar )
{
	if ( !m_selected.contains( toolbar ) )
		qFatal( "ToolbarEditor::updateToolbarActions: invalid toolbar" );

	ActionListModel* model = ( ActionListModel* ) listActions->model();
	m_selected[ toolbar ] = model->actions();
}

QStringList	ToolbarEditor::actionsForToolbar( QToolBar* toolbar )
{
	if ( !m_selected.contains( toolbar ) )
		return QStringList();
	else
		return m_selected[ toolbar ];
}

void ToolbarEditor::accept()
{
	// Copy the last settings
	updateToolbarActions( m_activeToolbar );

	// Reset models
	delete listAvailable->model();
	delete listActions->model();
	listActions->setModel( 0 );
	listAvailable->setModel( 0 );

	QDialog::accept();
}
