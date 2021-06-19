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

#ifndef RECENTFILES_H
#define RECENTFILES_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMenu>


// This class assumes QSettings object can be created using default constructor, i.e.
//   QCoreApplication::setOrganizationName( ... );
//	 QCoreApplication::setOrganizationDomain( ... );
//	 QCoreApplication::setApplicationName( ... );
// have been called.
//
// This class is based on Qt example
//
class RecentFiles : public QObject
{
		Q_OBJECT

	public:
		// A constructor specifies the menu to add recent files to, and the action to add it before.
		RecentFiles( QMenu* menu, QAction* before, int maxfiles = 5, const QString& settingsname = QString::null );
		virtual ~RecentFiles();

	signals:
		void	openRecentFile( const QString& file );

	public slots:
		// Sets the current file to the recent file. Does the following:
		// - Adds it to the top of recent files list, or moves it to the top;
		// - Removes the last entry, if necessary;
		void	setCurrentFile( const QString& file );

		// Removes the current file from the recent files. Useful, for example,
		// when attempt to open a recent project failed.
		void	removeRecentFile( const QString& file );

		// Returns the last added recent file
		QString	latestFile();

	protected:
		// Override those functions in a derived class to store/load the
		// list of recent files from a different place
		QStringList	loadRecentFiles();
		void		saveRecentFiles( const QStringList& files );

	private slots:
		void	actionRecent();
		void	updateMenu();

	private:
		QString				m_settingsName;
		QAction*				m_separator;
		QVector< QAction* >	m_actions;
};

#endif // RECENTFILES_H
