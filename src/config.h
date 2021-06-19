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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QSettings>

#include "recentfiles.h"


class Config
{
	public:
		enum choose_action_t
		{
			ACTION_ALWAYS_OPEN,
			ACTION_ASK_USER,
			ACTION_DONT_OPEN
		};

		enum ToolbarMode
		{
			TOOLBAR_SMALLICONS,
			TOOLBAR_LARGEICONS,
			TOOLBAR_LARGEICONSTEXT,
			TOOLBAR_TEXTONLY
		};

		enum StartupMode
		{
			STARTUP_DO_NOTHING,
			STARTUP_LOAD_LAST_FILE,
			STARTUP_POPUP_OPENFILE
		};

		Config();
		void	save();

		// Returns the setting filename for this ebook
		QString	getEbookSettingFile( const QString& ebookfile ) const;

		// Returns the index filename for this ebook
		QString	getEbookIndexFile( const QString& ebookfile )  const;

	public:
		QString				m_lastOpenedDir;

		StartupMode			m_startupMode;
		choose_action_t		m_onNewChmClick;
		choose_action_t		m_onExternalLinkClick;
		int					m_numOfRecentFiles;
		bool				m_HistoryStoreExtra;
		ToolbarMode			m_toolbarMode;

		bool				m_browserEnableJS;
		bool				m_browserEnableJava;
		bool				m_browserEnablePlugins;
		bool				m_browserEnableImages;
		bool				m_browserEnableOfflineStorage;
		bool				m_browserEnableLocalStorage;
		bool				m_browserEnableRemoteContent;
		bool                m_browserHighlightSearchResults;
		bool                m_tocOpenAllEntries;
		bool                m_tabUseSingleClick;

		bool				m_advUseInternalEditor;
		QString				m_advExternalEditorPath;
		bool				m_advLayoutDirectionRL;
		bool				m_advAutodetectEncoding;
		bool				m_advCheckNewVersion;

	private:
		QString				m_datapath;
};

extern Config* pConfig;

#endif
