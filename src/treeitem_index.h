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

#ifndef TREEITEM_INDEX_H
#define TREEITEM_INDEX_H

#include <QTreeWidgetItem>

class TreeItem_Index : public QTreeWidgetItem
{
	public:
		TreeItem_Index( QTreeWidgetItem* parent, QTreeWidgetItem* after, const QString& name, const QList<QUrl>& urls,
						const QString& seealso );
		TreeItem_Index( QTreeWidget* parent, QTreeWidgetItem* after, const QString& name, const QList<QUrl>& urls,
						const QString& seealso );

		QUrl		getUrl() const;
		bool		containstUrl( const QUrl& url ) const;
		bool		isSeeAlso() const;
		QString		seeAlso() const;

		// Overridden methods
		int 		columnCount () const;
		QVariant 	data ( int column, int role ) const;

	private:
		QString		m_name;
		QList<QUrl>	m_urls;
		QString		m_seealso;
};

#endif // TREEITEM_INDEX_H
