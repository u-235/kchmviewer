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

#include <QWebEngineSettings>

#include "../config.h"
#include "../mainwindow.h"
#include "../viewwindow.h"
#include "../viewwindowmgr.h"


// A small overriden class to handle a middle click
class ViewWindowTabWidget : public QTabWidget
{
	public:
		ViewWindowTabWidget( QWidget* parent ) : QTabWidget( parent ) {}

	protected:
		void mouseReleaseEvent ( QMouseEvent* event )
		{
			if ( event->button() == Qt::MidButton )
			{
				int tab = tabBar()->tabAt( event->pos() );

				if ( tab != -1 )
					emit tabCloseRequested( tab );
			}
		}
};

ViewWindowMgr::ViewWindowMgr( QWidget* parent )
	: QWidget( parent ), Ui::TabbedBrowser()
{
	// UIC
	setupUi( this );

	// Set up the initial settings
	applyBrowserSettings();

	// Create the tab widget
	m_tabWidget = new ViewWindowTabWidget( this );
	verticalLayout->insertWidget( 0, m_tabWidget, 10 );

	// on current tab changed
	connect( m_tabWidget, SIGNAL( currentChanged( int ) ), this, SLOT( onTabChanged( int ) ) );
	connect( m_tabWidget, SIGNAL( tabCloseRequested( int ) ), this, SLOT( onCloseWindow( int ) ) );

	// Create a "new tab" button
	QToolButton* newButton = new QToolButton( this );
	newButton->setCursor( Qt::ArrowCursor );
	newButton->setAutoRaise( true );
	newButton->setIcon( QIcon( ":/images/addtab.png" ) );
	newButton->setToolTip( i18n( "Add page" ) );
	connect( newButton, SIGNAL( clicked() ), this, SLOT( openNewTab() ) );

	// Put it there
	m_tabWidget->setCornerWidget( newButton, Qt::TopLeftCorner );

	// Hide the search frame
	frameFind->setVisible( false );

	// Search Line edit
	connect( editFind,
			 SIGNAL( textEdited ( const QString& ) ),
			 this,
			 SLOT( editTextEdited( const QString& ) ) );

	connect( editFind, SIGNAL( returnPressed() ), this, SLOT( onFindNext() ) );

	// Search toolbar buttons
	toolClose->setShortcut( Qt::Key_Escape );
	connect( toolClose, SIGNAL( clicked() ), this, SLOT( closeSearch() ) );

	connect( toolPrevious, SIGNAL( clicked() ), this, SLOT( onFindPrevious() ) );
	connect( toolNext, SIGNAL( clicked() ), this, SLOT( onFindNext() ) );
}

ViewWindowMgr::~ViewWindowMgr( )
{
}

void ViewWindowMgr::createMenu( MainWindow*, QMenu* menuWindow, QAction* actionCloseWindow )
{
	m_menuWindow = menuWindow;
	m_actionCloseWindow = actionCloseWindow;
}

void ViewWindowMgr::invalidate()
{
	closeAllWindows();
	addNewTab( true );
}

ViewWindow* ViewWindowMgr::current()
{
	TabData* tab = findTab( m_tabWidget->currentWidget() );

	if ( !tab )
		abort();

	return tab->window;
}

ViewWindow* ViewWindowMgr::addNewTab( bool set_active )
{
	ViewWindow* viewvnd = new ViewWindow( m_tabWidget );

	editFind->installEventFilter( this );

	// Create the tab data structure
	TabData tabdata;
	tabdata.window = viewvnd;
	tabdata.action = new QAction( "window", this ); // temporary name; real name is set in setTabName
	tabdata.widget = viewvnd;

	connect( tabdata.action,
			 SIGNAL( triggered() ),
			 this,
			 SLOT( activateWindow() ) );

	m_Windows.push_back( tabdata );
	m_tabWidget->addTab( tabdata.widget, "" );
	Q_ASSERT( m_Windows.size() == m_tabWidget->count() );

	// Set active if it is the first tab
	if ( set_active || m_Windows.size() == 1 )
		m_tabWidget->setCurrentWidget( tabdata.widget );

	// Handle clicking on link in browser window
	connect( viewvnd,
			 SIGNAL( linkClicked ( const QUrl& ) ),
			 ::mainWindow,
			 SLOT( activateUrl( const QUrl& ) ) );

	connect( viewvnd, SIGNAL( dataLoaded( ViewWindow* ) ), this, SLOT( onWindowContentChanged( ViewWindow* ) ) );

	// Set up the accelerator if we have room
	if ( m_Windows.size() < 10 )
		tabdata.action->setShortcut( QKeySequence( i18n( "Alt+%1" ).arg( m_Windows.size() ) ) );

	// Add it to the "Windows" menu
	m_menuWindow->addAction( tabdata.action );

	return viewvnd;
}

