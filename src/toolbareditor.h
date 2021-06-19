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

#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QToolBar>
#include <QAction>
#include <QStringList>

#include "ui_toolbareditor.h"

class ToolbarEditor : public QDialog, public Ui::ToolbarEditor
{
		Q_OBJECT

	public:
		// Returns true if an action name in the list actionsForToolbar() is a separator
		static bool	isSeparatorName( const QString& name );

	public:
		ToolbarEditor( QWidget* parent = 0 );
		~ToolbarEditor();

		// Add a toolbar to the list of toolbars to be editer
		void	addToolbar( QToolBar* toolbar );

		// Add toolbars to the list of toolbars to be editer
		void	addToolbars( QList<QToolBar*> toolbars );

		// Set the actions available to select in toolbars. Actions present in toolbars must be
		// present in this list, or they will not be shown.
		void	setAvailableActions( QList<QAction*> availableActions );

		// Enables or disables showing actions without icons in the available/selected lists.
		// Enabled by default.
		void	setAllowActionsWithoutIcons( bool allow );

		// Returns a list of selected actions for a specific toolbar
		QStringList	actionsForToolbar( QToolBar* toolbar );

	public slots:
		void	toolbarSelected( int index );
		void	accept();
		int		exec();

	private:
		friend class ActionListModel;
		QAction* findAction( const QString& objectname ) const;

		// Adds the existing toolbar actions into the internal storage
		void	initToolbarActions( QToolBar* toolbar );

		// Copies the changed toolbar actions from the list into the internal storage
		void	updateToolbarActions( QToolBar* toolbar );

		// Shows the current toolbar actions in views
		void	setupViews( QToolBar* toolbar );

	private:
		// A list of all available actions
		QList<QAction*>		m_availableActions;

		// A vector of edited toolbars to preserve the order
		QList< QToolBar* >	m_toolbars;

		// A map of edited toolbars with selected actions
		QMap< QToolBar*, QStringList >	m_selected;

		// Currently shown actions for toolbar
		QToolBar*			m_activeToolbar;

		// Params
		bool				m_allowActionsWithoutIcons;
};


#endif // TOOLBAREDITOR_H
