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

#include <QtDBus/QtDBus>

#include "dbus_interface.h"
#include "mainwindow.h"
#include "navigationpanel.h"
#include "tab_search.h"


DBusInterface::DBusInterface( QObject* parent )
	: QObject( parent )
{
	QDBusConnection::sessionBus().registerObject( "/application",
			this,
			QDBusConnection::ExportScriptableSlots );
}


DBusInterface::~DBusInterface()
{
}


void DBusInterface::loadHelpFile( const QString& filename, const QString& page2open )
{
	QStringList args;

	args.push_back( filename );
	args.push_back( page2open );

	qApp->postEvent( ::mainWindow, new UserEvent( "loadAndOpen", args ) );
}


void DBusInterface::openPage( const QString& page2open )
{
	QStringList args;

	args.push_back( page2open );
	qApp->postEvent( ::mainWindow, new UserEvent( "openPage", args ) );
}


void DBusInterface::guiFindInIndex( const QString& word )
{
	QStringList args;

	args.push_back( word );
	qApp->postEvent( ::mainWindow, new UserEvent( "findInIndex", args ) );
}


void DBusInterface::guiSearchQuery( const QString& query )
{
	QStringList args;

	args.push_back( query );
	qApp->postEvent( ::mainWindow, new UserEvent( "searchQuery", args ) );
}

QStringList DBusInterface::searchQuery( const QString& query )
{
	return ::mainWindow->navigator()->searchQuery( query );
}
