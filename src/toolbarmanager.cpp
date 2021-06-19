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

#include <QSettings>

#include "toolbareditor.h"
#include "toolbarmanager.h"


QString ToolbarManager::separatorName()
{
	return ".separator.";
}

QString	ToolbarManager::actionName( QAction* action )
{
	if ( action->isSeparator() )
		return ToolbarManager::separatorName();
	else
		return action->objectName();
}

bool ToolbarManager::hasAction( const QList<QAction*>& actions, QAction* action )
{
	foreach ( QAction* act, actions )
		if ( ToolbarManager::actionName( act ) == ToolbarManager::actionName( action ) )
			return true;

	return false;
}


ToolbarManager::ToolbarManager( QObject* parent, const QString& settingpath )
	: QObject( parent )
{
	m_settingsRoot = settingpath;
}

ToolbarManager::~ToolbarManager()
{
}

void ToolbarManager::setAvailableActions( QList<QAction*> availableActions )
{
	m_availableActions = availableActions;
}

void ToolbarManager::queryAvailableActions( QObject* source )
{
	m_availableActions.clear();

	// Enumerate through all available actions, and add them
	QObjectList objs = source->children();

	for ( QObjectList::const_iterator it = objs.begin(); it != objs.end(); ++it )
		if ( !strcmp( ( *it )->metaObject()->className(), "QAction" ) )
			m_availableActions.push_back( ( QAction* ) *it );
}

void ToolbarManager::addManaged( QToolBar* toolbar )
{
	m_toolbars.push_back( toolbar );
}

void ToolbarManager::applyActions( QToolBar* toolbar, const QStringList& actions )
{
	// Apply the actions to the toolbar
	toolbar->clear();

	foreach ( QString name, actions )
	{
		if ( name == separatorName() )
		{
			toolbar->addSeparator();
			continue;
		}

		foreach ( QAction* action, m_availableActions )
		{
			if ( actionName( action ) == name )
			{
				toolbar->addAction( action );
				break;
			}
		}
	}
}

void ToolbarManager::load()
{
	if ( m_availableActions.isEmpty() )
		qWarning( "ToolbarManager::load(): available action list is empty, did you forget to call setAvailableActions()?" );

	QSettings settings;

	foreach ( QToolBar* toolbar, m_toolbars )
	{
		QString settingName = m_settingsRoot + toolbar->objectName();

		// Do we have stored settings for this toolbar?
		if ( !settings.contains( settingName ) )
			continue;

		applyActions( toolbar, settings.value( settingName ).toStringList() );
	}
}

void ToolbarManager::save()
{
	QSettings settings;

	foreach ( QToolBar* toolbar, m_toolbars )
	{
		QString settingName = m_settingsRoot + toolbar->objectName();
		QStringList names;

		foreach ( QAction* action, toolbar->actions() )
		{
			if ( action->isSeparator() )
				names.push_back( separatorName() );
			else if ( hasAction( m_availableActions, action ) )
				names.push_back( actionName( action ) );
		}

		settings.setValue( settingName, names );
	}
}

void ToolbarManager::editDialog()
{
	ToolbarEditor dlg;
	dlg.setAvailableActions( m_availableActions );
	dlg.addToolbars( m_toolbars );

	if ( dlg.exec() == QDialog::Rejected )
		return;

	foreach ( QToolBar* toolbar, m_toolbars )
		applyActions( toolbar, dlg.actionsForToolbar( toolbar ) );
}
