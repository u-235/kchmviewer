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

#ifndef TAB_CONTENTS_H
#define TAB_CONTENTS_H

#include "kde-qt.h"
#include "treeitem_toc.h"
#include "ui_tab_contents.h"


class TabContents : public QWidget, public Ui::TabContents
{
		Q_OBJECT
	public:
		TabContents( QWidget* parent = 0 );
		~TabContents();

		void	refillTableOfContents();
		void	showItem( TreeItem_TOC* item );
		void	search( const QString& text );
		void	focus();

		TreeItem_TOC* 	getTreeItem( const QUrl& url );

	public slots:
		void	onContextMenuRequested ( const QPoint& point );
		void	onClicked ( QTreeWidgetItem* item, int column );

	private:
		QMenu* 		m_contextMenu;
};


#endif /* INCLUDE_KCHMCONTENTSWINDOW_H */
