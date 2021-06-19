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

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDateTime>

#include "settings.h"
#include "config.h"

static qint32 SETTINGS_MAGIC = 0xD8AB4E76;
static qint32 SETTINGS_VERSION = 4;

/*
 * The order is important!
 * To be compatible with next versions, you may add items ONLY before the MARKER_END!
 */
enum marker_t
{
	MARKER_FILESIZE = 1,
	MARKER_FILETIME,

	MARKER_ACTIVETABSYSTEM,
	MARKER_ACTIVETABWINDOW,
	MARKER_ACTIVEENCODING,
	MARKER_SEARCHHISTORY,
	MARKER_WINDOW_SIZE,

	MARKER_BOOKMARKS,
	MARKER_VIEWINDOWS,

	MARKER_CONTENTSDATA,
	MARKER_INDEXDATA,

	MARKER_ACTIVEENCODINGNAME,

	// This should be the last
	MARKER_END = 0x7FFF
};

// Helpers for serialization of SavedBookmark through QDataStream
static inline QDataStream& operator<< ( QDataStream& s, const Settings::SavedBookmark& b )
{
	s << b.name;
	s << b.url;
	s << b.scroll_y;
	return s;
}

static inline QDataStream& operator>> ( QDataStream& s, Settings::SavedBookmark& b )
{
	s >> b.name;
	s >> b.url;
	s >> b.scroll_y;
	return s;
}

// Helpers for serialization of SavedViewWindow through QDataStream
static inline QDataStream& operator<< ( QDataStream& s, const Settings::SavedViewWindow& b )
{
	// Store the version first. Later we can increase it when adding new members.
	s << 2;
	s << b.url;
	s << b.scroll_y;
	s << b.zoom;
	return s;
}

static inline QDataStream& operator>> ( QDataStream& s, Settings::SavedViewWindow& b )
{
	qint32 version;

	s >> version;
	s >> b.url;
	s >> b.scroll_y;

	if ( version == 1 )
	{
		// Old one; convert zoom
		int oldzoom;

		s >> oldzoom;
		b.zoom = 1.0 + oldzoom * 0.5;
	}
	else
		s >> b.zoom;

	return s;
}


Settings::Settings()
{
	m_activetabsystem = 0;
	m_activetabwindow = 0;
	m_activeEncoding = "CP1252";

	m_window_size_x = 700;
	m_window_size_y = 500;
	m_window_size_splitter = 200;
}


bool Settings::loadSettings( const QString& filename )
{
	m_activetabsystem = 0;
	m_activetabwindow = 0;
	m_activeEncoding = "CP1252";

	m_searchhistory.clear();
	m_bookmarks.clear();
	m_viewwindows.clear();

	QFileInfo finfo ( filename );

	m_settingsFile = QString::null;
	m_searchIndex = QString::null;

	if ( !finfo.size() )
		return false;

	// Init those params, as they'll be used during save the first time even if the file is not here
	m_currentfilesize = finfo.size();
	m_currentfiledate = finfo.lastModified().toTime_t();
	m_settingsFile = pConfig->getEbookSettingFile( filename );
	m_searchIndex = pConfig->getEbookIndexFile( filename );

	QFile file( m_settingsFile );

	if ( !file.open ( QIODevice::ReadOnly ) )
		return false; // it's ok, file may not exist

	QDataStream stream ( &file );

	// Read and check header
	qint32 data;
	bool complete_read = false;
	stream >> data; // magic

	if ( data != SETTINGS_MAGIC )
	{
		qWarning ( "file %s has bad magic value, ignoring it.", qPrintable( file.fileName() ) );
		return false;
	}

	stream >> data; // version

	if ( data > SETTINGS_VERSION )
	{
		qWarning ( "file %s has unsupported data version %d, ignoring it.", qPrintable( file.fileName() ), data );
		return false;
	}

	// Read everything by marker
	while ( 1 )
	{
		stream >> data; // marker

		if ( data == MARKER_END )
		{
			complete_read = true;
			break;
		}

		switch ( data )
		{
		case MARKER_FILESIZE:
			stream >> m_currentfilesize;

			if ( m_currentfilesize != finfo.size() )
			{
				m_currentfilesize = finfo.size();
				return false;
			}

			break;

		case MARKER_FILETIME:
			stream >> m_currentfiledate;

			if ( m_currentfiledate != finfo.lastModified().toTime_t() )
			{
				m_currentfiledate = finfo.lastModified().toTime_t();
				return false;
			}

			break;

		case MARKER_ACTIVETABSYSTEM:
			stream >> m_activetabsystem;
			break;

		case MARKER_ACTIVETABWINDOW:
			stream >> m_activetabwindow;
			break;

		// Not used anymore
		case MARKER_ACTIVEENCODING:
			stream >> data;
			break;

		case MARKER_ACTIVEENCODINGNAME:
			stream >> m_activeEncoding;
			break;

		case MARKER_WINDOW_SIZE:
			stream >> m_window_size_x;
			stream >> m_window_size_y;
			stream >> m_window_size_splitter;
			break;

		case MARKER_SEARCHHISTORY:
			stream >> m_searchhistory;
			break;

		case MARKER_BOOKMARKS:
			stream >> m_bookmarks;
			break;

		case MARKER_VIEWINDOWS:
			stream >> m_viewwindows;
			break;
		}
	}

	return complete_read;
}


bool Settings::saveSettings( )
{
	QFile file( m_settingsFile );

	if ( !file.open ( QIODevice::WriteOnly ) )
	{
		qWarning ( "Could not write settings into file %s: %s",
				   qPrintable( file.fileName() ),
				   qPrintable( file.errorString() ) );
		return false;
	}

	QDataStream stream ( &file );

	// Save header
	stream << SETTINGS_MAGIC;
	stream << SETTINGS_VERSION;

	// Save size and last-modified
	stream << MARKER_FILESIZE;
	stream << m_currentfilesize;
	stream << MARKER_FILETIME;
	stream << m_currentfiledate;

	// Save generic settings
	stream << MARKER_ACTIVETABSYSTEM;
	stream << m_activetabsystem;

	// Save generic settings
	stream << MARKER_ACTIVETABWINDOW;
	stream << m_activetabwindow;

	stream << MARKER_ACTIVEENCODINGNAME;
	stream << m_activeEncoding;

	// Save search history vector
	stream << MARKER_SEARCHHISTORY;
	stream << m_searchhistory;

	// Save window size and splitter position
	stream << MARKER_WINDOW_SIZE;
	stream << m_window_size_x;
	stream << m_window_size_y;
	stream << m_window_size_splitter;

	stream << MARKER_BOOKMARKS;
	stream << m_bookmarks;

	stream << MARKER_VIEWINDOWS;
	stream << m_viewwindows;

	stream << MARKER_END;
	return true;
}


void Settings::removeSettings( const QString& filename )
{
	QString settingsfile = pConfig->getEbookSettingFile( filename );
	QString idxfile = pConfig->getEbookIndexFile( filename );

	QFile::remove( settingsfile );
	QFile::remove( idxfile );
}