void ViewWindowMgr::closeAllWindows( )
{
	while ( m_Windows.begin() != m_Windows.end() )
		closeWindow( m_Windows.first().widget );
}

void ViewWindowMgr::setTabName( ViewWindow* window )
{
	TabData* tab = findTab( window );

	if ( tab )
	{
		QString title = window->title().trimmed();

		// Trim too long string
		if ( title.length() > 25 )
			title = title.left( 22 ) + "...";

		m_tabWidget->setTabText( m_tabWidget->indexOf( window ), title );
		tab->action->setText( title );

		updateCloseButtons();
	}
}

void ViewWindowMgr::onCloseCurrentWindow( )
{
	// Do not allow to close the last window
	if ( m_Windows.size() == 1 )
		return;

	TabData* tab = findTab( m_tabWidget->currentWidget() );
	closeWindow( tab->widget );
}

void ViewWindowMgr::onCloseWindow( int num )
{
	// Do not allow to close the last window
	if ( m_Windows.size() == 1 )
		return;

	TabData* tab = findTab( m_tabWidget->widget( num ) );

	if ( tab )
		closeWindow( tab->widget );
}

void ViewWindowMgr::closeWindow( QWidget* widget )
{
	WindowsIterator it;

	for ( it = m_Windows.begin(); it != m_Windows.end(); ++it )
		if ( it->widget == widget )
			break;

	if ( it == m_Windows.end() )
		qFatal( "ViewWindowMgr::closeWindow called with unknown widget!" );

	m_menuWindow->removeAction( it->action );

	m_tabWidget->removeTab( m_tabWidget->indexOf( it->widget ) );
	delete it->window;
	delete it->action;

	m_Windows.erase( it );
	updateCloseButtons();

	// Change the accelerators, as we might have removed the item in the middle
	int count = 1;

	for ( WindowsIterator it = m_Windows.begin(); it != m_Windows.end() && count < 10; ++it, count++ )
		( *it ).action->setShortcut( QKeySequence( i18n( "Alt+%1" ).arg( count ) ) );
}

void ViewWindowMgr::restoreSettings( const Settings::viewindow_saved_settings_t& settings )
{
	// Destroy automatically created tab
	closeWindow( m_Windows.first().widget );

	for ( int i = 0; i < settings.size(); i++ )
	{
		ViewWindow* window = addNewTab( false );
		window->openUrl( settings[i].url ); // will call setTabName()
		window->setScrollbarPosition( settings[i].scroll_y );
		window->setZoomFactor( settings[i].zoom );
	}
}

void ViewWindowMgr::saveSettings( Settings::viewindow_saved_settings_t& settings )
{
	settings.clear();

	for ( int i = 0; i < m_tabWidget->count(); i++ )
	{
		QWidget* p = m_tabWidget->widget( i );
		TabData* tab = findTab( p );

		if ( !tab )
			abort();

		settings.push_back( Settings::SavedViewWindow( tab->window->getOpenedPage().toString(),
							tab->window->getScrollbarPosition(),
							tab->window->getZoomFactor() ) );
	}
}

void ViewWindowMgr::updateCloseButtons( )
{
	bool enabled = m_Windows.size() > 1;

	m_actionCloseWindow->setEnabled( enabled );
	m_tabWidget->setTabsClosable( enabled );
}

void ViewWindowMgr::onTabChanged( int newtabIndex )
{
	if ( newtabIndex == -1 )
		return;

	TabData* tab = findTab( m_tabWidget->widget( newtabIndex ) );

	if ( tab )
	{
		tab->window->updateHistoryIcons();
		mainWindow->browserChanged( tab->window );
		tab->widget->setFocus();
	}
}

void ViewWindowMgr::openNewTab()
{
	::mainWindow->openPage( current()->getOpenedPage(), MainWindow::OPF_NEW_TAB | MainWindow::OPF_CONTENT_TREE );
}

