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

#include "kde-qt.h"

#include "mainwindow.h"
#include "treeitem_toc.h"
#include "tab_contents.h"
#include "config.h"


TabContents::TabContents( QWidget* parent )
	: QWidget( parent ), Ui::TabContents()
{
	setupUi( this );

	m_contextMenu = 0;

	tree->header()->hide();

	// Handle clicking on m_contentsWindow element
	if ( pConfig->m_tabUseSingleClick )
	{
		connect( tree,
				 SIGNAL( itemClicked( QTreeWidgetItem*, int ) ),
				 this,
				 SLOT( onClicked ( QTreeWidgetItem*, int ) ) );
	}
	else
	{
		connect( tree,
				 SIGNAL( itemActivated ( QTreeWidgetItem*, int ) ),
				 this,
				 SLOT( onClicked ( QTreeWidgetItem*, int ) ) );
	}

	// Activate custom context menu, and connect it
	tree->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( tree,
			 SIGNAL( customContextMenuRequested ( const QPoint& ) ),
			 this,
			 SLOT( onContextMenuRequested( const QPoint& ) ) );

	if ( ::mainWindow->chmFile() )
		refillTableOfContents();

	focus();
}

TabContents::~TabContents()
{
}

void TabContents::refillTableOfContents( )
{
	ShowWaitCursor wc;
	QList< EBookTocEntry > data;

	if ( !::mainWindow->chmFile()->getTableOfContents( data )
			|| data.size() == 0 )
	{
		qWarning ( "Table of contents is present but is empty; wrong parsing?" );
		return;
	}

	// Fill up the tree; we use a pretty complex routine to handle buggy CHMs
	QVector< TreeItem_TOC*> lastchild;
	QVector< TreeItem_TOC*> rootentry;
	bool warning_shown = false;

	tree->clear();

	for ( int i = 0; i < data.size(); i++ )
	{
		int indent = data[i].indent;

		// Do we need to add another indent?
		if ( indent >= rootentry.size() )
		{
			int maxindent = rootentry.size() - 1;

			// Resize the arrays
			lastchild.resize( indent + 1 );
			rootentry.resize( indent + 1 );

			if ( indent > 0 && maxindent < 0 )
				qFatal( "Invalid fisrt TOC indent (first entry has no root entry), aborting." );

			// And init the rest if needed
			if ( ( indent - maxindent ) > 1 )
			{
				if ( !warning_shown )
				{
					qWarning( "Invalid TOC step, applying workaround. Results may vary." );
					warning_shown = true;
				}

				for ( int j = maxindent; j < indent; j++ )
				{
					lastchild[j + 1] = lastchild[j];
					rootentry[j + 1] = rootentry[j];
				}
			}

			lastchild[indent] = 0;
			rootentry[indent] = 0;
		}

		// Create the node
		TreeItem_TOC* item;

		if ( indent == 0 )
			item = new TreeItem_TOC( tree, lastchild[indent], data[i].name, data[i].url, data[i].iconid );
		else
		{
			// New non-root entry. It is possible (for some buggy CHMs) that there is no previous entry: previoous entry had indent 1,
			// and next entry has indent 3. Backtracking it up, creating missing entries.
			if ( rootentry[indent - 1] == 0 )
				qFatal( "Child entry indented as %d with no root entry!", indent );

			item = new TreeItem_TOC( rootentry[indent - 1], lastchild[indent], data[i].name, data[i].url, data[i].iconid );
		}

		if ( pConfig->m_tocOpenAllEntries )
			item->setExpanded( true );

		lastchild[indent] = item;
		rootentry[indent] = item;
	}

	tree->update();
}


static TreeItem_TOC* findTreeItem( TreeItem_TOC* item, const QUrl& url, bool ignorefragment )
{
	if ( item->containstUrl( url, ignorefragment ) )
		return item;

	for ( int i = 0; i < item->childCount(); ++i )
	{
		TreeItem_TOC* bitem = findTreeItem( ( TreeItem_TOC* ) item->child( i ), url, ignorefragment );

		if ( bitem )
			return bitem;
	}

	return 0;
}

TreeItem_TOC* TabContents::getTreeItem( const QUrl& url )
{
	// During the first iteraction we check for the fragment as well, so the URLs
	// like ch05.htm#app1 and ch05.htm#app2 could be handled as different TOC entries
	for ( int i = 0; i < tree->topLevelItemCount(); i++ )
	{
		TreeItem_TOC* item = findTreeItem( ( TreeItem_TOC* ) tree->topLevelItem( i ), url, false );

		if ( item )
			return item;
	}

	// During the second iteraction we ignore the fragment, so if there is no ch05.htm#app1
	// but there is ch05.htm, we just use it
	for ( int i = 0; i < tree->topLevelItemCount(); i++ )
	{
		TreeItem_TOC* item = findTreeItem( ( TreeItem_TOC* ) tree->topLevelItem( i ), url, true );

		if ( item )
			return item;
	}

	return 0;
}

void TabContents::showItem( TreeItem_TOC* item )
{
	tree->setCurrentItem( item );
	tree->scrollToItem( item );
}


void TabContents::onClicked( QTreeWidgetItem* item, int )
{
	if ( !item )
		return;

	TreeItem_TOC* treeitem = ( TreeItem_TOC* ) item;
	::mainWindow->activateUrl( treeitem->getUrl() );
}

void TabContents::onContextMenuRequested( const QPoint& point )
{
	TreeItem_TOC* treeitem = ( TreeItem_TOC* ) tree->itemAt( point );

	if ( treeitem )
	{
		::mainWindow->currentBrowser()->setTabKeeper( treeitem->getUrl() );
		::mainWindow->tabItemsContextMenu()->popup( tree->viewport()->mapToGlobal( point ) );
	}
}


void TabContents::search( const QString& text )
{
	QList<QTreeWidgetItem*> items = tree->findItems( text, Qt::MatchWildcard | Qt::MatchRecursive );

	if ( items.isEmpty() )
		return;

	TreeItem_TOC* treeitem = ( TreeItem_TOC* ) items.first();
	::mainWindow->activateUrl( treeitem->getUrl() );
}

void TabContents::focus()
{
	if ( !tree->hasFocus() )
		tree->setFocus();
}
