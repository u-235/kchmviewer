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
#include "viewwindow.h"
#include "version.h"
#include "config.h"
#include "tab_bookmarks.h"

class BookmarkItem : public QListWidgetItem
{
	public:
		BookmarkItem( TabBookmarks* widget, QListWidget* parent, const QString& name, const QString& url, int pos )
			: QListWidgetItem( parent )
		{
			m_name = name;
			m_url = url;
			m_scroll_y = pos;
			m_action = new QAction( name, widget );
			m_action->setData( qVariantFromValue( ( void* ) this ) );

			QObject::connect( m_action,
							  SIGNAL( triggered() ),
							  widget,
							  SLOT( actionBookmarkActivated() ) );
		}

		void setName( const QString& name )
		{
			m_name = name;
		}

		// Visualization
		virtual QVariant data ( int role ) const
		{
			switch ( role )
			{
			case Qt::ToolTipRole:
			case Qt::WhatsThisRole:
			case Qt::DisplayRole:
				return m_name;
			}

			return QVariant();
		}

		QString		m_name;
		QString		m_url;
		int			m_scroll_y;
		QAction* 	m_action;
};



TabBookmarks::TabBookmarks( QWidget* parent )
	: QWidget( parent ), Ui::TabBookmarks()
{
	// UIC code
	setupUi( this );

	if ( pConfig->m_tabUseSingleClick )
	{
		connect( list,
				 SIGNAL( itemClicked( QListWidgetItem* ) ),
				 this,
				 SLOT( onItemActivated( QListWidgetItem* ) ) );
	}
	else
	{
		connect( list,
				 SIGNAL( itemActivated( QListWidgetItem* ) ),
				 this,
				 SLOT( onItemActivated( QListWidgetItem* ) ) );
	}

	connect( btnAdd,
			 SIGNAL( clicked () ),
			 this,
			 SLOT( onAddBookmarkPressed( ) ) );

	connect( btnDel,
			 SIGNAL( clicked () ),
			 this,
			 SLOT( onDelBookmarkPressed( ) ) );

	connect( btnEdit,
			 SIGNAL( clicked () ),
			 this,
			 SLOT( onEditBookmarkPressed( ) ) );

	m_menuBookmarks = 0;
	m_contextMenu = 0;
	m_listChanged = false;

	// Activate custom context menu, and connect it
	list->setContextMenuPolicy( Qt::CustomContextMenu );

	connect( list,
			 SIGNAL( customContextMenuRequested ( const QPoint& ) ),
			 this,
			 SLOT( onContextMenuRequested( const QPoint& ) ) );

	focus();
}

void TabBookmarks::onAddBookmarkPressed( )
{
	bool ok;
	QString url = ::mainWindow->currentBrowser()->getOpenedPage().toString();
	QString title = ::mainWindow->chmFile()->getTopicByUrl( url );
	QString name = QInputDialog::getText(
					   this,
					   i18n( "%1 - add a bookmark" ) . arg( QCoreApplication::applicationName() ),
					   i18n( "Enter the name for this bookmark:" ),
					   QLineEdit::Normal,
					   title,
					   &ok );

	if ( !ok || name.isEmpty() )
		return;

	BookmarkItem* item = new BookmarkItem ( this,
											list,
											name,
											url,
											::mainWindow->currentBrowser()->getScrollbarPosition() );

	m_menuBookmarks->addAction( item->m_action );
	m_listChanged = true;
}


void TabBookmarks::onDelBookmarkPressed( )
{
	BookmarkItem* item = ( BookmarkItem* ) list->currentItem();

	if ( item )
	{
		m_menuBookmarks->removeAction( item->m_action );
		delete item;
		m_listChanged = true;
	}
}


void TabBookmarks::onEditBookmarkPressed( )
{
	BookmarkItem* item = ( BookmarkItem* ) list->currentItem();

	if ( item )
	{
		bool ok;
		QString name = QInputDialog::getText(
						   this,
						   i18n( "%1 - edit the bookmark name" ) . arg( QCoreApplication::applicationName() ),
						   i18n( "Enter the name for this bookmark:" ),
						   QLineEdit::Normal,
						   item->m_name,
						   &ok );

		if ( !ok || name.isEmpty() )
			return;

		item->setName( name );
		item->m_action->setText( name );
		m_listChanged = true;
		update();
	}
}


void TabBookmarks::restoreSettings( const Settings::bookmark_saved_settings_t& settings )
{
	for ( int i = 0; i < settings.size(); i++ )
	{
		BookmarkItem* item = new BookmarkItem( this, list, settings[i].name, settings[i].url, settings[i].scroll_y );
		m_menuBookmarks->addAction( item->m_action );
	}
}


void TabBookmarks::saveSettings( Settings::bookmark_saved_settings_t& settings )
{
	settings.clear();

	for ( int i = 0; i < list->count(); i++ )
	{
		BookmarkItem* treeitem = ( BookmarkItem* ) list->item( i );
		settings.push_back( Settings::SavedBookmark( treeitem->m_name, treeitem->m_url, treeitem->m_scroll_y ) );
	}
}

void TabBookmarks::invalidate( )
{
	for ( int i = 0; i < list->count(); i++ )
		m_menuBookmarks->removeAction( ( ( BookmarkItem* ) list->item( i ) )->m_action );

	list->clear();
}

void TabBookmarks::focus()
{
	if ( list->hasFocus() )
		list->setFocus();
}

void TabBookmarks::createMenu( QMenu* menuBookmarks )
{
	m_menuBookmarks = menuBookmarks;
}

void TabBookmarks::onItemActivated( QListWidgetItem* item )
{
	if ( !item )
		return;

	BookmarkItem* treeitem = ( BookmarkItem* ) item;

	if ( ::mainWindow->currentBrowser()->getOpenedPage().toString() != treeitem->m_url )
	{
		::mainWindow->openPage( treeitem->m_url, MainWindow::OPF_CONTENT_TREE );
		::mainWindow->currentBrowser()->setScrollbarPosition( treeitem->m_scroll_y );
	}
	else
	{
		// Force the scroll since the page is not reloaded
		::mainWindow->currentBrowser()->setScrollbarPosition( treeitem->m_scroll_y, true );
	}
}

void TabBookmarks::actionBookmarkActivated()
{
	QAction* action = qobject_cast< QAction* >( sender() );

	BookmarkItem* item = ( BookmarkItem* ) action->data().value< void* > ();
	onItemActivated( item );
}

void TabBookmarks::onContextMenuRequested( const QPoint& point )
{
	BookmarkItem* item = ( BookmarkItem* ) list->itemAt( point );

	if ( item )
	{
		::mainWindow->currentBrowser()->setTabKeeper( item->m_url );
		::mainWindow->tabItemsContextMenu()->popup( list->viewport()->mapToGlobal( point ) );
	}
}
