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

#ifndef TAB_BOOKMARK_H
#define TAB_BOOKMARK_H

#include "kde-qt.h"
#include "settings.h"

#include "ui_tab_bookmarks.h"


class TabBookmarks : public QWidget, public Ui::TabBookmarks
{
		Q_OBJECT
	public:
		TabBookmarks( QWidget* parent );

		void 	createMenu( QMenu* menuBookmarks );

		void	restoreSettings ( const Settings::bookmark_saved_settings_t& settings );
		void	saveSettings ( Settings::bookmark_saved_settings_t& settings );
		void	invalidate();
		void	focus();

	public slots:
		void 	onAddBookmarkPressed ();

	private slots:
		void	actionBookmarkActivated();
		void	onDelBookmarkPressed( );
		void	onEditBookmarkPressed( );
		void	onItemActivated ( QListWidgetItem* );
		void	onContextMenuRequested ( const QPoint& point );

	private:
		QMenu* 			m_menuBookmarks;
		QMenu* 		 	m_contextMenu;
		QString			m_bookmarkFileName;
		bool			m_listChanged;
};

#endif
