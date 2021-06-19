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

#ifndef TAB_INDEX_H
#define TAB_INDEX_H


#include "kde-qt.h"
#include "ui_tab_index.h"


class TabIndex : public QWidget, public Ui::TabIndex
{
		Q_OBJECT
	public:
		TabIndex( QWidget* parent = 0 );

		void	invalidate();
		void	search( const QString& index );
		void	focus();

	private slots:
		void 	onTextChanged ( const QString& newvalue );
		void 	onReturnPressed ();
		void	onItemActivated ( QTreeWidgetItem* item, int column );
		void	onContextMenuRequested ( const QPoint& point );

	private:
		void	showEvent ( QShowEvent* );

		void	refillIndex();

		QMenu* 			 	m_contextMenu;
		QTreeWidgetItem*		m_lastSelectedItem;
		bool				m_indexListFilled;
};

#endif
