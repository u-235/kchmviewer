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

#include <QHeaderView>

#include "mainwindow.h"
#include "config.h"
#include "tab_search.h"
#include "ebook_search.h"


class SearchTreeViewItem : public QTreeWidgetItem
{
	public:
		SearchTreeViewItem( QTreeWidget* tree, const QString& name, const QUrl& url )
			:	QTreeWidgetItem( tree ), m_name( name ), m_url( url ) {}

		QUrl	getUrl() const
		{
			return m_url;
		}

	protected:
		// Overriden members
		int columnCount () const
		{
			return 2;
		}

		// Overriden member
		QVariant data ( int column, int role ) const
		{
			switch ( role )
			{
			// Item name
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
			case Qt::WhatsThisRole:
				if ( column == 0 )
					return m_name;
				else
					return m_url.path();
			}

			return QVariant();
		}

	private:
		QString		m_name;
		QUrl		m_url;
};



TabSearch::TabSearch( QWidget* parent )
	: QWidget( parent ), Ui::TabSearch()
{
	// UIC stuff
	setupUi( this );

	// Clickable Help label
	connect( lblHelp,
			 SIGNAL( linkActivated( const QString& ) ),
			 this,
			 SLOT( onHelpClicked( const QString& ) ) );

	// Go Button
	connect( btnGo,
			 SIGNAL( clicked () ),
			 this,
			 SLOT( onReturnPressed() ) );

	// Pressing 'Return' in the combo box line edit
	connect( searchBox->lineEdit(),
			 SIGNAL( returnPressed() ),
			 this,
			 SLOT( onReturnPressed() ) );

	// Clicking on tree element
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

	focus();

	m_contextMenu = 0;
	m_genIndexProgress = 0;
	m_searchEngineInitDone = false;

	m_searchEngine = new EBookSearch();
	connect( m_searchEngine, SIGNAL( progressStep( int, const QString& ) ), this, SLOT( onProgressStep( int,
			 const QString& ) ) );
}


void TabSearch::invalidate( )
{
	tree->clear();
	searchBox->clear();
	searchBox->lineEdit()->clear();

	delete m_genIndexProgress;
	m_genIndexProgress = 0;

	m_searchEngineInitDone = false;
}


void TabSearch::onReturnPressed( )
{
	QList<QUrl> results;
	QString text = searchBox->lineEdit()->text();

	if ( text.isEmpty() )
		return;

	tree->clear();

	if ( searchQuery( text, &results ) )
	{
		if ( !results.empty() )
		{
			for ( int i = 0; i < results.size(); i++ )
			{
				SearchTreeViewItem* item = new SearchTreeViewItem ( tree,
						::mainWindow->chmFile()->getTopicByUrl( results[i] ),
						results[i] );

				if ( i == 0 )
					tree->setCurrentItem( item );
			}

			::mainWindow->showInStatusBar( i18n( "Search returned %1 result(s)" ) . arg( results.size() ) );
			tree->setFocus();
		}
		else
			::mainWindow->showInStatusBar( i18n( "Search returned no results" ) );
	}
	else
		::mainWindow->showInStatusBar( i18n( "Search failed" ) );
}


void TabSearch::onItemActivated( QTreeWidgetItem* item, int )
{
	if ( !item )
		return;

	SearchTreeViewItem* treeitem = ( SearchTreeViewItem* ) item;
	::mainWindow->currentBrowser()->openUrl( treeitem->getUrl() );
}


void TabSearch::restoreSettings( const Settings::search_saved_settings_t& settings )
{
	for ( int i = 0; i < settings.size(); i++ )
		searchBox->addItem ( settings[i] );
}


void TabSearch::saveSettings( Settings::search_saved_settings_t& settings )
{
	settings.clear();

	for ( int i = 0; i < searchBox->count(); i++ )
		settings.push_back( searchBox->itemText( i ) );
}


