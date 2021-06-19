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

#ifndef NAVIGATIONPANEL_H
#define NAVIGATIONPANEL_H

#include <QDockWidget>
#include "ui_navigatorpanel.h"
#include "kde-qt.h"

class EBook;

// This class shows content, index, search and bookmark tabs
class NavigationPanel : public QDockWidget, public Ui::NavigatorPanel
{
		Q_OBJECT

	public:
		enum
		{
			TAB_CONTENTS,
			TAB_INDEX,
			TAB_SEARCH,
			TAB_BOOKMARK
		};

		NavigationPanel( QWidget* parent );

		// Sets the bookmark menu (maintained by bookmark tab)
		void	setBookmarkMenu( QMenu* menu );

		// Invalidate data in all tabs
		void	invalidate();

		// Update tabs content from CHM file data
		void	updateTabs( EBook* file );

		// Save/load current file settings
		void	applySettings( Settings* settings );
		void	getSettings( Settings* settings );

		// Active tab get/set
		int		active() const;
		void	setActive( int index );

		// Refresh content and index tab contents
		void	refresh();

		// Locate URL or text in the contents tab
		bool	findUrlInContents( const QUrl& url );
		void	findTextInContents( const QString& text );

		// Find text in index tab
		void	findInIndex( const QString& text );

		// Find text in search tab
		void	executeQueryInSearch( const QString& text );

		// Just find text without using search tab
		QStringList	searchQuery( const QString& text );

	public slots:
		// Add a new bookmark
		void	addBookmark();

		// Show previous/next page in table of contents
		void	showPrevInToc();
		void	showNextInToc();

	private:
		TabContents*				m_contentsTab;
		TabIndex*				m_indexTab;
		TabSearch*				m_searchTab;
		TabBookmarks*			m_bookmarksTab;
};

#endif // NAVIGATIONPANEL_H