void ViewWindowMgr::activateWindow()
{
	QAction* action = qobject_cast< QAction* >( sender() );

	for ( WindowsIterator it = m_Windows.begin(); it != m_Windows.end(); ++it )
	{
		if ( it->action != action )
			continue;

		QWidget* widget = it->widget;
		m_tabWidget->setCurrentWidget( widget );
		break;
	}
}

void ViewWindowMgr::closeSearch()
{
	frameFind->hide();
	m_tabWidget->currentWidget()->setFocus();
}

ViewWindowMgr::TabData* ViewWindowMgr::findTab( QWidget* widget )
{
	for ( WindowsIterator it = m_Windows.begin(); it != m_Windows.end(); ++it )
		if ( it->widget == widget )
			return ( it.operator->() );

	return 0;
}

void ViewWindowMgr::setCurrentPage( int index )
{
	m_tabWidget->setCurrentIndex( index );
}

int ViewWindowMgr::currentPageIndex() const
{
	return m_tabWidget->currentIndex();
}

void ViewWindowMgr::onActivateFind()
{
	frameFind->show();
	labelWrapped->setVisible( false );
	editFind->setFocus( Qt::ShortcutFocusReason );
	editFind->selectAll();
}

void ViewWindowMgr::find( bool backward )
{
	QWebEnginePage::FindFlags webkitflags = 0;

	if ( checkCase->isChecked() )
		webkitflags |= QWebEnginePage::FindCaseSensitively;

	if ( backward )
		webkitflags |= QWebEnginePage::FindBackward;

	if ( pConfig->m_browserHighlightSearchResults )
	{
		// From the doc:
		// If the HighlightAllOccurrences flag is passed, the
		// function will highlight all occurrences that exist
		// in the page. All subsequent calls will extend the
		// highlight, rather than replace it, with occurrences
		// of the new string.

		// If the search text is different, we run the empty string search
		// to discard old highlighting
		if ( m_lastSearchedWord != editFind->text() )
			current()->findText( "", webkitflags );// FIXME | QWebEnginePage::HighlightAllOccurrences

		m_lastSearchedWord = editFind->text();

		// Now we call search with highlighting enabled, while the main search below will have
		// it disabled. This leads in both having the highlighting results AND working forward/
		// backward buttons.
		current()->findText( editFind->text(), webkitflags );// FIXME  | QWebEnginePage::HighlightAllOccurrences
	}

	// Pre-hide the wrapper
	labelWrapped->hide();

	current()->findText( editFind->text(), webkitflags, [ = ]( bool found )
	{
		// If we didn't find anything, enable the wrap and try again
		if ( !found )
		{
			current()->findText( editFind->text(), webkitflags, [ = ]( bool found )
			{
				if ( found )
					labelWrapped->show();
			} );
		}

		if ( !frameFind->isVisible() )
			frameFind->show();

		QPalette p = editFind->palette();

		if ( !found )
			p.setColor( QPalette::Active, QPalette::Base, QColor( 255, 102, 102 ) );
		else
			p.setColor( QPalette::Active, QPalette::Base, Qt::white );

		editFind->setPalette( p );
	} );
}

void ViewWindowMgr::editTextEdited( const QString& )
{
	find();
}

void ViewWindowMgr::onFindNext()
{
	find();
}

void ViewWindowMgr::onFindPrevious()
{
	find( true );
}

void ViewWindowMgr::onWindowContentChanged( ViewWindow* window )
{
	setTabName( ( ViewWindow* ) window );
}

void ViewWindowMgr::copyUrlToClipboard()
{
	QString url = current()->getOpenedPage().toString();

	if ( !url.isEmpty() )
		QApplication::clipboard()->setText( url );
}


void ViewWindowMgr::applyBrowserSettings()
{
	QWebEngineSettings* setup = QWebEngineSettings::globalSettings();

	setup->setAttribute( QWebEngineSettings::AutoLoadImages, pConfig->m_browserEnableImages );
	setup->setAttribute( QWebEngineSettings::JavascriptEnabled, pConfig->m_browserEnableJS );
	setup->setAttribute( QWebEngineSettings::PluginsEnabled, pConfig->m_browserEnablePlugins );
	setup->setAttribute( QWebEngineSettings::LocalStorageEnabled, pConfig->m_browserEnableLocalStorage );
}