void TabSearch::onHelpClicked( const QString& )
{
	QWhatsThis::showText ( mapToGlobal( lblHelp->pos() ),
						   i18n( "<html><p>The improved search engine allows you to search for a word, symbol or phrase, which is set of words and symbols included in quotes. Only the documents which include all the terms specified in th search query are shown; no prefixes needed.<p>Unlike MS CHM internal search index, my improved search engine indexes everything, including special symbols. Therefore it is possible to search (and find!) for something like <i>$q = new ChmFile();</i>. This search also fully supports Unicode, which means that you can search in non-English documents.<p>If you want to search for a quote symbol, use quotation mark instead. The engine treats a quote and a quotation mark as the same symbol, which allows to use them in phrases.</html>" ) );
}


bool TabSearch::initSearchEngine( )
{
	ShowWaitCursor waitcursor;

	QString indexfile = ::mainWindow->currentSettings()->searchIndexFile();

	// First try to read the index if exists
	QFile file( indexfile );

	if ( file.open( QIODevice::ReadOnly ) )
	{
		QDataStream stream( &file );

		::mainWindow->statusBar()->showMessage( i18n( "Reading dictionary..." ) );
		qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

		if ( m_searchEngine->loadIndex( stream ) )
		{
			m_searchEngineInitDone = true;
			return true;
		}
	}

	// So the index cannot be read or does not exist. Create a new one.

	// Show the user what we gonna do
	m_genIndexProgress = new QProgressDialog( this );
	m_genIndexProgress->setWindowTitle( i18n( "Generating search index..." ) );
	m_genIndexProgress->setLabelText( i18n( "Generating search index..." ) );
	m_genIndexProgress->setMaximum( 100 );
	m_genIndexProgress->reset();
	m_genIndexProgress->show();

	::mainWindow->statusBar()->showMessage( tr( "Generating search index..." ) );

	// Show 'em
	qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

	// Since we gonna save it, reopen the file
	file.close();

	if ( !file.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( 0, "Cannot save index", tr( "The index cannot be saved into file %1" ) .arg( file.fileName() ) );
		return false;
	}

	// Run the generation
	QDataStream stream( &file );

	m_searchEngine->generateIndex( ::mainWindow->chmFile(), stream );

	delete m_genIndexProgress;
	m_genIndexProgress = 0;

	if ( m_searchEngine->hasIndex() )
	{
		m_searchEngineInitDone = true;
		return true;
	}

	m_searchEngineInitDone = false;
	return false;
}


void TabSearch::execSearchQueryInGui( const QString& query )
{
	searchBox->lineEdit()->setText( query );
	onReturnPressed();
}


bool TabSearch::searchQuery( const QString& query, QList< QUrl >* results )
{
	if ( !m_searchEngineInitDone )
	{
		if ( !initSearchEngine() )
			return false;
	}

	if ( !m_searchEngine->hasIndex() )
	{
		QMessageBox::information ( this, "No index present", "The index is not present" );
		return false;
	}

	if ( query.isEmpty() )
		return false;

	ShowWaitCursor waitcursor;
	bool result;

	result = m_searchEngine->searchQuery( query, results, ::mainWindow->chmFile() );
	return result;
}

void TabSearch::focus()
{
	if ( !searchBox->hasFocus() && !tree->hasFocus() )
		searchBox->setFocus();
}

void TabSearch::onContextMenuRequested( const QPoint& point )
{
	SearchTreeViewItem* treeitem = ( SearchTreeViewItem* ) tree->itemAt( point );

	if ( treeitem )
	{
		::mainWindow->currentBrowser()->setTabKeeper( treeitem->getUrl() );
		::mainWindow->tabItemsContextMenu()->popup( tree->viewport()->mapToGlobal( point ) );
	}
}


void TabSearch::onProgressStep( int value, const QString& stepName )
{
	if ( m_genIndexProgress )
	{
		m_genIndexProgress->setLabelText( stepName );
		m_genIndexProgress->setValue( value );
	}
}
