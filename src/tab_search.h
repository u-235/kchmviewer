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

#ifndef TAB_SEARCH_H
#define TAB_SEARCH_H

#include "kde-qt.h"
#include "settings.h"
#include "ui_tab_search.h"

class EBookSearch;

class TabSearch : public QWidget, public Ui::TabSearch
{
		Q_OBJECT
	public:
		TabSearch( QWidget* parent = 0 );

		void	invalidate();
		void	restoreSettings ( const Settings::search_saved_settings_t& settings );
		void	saveSettings( Settings::search_saved_settings_t& settings );
		void	execSearchQueryInGui( const QString& query );
		bool	searchQuery( const QString& query, QList<QUrl>* results );
		void	focus();

	private slots:
		void	onContextMenuRequested ( const QPoint& point );
		void	onHelpClicked( const QString& );
		void 	onReturnPressed ();
		void	onItemActivated( QTreeWidgetItem* item, int );

		// For index generation
		void	onProgressStep( int value, const QString& stepName );

	private:
		bool	initSearchEngine();

	private:
		QMenu*			 	m_contextMenu;
		EBookSearch*			m_searchEngine;
		bool				m_searchEngineInitDone;

		// For index generation
		QProgressDialog* 	m_genIndexProgress;
};

#endif
