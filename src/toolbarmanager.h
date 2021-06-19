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

#ifndef TOOLBARMANAGER_H
#define TOOLBARMANAGER_H

#include <QMap>
#include <QList>
#include <QObject>
#include <QAction>
#include <QToolBar>

// This class manages application toolbars, including the following:
// - Stores and restores the toolbars, including their position and content;
// - Allows toolbar editing;
class ToolbarManager : public QObject
{
		Q_OBJECT

	public:
		// Returns the name of the separator object which should be used in place
		// of separators where needed.
		static QString	separatorName();
		static QString	actionName( QAction* action );
		static bool		hasAction( const QList<QAction*>& actions, QAction* action );

		ToolbarManager( QObject* parent = 0, const QString& settingpath = "/tooolbars" );
		virtual ~ToolbarManager();

		// Set the actions available in all toolbars. Actions which are stored for toolbars
		// must be present in this list, or they will be ignored.
		void	setAvailableActions( QList<QAction*> availableActions );

		// Query the actions available to set in toolbar from the provided QObject as children
		// of this object. Typically (always for UIC-generated files) all actions have application
		// MainWindow as their parent.
		void	queryAvailableActions( QObject* source );

		// Adds a toolbar to the list of managed toolbars
		void	addManaged( QToolBar* toolbar );

		// Loads the managed toolbars actions; keeps toolbars intact if nothing is loaded
		void	load();

		// Saves the managed toolbars actions
		void	save();

		// Shows the edit toolbars dialog
		void	editDialog();

	private:
		void	applyActions( QToolBar* toolbar, const QStringList& actions );

		// Keeps available actions
		QList<QAction*>			m_availableActions;

		// Keeps the managed toolbars
		QList< QToolBar*>		m_toolbars;

		// Setting name
		QString					m_settingsRoot;
};

#endif // TOOLBARMANAGER_H
