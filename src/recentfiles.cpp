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

#include <QFileInfo>
#include <QSettings>
#include <QStringList>

#include "recentfiles.h"

RecentFiles::RecentFiles( QMenu* menu, QAction* before, int maxfiles, const QString& settingsname )
{
	if ( maxfiles < 1 )
		qFatal( "RecentFiles::RecentFiles: maxfiles (%d) is < 1 ", maxfiles );

	m_settingsName = settingsname.isEmpty() ? "recentFileList" : settingsname;
	m_actions.resize( maxfiles );

	// Create the actions
	for ( int i = 0; i < maxfiles; ++i )
	{
		m_actions[i] = new QAction( this );
		m_actions[i]->setVisible( false );
		connect( m_actions[i], SIGNAL( triggered() ), this, SLOT( actionRecent() ) );
	}

	// Add them to the menu
	for ( int i = 0; i < maxfiles; ++i )
		menu->insertAction( before, m_actions[i] );

	// Add a separator after the last action
	m_separator = menu->insertSeparator( before );

	// Update the actions menu
	updateMenu();
}

RecentFiles::~RecentFiles()
{
}

void RecentFiles::setCurrentFile( const QString& file )
{
	QStringList files = loadRecentFiles();
	files.removeAll( file );
	files.prepend( file );

	while ( files.size() > m_actions.size() )
		files.removeLast();

	saveRecentFiles( files );

	updateMenu();
}

void RecentFiles::removeRecentFile( const QString& file )
{
	QStringList files = loadRecentFiles();
	files.removeAll( file );
	saveRecentFiles( files );

	updateMenu();
}

void RecentFiles::actionRecent()
{
	QAction* action = qobject_cast<QAction*>( sender() );

	if ( action )
		emit openRecentFile( action->data().toString() );
}

void RecentFiles::updateMenu()
{
	QStringList files = loadRecentFiles();
	int numRecentFiles = qMin( files.size(), m_actions.size() );

	for ( int i = 0; i < m_actions.size(); ++i )
	{
		if ( i < numRecentFiles )
		{
			QString text = tr( "&%1 %2" ).arg( i + 1 ).arg( QFileInfo( files[i] ).fileName() );
			m_actions[i]->setText( text );
			m_actions[i]->setToolTip( files[i] );
			m_actions[i]->setData( files[i] );
			m_actions[i]->setVisible( true );
		}
		else
			m_actions[i]->setVisible( false );
	}

	m_separator->setVisible( numRecentFiles > 0 );
}

QString	RecentFiles::latestFile()
{
	QStringList files = loadRecentFiles();

	if ( files.isEmpty() )
		return QString::null;
	else
		return files[0];
}

QStringList	RecentFiles::loadRecentFiles()
{
	QSettings settings;
	return settings.value( m_settingsName ).toStringList();
}

void RecentFiles::saveRecentFiles( const QStringList& files )
{
	QSettings settings;
	settings.setValue( m_settingsName, files );
}
