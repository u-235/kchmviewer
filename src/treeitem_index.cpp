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

#include "ebook.h"
#include "mainwindow.h"
#include "dialog_chooseurlfromlist.h"
#include "treeitem_index.h"

TreeItem_Index::TreeItem_Index( QTreeWidgetItem* parent, QTreeWidgetItem* after, const QString& name,
								const QList<QUrl>& urls, const QString& seealso )
	: QTreeWidgetItem( parent, after )
{
	m_name = name;
	m_urls = urls;
	m_seealso = seealso;
}

TreeItem_Index::TreeItem_Index( QTreeWidget* parent, QTreeWidgetItem* after, const QString& name,
								const QList<QUrl>& urls, const QString& seealso )
	: QTreeWidgetItem( parent, after )
{
	m_name = name;
	m_urls = urls;
	m_seealso = seealso;
}

QUrl TreeItem_Index::getUrl() const
{
	if ( m_urls.size() == 1 )
		return m_urls.front();

	// Create a dialog with URLs, and show it, so user can select an URL he/she wants.
	QStringList titles;
	EBook* xchm = ::mainWindow->chmFile();

	for ( int i = 0; i < m_urls.size(); i++ )
	{
		QString title = xchm->getTopicByUrl( m_urls[i] );

		if ( title.isEmpty() )
		{
			qWarning( "Could not get item name for url '%s'", qPrintable( m_urls[i].toString() ) );
			titles.push_back( QString::null );
		}
		else
			titles.push_back( title );
	}

	DialogChooseUrlFromList dlg( ::mainWindow );
	return dlg.getSelectedItemUrl( m_urls, titles );
}

bool TreeItem_Index::containstUrl( const QUrl& url ) const
{
	for ( int i = 0; i < m_urls.size(); i++ )
	{
		if ( m_urls[i] == url )
			return true;
	}

	return false;
}

bool TreeItem_Index::isSeeAlso() const
{
	return !m_seealso.isEmpty();
}

QString TreeItem_Index::seeAlso() const
{
	return m_seealso;
}

int TreeItem_Index::columnCount() const
{
	return 1;
}

QVariant TreeItem_Index::data( int column, int role ) const
{
	if ( column != 0 )
		return QVariant();

	switch ( role )
	{
	// Item name
	case Qt::DisplayRole:
		return m_name;

	// Item foreground color
	case Qt::ForegroundRole:

		// For Index URL it means that there is URL list in m_url
		if ( m_urls.size() > 1 )
			return QBrush( QColor( Qt::red ) );
		else if ( isSeeAlso() )
			return QBrush( QColor( Qt::lightGray ) );

		break;

	case Qt::ToolTipRole:
	case Qt::WhatsThisRole:
		return m_name;
	}

	return QVariant();
}
