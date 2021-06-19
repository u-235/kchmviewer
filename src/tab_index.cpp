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

#include "mainwindow.h"
#include "treeitem_index.h"
#include "tab_index.h"
#include "config.h"

TabIndex::TabIndex ( QWidget* parent )
	: QWidget( parent ), Ui::TabIndex()
{
	// UIC stuff
	setupUi( this );

	tree->headerItem()->setHidden( true );

	connect( text,
			 SIGNAL( textChanged ( const QString& ) ),
			 this,
			 SLOT( onTextChanged( const QString& ) ) );

	connect( text,
			 SIGNAL( returnPressed() ),
			 this,
			 SLOT( onReturnPressed() ) );

	if ( pConfig->m_tabUseSingleClick )
	{
		connect( tree,
				 SIGNAL( itemClicked( QTreeWidgetItem*, int ) ),
				 this,
				 SLOT( onItemActivated( QTreeWidgetItem*, int ) ) );
	}
	else
	{
		connect( tree,
				 SIGNAL( itemActivated ( QTreeWidgetItem*, int ) ),
				 this,
				 SLOT( onItemActivated( QTreeWidgetItem*, int ) ) );
	}

	// Activate custom context menu, and connect it
	tree->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( tree,
			 SIGNAL( customContextMenuRequested ( const QPoint& ) ),
			 this,
			 SLOT( onContextMenuRequested( const QPoint& ) ) );

	m_indexListFilled = false;
	m_lastSelectedItem = 0;
	m_contextMenu = 0;

	focus();
}

void TabIndex::onTextChanged ( const QString& newvalue )
{
	QList<QTreeWidgetItem*> items = tree->findItems( newvalue, Qt::MatchStartsWith );

	if ( !items.isEmpty() )
	{
		m_lastSelectedItem = items[0];
		tree->setCurrentItem( m_lastSelectedItem );
		tree->scrollToItem( m_lastSelectedItem );
	}
	else
		m_lastSelectedItem = 0;
}


void TabIndex::showEvent( QShowEvent* )
{
	if ( !::mainWindow->chmFile() || m_indexListFilled )
		return;

	m_indexListFilled = true;
	refillIndex();
}

void TabIndex::onReturnPressed( )
{
	if ( !m_lastSelectedItem )
		return;

	TreeItem_Index* treeitem = ( TreeItem_Index* ) m_lastSelectedItem;
	::mainWindow->activateUrl( treeitem->getUrl() );
}


void TabIndex::invalidate( )
{
	tree->clear();
	m_indexListFilled = false;
	m_lastSelectedItem = 0;
}

void TabIndex::onItemActivated ( QTreeWidgetItem* item, int )
{
	if ( !item )
		return;

	TreeItem_Index* treeitem = ( TreeItem_Index* ) item;

	// Prevent opened index tree item from closing; because the tree open/close
	// procedure will be triggered after the slots are called, we change the tree
	// state to "collapsed", so the slot handler expands it again.
	if ( item->isExpanded() )
		item->setExpanded( false );

	QUrl url = treeitem->getUrl();

	if ( !url.isValid() )
		return;

	if ( treeitem->isSeeAlso() ) // 'see also' link
	{
		QList<QTreeWidgetItem*> items = tree->findItems( treeitem->seeAlso(), Qt::MatchFixedString );

		if ( !items.isEmpty() )
		{
			m_lastSelectedItem = items[0];
			tree->setCurrentItem( m_lastSelectedItem );
			tree->scrollToItem( m_lastSelectedItem );
		}
		else
			m_lastSelectedItem = 0;
	}
	else
		::mainWindow->openPage( url, MainWindow::OPF_CONTENT_TREE );
}


void TabIndex::refillIndex( )
{
	ShowWaitCursor wc;
	QList< EBookIndexEntry > data;

	if ( !::mainWindow->chmFile()->getIndex( data ) || data.size() == 0 )
	{
		qWarning ( "CHM index present but is empty; wrong parsing?" );
		return;
	}

	QVector< TreeItem_Index*> lastchild;
	QVector< TreeItem_Index*> rootentry;
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
		TreeItem_Index* item;

		if ( indent == 0 )
			item = new TreeItem_Index( tree, lastchild[indent], data[i].name, data[i].urls, data[i].seealso );
		else
		{
			// New non-root entry. It is possible (for some buggy CHMs) that there is no previous entry: previoous entry had indent 1,
			// and next entry has indent 3. Backtracking it up, creating missing entries.
			if ( rootentry[indent - 1] == 0 )
				qFatal( "Child entry indented as %d with no root entry!", indent );

			item = new TreeItem_Index( rootentry[indent - 1], lastchild[indent], data[i].name, data[i].urls, data[i].seealso );
		}

		// Make it open
		item->setExpanded( true );

		lastchild[indent] = item;
		rootentry[indent] = item;
	}

	tree->update();
}

void TabIndex::search( const QString& index )
{
	if ( !::mainWindow->chmFile() )
		return;

	if ( !m_indexListFilled )
	{
		m_indexListFilled = true;
		refillIndex();
	}

	text->setText( index );
	onTextChanged( index );
}

void TabIndex::focus()
{
	if ( !tree->hasFocus() )
		tree->setFocus();
}

void TabIndex::onContextMenuRequested( const QPoint& point )
{
	TreeItem_Index* treeitem = ( TreeItem_Index* ) tree->itemAt( point );

	if ( treeitem )
	{
		::mainWindow->currentBrowser()->setTabKeeper( treeitem->getUrl() );
		::mainWindow->tabItemsContextMenu()->popup( tree->viewport()->mapToGlobal( point ) );
	}
}
