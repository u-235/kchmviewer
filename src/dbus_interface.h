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

#ifndef DBUS_INTERFACE_H
#define DBUS_INTERFACE_H

#include <QObject>
#include <QString>
#include <QStringList>


#define SERVICE_NAME            "net.kchmviewer.application"

class DBusInterface : public QObject
{
		Q_OBJECT
		Q_CLASSINFO( "D-Bus Interface", "net.kchmviewer.application" )

	public:
		DBusInterface( QObject* parent = 0 );
		~DBusInterface();

	public Q_SLOTS:
		//! Loads a CHM file \a filename , and opens the URL \a url. Use URL "/" to open default homepage
		Q_SCRIPTABLE void loadHelpFile( const QString& filename, const QString& url );

		//! Opens a specific \a url inside the loaded CHM file
		Q_SCRIPTABLE void openPage( const QString& url );

		//! Tries to find word in index, opening the index window and scrolling it there
		Q_SCRIPTABLE void guiFindInIndex( const QString& word );

		//! Executes a search in GUI. \a query contains the complete search query.
		Q_SCRIPTABLE void guiSearchQuery( const QString& query );

		//! Executes a search; GUI is not involved and user sees nothing.
		//! \a query contains the complete search query.
		//! Returns a list of URLs, or empty array if nothing os
		Q_SCRIPTABLE QStringList searchQuery( const QString& query );
};

#endif // DBUSIFACE_H
