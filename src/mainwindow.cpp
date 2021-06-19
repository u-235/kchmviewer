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

#include <QProcess>
#include <QDesktopServices>
#include <QSettings>
#include <QDateTime>
#include <QTextStream>

#include "kde-qt.h"

#include "mainwindow.h"
#include "config.h"
#include "settings.h"
#include "viewwindow.h"
#include "viewwindowmgr.h"
#include "dialog_setup.h"
#include "recentfiles.h"
#include "navigationpanel.h"
#include "toolbarmanager.h"
#include "version.h"
#include "textencodings.h"
#include "ui_dialog_about.h"

#ifdef Q_WS_X11
	#include <QX11Info>
	#include <X11/Xlib.h>
#endif

// Maximum memory size for inter-application communication
static const int SHARED_MEMORY_SIZE = 4096;

static const unsigned int WINDOW_DEFAULT_X_SIZE = 900;
static const unsigned int WINDOW_DEFAULT_Y_SIZE = 700;


MainWindow::MainWindow( const QStringList& arguments )
	: QMainWindow ( 0 ), Ui::MainWindow()
{
	const unsigned int SPLT_X_SIZE = 300;

	m_arguments = arguments;

	// Delete the pointer when the window is closed
	setAttribute( Qt::WA_DeleteOnClose );

	// UIC stuff
	setupUi( this );

	// Set up layout direction
	if ( pConfig->m_advLayoutDirectionRL )
		qApp->setLayoutDirection( Qt::RightToLeft );
	else
		qApp->setLayoutDirection( Qt::LeftToRight );

	m_ebookFile = 0;
	m_autoteststate = STATE_OFF;
	m_sharedMemory = 0;

	m_currentSettings = new Settings();

	// Create the view window, which is a central widget
	m_viewWindowMgr = new ViewWindowMgr( this );
	setCentralWidget( m_viewWindowMgr );

	// Create a navigation panel
	m_navPanel = new NavigationPanel( this );

	// Add navigation dock
	m_navPanel->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
	addDockWidget( Qt::LeftDockWidgetArea, m_navPanel, Qt::Vertical );

	// Set up toolbar manager
	m_toolbarMgr = new ToolbarManager( this );

	m_toolbarMgr->queryAvailableActions( this );
	m_toolbarMgr->addManaged( mainToolbar );
	m_toolbarMgr->addManaged( navToolbar );
	m_toolbarMgr->addManaged( viewToolbar );
	m_toolbarMgr->load();

	// Set up other things
	setupActions();
	updateToolbars();
	setupLangEncodingMenu();

	// Resize main window and dock
	resize( WINDOW_DEFAULT_X_SIZE, WINDOW_DEFAULT_Y_SIZE );
	m_navPanel->resize( SPLT_X_SIZE, m_navPanel->height() );

	statusBar()->show();

	qApp->setWindowIcon( QPixmap( ":/images/kchmviewer.png" ) );

	if ( pConfig->m_numOfRecentFiles > 0 )
	{
		m_recentFiles = new RecentFiles( menu_File, file_exit_action, pConfig->m_numOfRecentFiles );
		connect( m_recentFiles, SIGNAL( openRecentFile( QString ) ), this, SLOT( actionOpenRecentFile( QString ) ) );
	}
	else
		m_recentFiles = 0;

	// Basically disable everything
	updateActions();
}

MainWindow::~MainWindow()
{
	// Temporary files cleanup
	while ( !m_tempFileKeeper.isEmpty() )
		delete m_tempFileKeeper.takeFirst();

	delete m_sharedMemory;
}

void MainWindow::launch()
{
	QTimer::singleShot( 0, this, SLOT( firstShow() ) );
}

bool MainWindow::hasSameTokenInstance()
{
	// Find out if token has been specified as this would mean we're running in a single instance mode
	QString token;

	// argv[0] in Qt is still a program name
	for ( int i = 1; i < m_arguments.size(); i++  )
	{
		// This is not bulletproof (think -showPage -token) but this is not likely to happen
		if ( m_arguments[i] == "-token" )
		{
			token = m_arguments[++i];
			break;
		}
	}

	if ( token.isEmpty() )
		return false;

	m_sharedMemory = new QSharedMemory( token );

	// If we can attach to it, the segment already exists
	if ( m_sharedMemory->attach() )
	{
		// Another instance exists; send the command-line there
		QByteArray args = m_arguments.join( "|" ).toLocal8Bit();

		if ( args.size() < SHARED_MEMORY_SIZE - 2 )
		{
			// Write the size first, then the string
			if ( m_sharedMemory->lock() )
			{
				char* data = ( char* ) m_sharedMemory->data();
				*( ( short* )data ) = args.size();
				memcpy( data + 2, args.data(), args.size() );

				m_sharedMemory->unlock();
			}
			else
				qDebug( "failed to lock" );
		}

		// Clean up
		delete m_sharedMemory;
		m_sharedMemory = 0;

		return true;
	}

	// Create a new segment
	if ( !m_sharedMemory->create( SHARED_MEMORY_SIZE ) )
	{
		QMessageBox::critical( 0,
							   i18n( "Shared memory segment failed" ),
							   i18n( "Failed to create a shared memory segment: %1" ).arg( m_sharedMemory->errorString() ) );
		return false;
	}

	// Set it up so our checker knows there's no data yet
	*( ( short* ) m_sharedMemory->data() ) = 0;

	// Recheck every second
	QTimer* timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( checkForSharedMemoryMessage() ) );
	timer->start( 1000 );
	return false;
}

void MainWindow::checkForSharedMemoryMessage()
{
	QStringList args;
	m_sharedMemory->lock();

	// Is there any data?
	char* data = ( char* ) m_sharedMemory->data();

	if ( data[0] != 0 || data[1] != 0 )
	{
		// Get the message length and the message
		short len = *( ( short* ) data );
		args = QString::fromLocal8Bit( data + 2, len ).split( "|" );

		// Clean up
		*( ( short* ) data ) = 0;
	}

	m_sharedMemory->unlock();

	// And process it if we find anything
	if ( !args.isEmpty() )
		parseCmdLineArgs( args, true );
}

void MainWindow::checkNewVersionAvailable()
{
	// Create a New version available object if necessary. This object will auto-delete itself
	CheckNewVersion* pNewVer = new CheckNewVersion();

	connect( pNewVer, SIGNAL( error( int ) ), this, SLOT( newVerAvailError( int ) ) );
	connect( pNewVer, SIGNAL( newVersionAvailable( NewVersionMetaMap ) ), this,
			 SLOT( newVerAvailable( NewVersionMetaMap ) ) );

	pNewVer->setUrl( "http://www.kchmviewer.net/latestversion.txt" );
	pNewVer->start();
}


bool MainWindow::loadFile ( const QString& loadFileName, bool call_open_page )
{
	QString fileName = loadFileName;

	// Strip file:// prefix if any
	if ( fileName.startsWith( "file://" ) )
		fileName.remove( 0, 7 );

	EBook* new_ebook = EBook::loadFile( fileName );

	if ( new_ebook )
	{
		// The new file is opened, so we can close the old one
		if ( m_ebookFile )
		{
			closeFile( );
			delete m_ebookFile;
		}

		m_ebookFile = new_ebook;
		updateActions();

		// Show current encoding in status bar
		if ( m_ebookFile->hasFeature( EBook::FEATURE_ENCODING ) )
			showInStatusBar( i18n( "Detected file encoding: %1 ( %2 )" )
							 .arg( TextEncodings::languageForCodec( m_ebookFile->currentEncoding() ) )
							 .arg( m_ebookFile->currentEncoding() ) );

		// Make the file name absolute; we'll need it later
		QDir qd;
		qd.setPath ( fileName );
		m_ebookFilename = qd.absolutePath();

		// Qt's 'dirname' does not work well
		QFileInfo qf ( m_ebookFilename );
		pConfig->m_lastOpenedDir = qf.dir().path();
		m_ebookFileBasename = qf.fileName();

		// Apply settings to the navigation dock
		m_navPanel->updateTabs( m_ebookFile );

		// and to navigation buttons
		nav_actionPreviousPage->setEnabled( hasTableOfContents()  );
		nav_actionNextPageToc->setEnabled( hasTableOfContents() );

		navSetBackEnabled( false );
		navSetForwardEnabled( false );

		m_viewWindowMgr->invalidate();
		refreshCurrentBrowser();

		if ( m_currentSettings->loadSettings ( fileName ) )
		{
			if ( m_ebookFile->hasFeature( EBook::FEATURE_ENCODING ) )
				setTextEncoding( m_currentSettings->m_activeEncoding );

			m_navPanel->applySettings( m_currentSettings );

			if ( call_open_page )
			{
				m_viewWindowMgr->restoreSettings( m_currentSettings->m_viewwindows );
				m_viewWindowMgr->setCurrentPage( m_currentSettings->m_activetabwindow );

				if ( m_ebookFile->hasFeature( EBook::FEATURE_TOC ) )
					actionLocateInContentsTab();
			}

			// Restore the main window size
			resize( m_currentSettings->m_window_size_x, m_currentSettings->m_window_size_y );
			m_navPanel->resize( m_currentSettings->m_window_size_splitter, m_navPanel->height() );
			m_navPanel->setActive( NavigationPanel::TAB_CONTENTS );
		}
		else
		{
			m_navPanel->setActive( NavigationPanel::TAB_CONTENTS );

			if ( m_ebookFile->hasFeature( EBook::FEATURE_ENCODING ) )
				setTextEncoding( m_ebookFile->currentEncoding() );

			if ( call_open_page )
				openPage( m_ebookFile->homeUrl() );
		}

		// Disable the menu if ebook format doesn't support encoding changes
		view_Set_encoding_action->setEnabled( m_ebookFile->hasFeature( EBook::FEATURE_ENCODING ) );

		if ( m_recentFiles )
			m_recentFiles->setCurrentFile( m_ebookFilename );

		return true;
	}
	else
	{
		QMessageBox mbox(
			i18n( "%1 - failed to load the chm file" ) . arg( QCoreApplication::applicationName() ),
			i18n( "Unable to load the chm file %1" ) . arg( fileName ),
			QMessageBox::Critical,
			QMessageBox::Ok,
			Qt::NoButton,
			Qt::NoButton );
		mbox.exec();

		statusBar()->showMessage(
			i18n( "Could not load file %1" ).arg( fileName ),
			2000 );

		return false;
	}
}


void MainWindow::refreshCurrentBrowser( )
{
	QString title = m_ebookFile->title();

	if ( title.isEmpty() )
		title = QCoreApplication::applicationName();

	// KDE adds application name automatically, so we don't need it here
#if !defined (USE_KDE)
	else
		title = ( QString ) QCoreApplication::applicationName() + " - " + title;

#endif

	setWindowTitle( title );

	currentBrowser()->invalidate();

	m_navPanel->refresh();
}


void MainWindow::activateUrl( const QUrl& link )
{
	if ( link.isEmpty() )
		return;

	Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();

	if ( mods & Qt::ShiftModifier )
		openPage( link, OPF_NEW_TAB | OPF_CONTENT_TREE );
	else if ( mods & Qt::ControlModifier )
		openPage( link, OPF_NEW_TAB | OPF_BACKGROUND );
	else
		openPage( link, OPF_CONTENT_TREE );
}


bool MainWindow::openPage( const QUrl& url, unsigned int flags )
{
	QString otherlink;

	// Feed to the browser all non-internal URLs
	if ( !m_ebookFile->isSupportedUrl( url  ) )
	{
		switch ( pConfig->m_onExternalLinkClick )
		{
		case Config::ACTION_DONT_OPEN:
			break;

		case Config::ACTION_ASK_USER:
			if ( QMessageBox::question( this,
										i18n( "%1 - remote link clicked - %2" ) . arg( QCoreApplication::applicationName() ) . arg( otherlink ),
										i18n( "A remote link %1 will start the external program to open it.\n\nDo you want to continue?" ).arg(
											url.toString() ),
										i18n( "&Yes" ), i18n( "&No" ),
										QString::null, 0, 1 ) )
				return false;

		// no break! should continue to open.

		case Config::ACTION_ALWAYS_OPEN:
#if defined (USE_KDE)
			new KRun ( url, 0 );
#else
			QDesktopServices::openUrl( url );
#endif
			break;
		}

		return false; // do not change the current page.
	}

	ViewWindow* vwnd = currentBrowser();

	if ( flags & OPF_NEW_TAB )
	{
		vwnd = m_viewWindowMgr->addNewTab( !( flags & OPF_BACKGROUND ) );
		vwnd->setZoomFactor( currentBrowser()->zoomFactor() );
	}

	if ( vwnd->openUrl ( url ) )
	{
		// Open all the tree items to show current item (if needed)
		if ( ( flags & OPF_CONTENT_TREE ) != 0 )
			m_navPanel->findUrlInContents( url );
	}

	// Focus on the view window so keyboard scroll works; do not do it for the background tabs
	if ( ( flags & OPF_BACKGROUND ) == 0 )
		vwnd->setFocus( Qt::OtherFocusReason );

	return true;
}


void MainWindow::firstShow()
{
	if ( !parseCmdLineArgs( m_arguments ) )
	{
		if ( m_recentFiles && pConfig->m_startupMode == Config::STARTUP_LOAD_LAST_FILE
				&& !m_recentFiles->latestFile().isEmpty() )
		{
			loadFile( m_recentFiles->latestFile() );
			return;
		}

		if ( pConfig->m_startupMode == Config::STARTUP_POPUP_OPENFILE )
			actionOpenFile();
	}
}


void MainWindow::setTextEncoding( const QString& encoding )
{
	m_ebookFile->setCurrentEncoding( qPrintable( encoding ) );

	// Find the appropriate encoding item in "Set encodings" menu
	const QList<QAction*> encodings = m_encodingActions->actions();

	for ( QList<QAction*>::const_iterator it = encodings.begin();
			it != encodings.end();
			++it )
	{
		if ( ( *it )->data().toString() == encoding  )
		{
			if ( !( *it )->isChecked() )
				( *it )->setChecked( true );

			break;
		}
	}

	// Because updateView() will call view->invalidate(), which clears the view->getOpenedPage(),
	// we have to make a copy of it.
	QUrl url = currentBrowser()->getOpenedPage();

	// Regenerate the content and index trees
	refreshCurrentBrowser();

	currentBrowser()->openUrl( url );
}

void MainWindow::closeFile( )
{
	// Prepare the settings
	if ( pConfig->m_HistoryStoreExtra )
	{
		if ( m_ebookFile->hasFeature( EBook::FEATURE_ENCODING ) )
			m_currentSettings->m_activeEncoding = m_ebookFile->currentEncoding();

		m_currentSettings->m_activetabwindow = m_viewWindowMgr->currentPageIndex( );

		m_currentSettings->m_window_size_x = width();
		m_currentSettings->m_window_size_y = height();

#ifdef Q_OS_WIN

		// On Windows if the window is maximised or minimized, the WM will not restore positions/state,
		// so we reset to default size
		if ( isMaximized() || isMinimized() )
		{
			m_currentSettings->m_window_size_x = WINDOW_DEFAULT_X_SIZE;
			m_currentSettings->m_window_size_y = WINDOW_DEFAULT_Y_SIZE;
		}

#endif

		m_currentSettings->m_window_size_splitter = m_navPanel->width();

		m_navPanel->getSettings( m_currentSettings );

		m_viewWindowMgr->saveSettings( m_currentSettings->m_viewwindows );

		m_currentSettings->saveSettings( );
	}

	pConfig->save();
}


void MainWindow::closeEvent ( QCloseEvent* e )
{
	// Save the settings if we have something opened
	if ( m_ebookFile )
	{
		closeFile( );
		delete m_ebookFile;
		m_ebookFile = 0;
	}

	// Save toolbars
	m_toolbarMgr->save();

	QMainWindow::closeEvent ( e );
}

void MainWindow::printHelpAndExit()
{
	fprintf ( stderr, "Usage: %s [options] [helpfile]\n"
			  "    The following options supported:\n"
			  "  -showPage <url>   opens the url in the help file\n"
			  "  -index <text>     searches for text in the Index tab\n"
			  "  -search <query>   searches for query in the Search tab, and activate the first entry if found\n"
			  "  -token <token>    specifies the application token; see the integration reference\n"
			  "  -background       start minimized\n"
			  "  -novcheck         disable check for new version even if enabled in configuration\n"
			  , qPrintable( m_arguments[0] ) );

	exit ( 1 );
}

bool MainWindow::parseCmdLineArgs( const QStringList& args, bool from_another_app )
{
	QString filename, search_query, search_index, open_url, search_toc;
	bool do_autotest = false, disable_vcheck = false, force_background = false;

	// argv[0] in Qt is still a program name
	for ( int i = 1; i < args.size(); i++  )
	{
		if ( args[i] == "-h" || args[i] == "--help" )
			printHelpAndExit();
		else if ( args[i] == "--autotestmode" || args[i] == "--shortautotestmode" )
			do_autotest = true;
		else if ( args[i] == "--search" || args[i] == "-search" )
			search_query = args[++i];
		else if ( args[i] == "--sindex" || args[i] == "-index" )
			search_index = args[++i];
		else if ( args[i] == "--stoc" )
			search_toc = args[++i];
		else if ( args[i] == "-token" )
			i++; // ignore
		else if ( args[i] == "-background" )
			force_background = true;
		else if ( args[i] == "-novcheck" )
			disable_vcheck = true;
		else if ( args[i] == "-v" || args[i] == "--version" )
		{
			printf( "kchmviewer version %d.%d built at %s %s\n", APP_VERSION_MAJOR, APP_VERSION_MINOR, __DATE__, __TIME__ );
			exit( 0 );
		}
		else if ( args[i] == "--url" || args[i] == "-showPage" )
			open_url = args[++i];
		else
		{
			if ( filename.isEmpty() )
				filename = args[i];
			else
			{
				// Don't quit just because wrong CL was passed
				if ( from_another_app )
					return false;

				fprintf ( stderr, "Invalid command-line option %s (ebook filename is already specified as %s)\n",
						  qPrintable( filename ), qPrintable( args[i] ) );

				printHelpAndExit();
			}
		}
	}

	// Check for a new version if needed
	if ( pConfig->m_advCheckNewVersion && !disable_vcheck )
	{
		QSettings settings;

		if ( settings.contains( "advanced/lastupdate" ) )
		{
			QDateTime lastupdate = settings.value( "advanced/lastupdate" ).toDateTime();

			if ( lastupdate.secsTo( QDateTime::currentDateTime() ) >= 86400 * 7 ) // seven days
				checkNewVersionAvailable();
		}
	}

	// Opening the file?
	if ( !filename.isEmpty() )
	{
		// If we have already opened the same file, no need to reopen it again
		if ( !m_ebookFile || QDir( m_ebookFilename ) != QDir( filename ) )
			if ( !loadFile( filename ) )
				return true; // skip the latest checks, but do not exit from the program

		if ( !open_url.isEmpty() )
		{
			QStringList event_args;
			event_args.push_back( m_ebookFile->pathToUrl( open_url ).toString() );
			qApp->postEvent( this, new UserEvent( "openPage", event_args ) );
		}
		else if ( !search_index.isEmpty() )
		{
			QStringList event_args;
			event_args.push_back( search_index );
			qApp->postEvent( this, new UserEvent( "findInIndex", event_args ) );
		}
		else if ( !search_query.isEmpty() )
		{
			QStringList event_args;
			event_args.push_back( search_query );
			qApp->postEvent( this, new UserEvent( "searchQuery", event_args ) );
		}
		else if ( !search_toc.isEmpty() )
		{
			QStringList event_args;
			event_args.push_back( search_toc );
			qApp->postEvent( this, new UserEvent( "findInToc", event_args ) );
		}

		if ( do_autotest )
		{
			if ( filename.isEmpty() )
				qFatal ( "Could not use Auto Test mode without a chm file!" );

			m_autoteststate = STATE_INITIAL;
			showMinimized ();
			runAutoTest();
		}

		if ( force_background )
			showMinimized();
		else if ( from_another_app )
		{
#ifdef Q_WS_X11
			// On Linux - at least on KDE - activating the foreground window
			// via activateWindow(); raise(); only works twice. Then it does not
			// work anymore, most likely because of some internal counter in Qt.
			// The code below, however, works fine.
			Display* display = x11Info().display();
			WId win = winId();

			XEvent event = { 0 };
			event.xclient.type = ClientMessage;
			event.xclient.serial = 0;
			event.xclient.send_event = True;
			event.xclient.message_type = XInternAtom( display, "_NET_ACTIVE_WINDOW", False );
			event.xclient.window = win;
			event.xclient.format = 32;

			XSendEvent( display, DefaultRootWindow( display ), False, SubstructureRedirectMask | SubstructureNotifyMask, &event );
			XMapRaised( display, win );
#else
			// On Windows it is not possible to activate the window of a non-active process. From MSDN:
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ms633539%28v=vs.85%29.aspx
			//
			// An application cannot force a window to the foreground while the user is working with another window.
			// Instead, Windows flashes the taskbar button of the window to notify the user.
			activateWindow();
			raise();
			show();
#endif
		}

		return true;
	}

	return false;
}

ViewWindow* MainWindow::currentBrowser( ) const
{
	return m_viewWindowMgr->current();
}

void MainWindow::onOpenPageInNewTab( )
{
	openPage( currentBrowser()->getNewTabLink(), OPF_NEW_TAB | OPF_CONTENT_TREE );
}

void MainWindow::onOpenPageInNewBackgroundTab( )
{
	openPage( currentBrowser()->getNewTabLink(), OPF_NEW_TAB | OPF_BACKGROUND );
}

void MainWindow::browserChanged( ViewWindow* newbrowser )
{
	m_navPanel->findUrlInContents( newbrowser->getOpenedPage() );
}

bool MainWindow::event( QEvent* e )
{
	if ( e->type() == QEvent::User )
		return handleUserEvent( ( UserEvent* ) e );

	return QMainWindow::event( e );
}

bool MainWindow::handleUserEvent( const UserEvent* event )
{
	if ( event->m_action == "loadAndOpen" )
	{
		if ( event->m_args.size() != 1 && event->m_args.size() != 2 )
			qFatal( "handleUserEvent: event loadAndOpen must receive 1 or 2 args" );

		QString chmfile = event->m_args[0];
		QString openurl = event->m_args.size() > 1 ? event->m_args[1] : "/";

		return loadFile( chmfile, false ) && openPage( openurl );
	}
	else if ( event->m_action == "openPage" )
	{
		if ( event->m_args.size() != 1 )
			qFatal( "handleUserEvent: event openPage must receive 1 arg" );

		return openPage( event->m_args[0] );
	}
	else if ( event->m_action == "findInIndex" )
	{
		if ( event->m_args.size() != 1 )
			qFatal( "handleUserEvent: event findInIndex must receive 1 arg" );

		if ( !hasIndex() )
			return false;

		actionSwitchToIndexTab();
		m_navPanel->findInIndex( event->m_args[0] );
		return true;
	}
	else if ( event->m_action == "findInToc" )
	{
		if ( event->m_args.size() != 1 )
			qFatal( "handleUserEvent: event findInToc must receive 1 arg" );

		if ( !hasTableOfContents() )
			return false;

		actionSwitchToContentTab();
		m_navPanel->findTextInContents( event->m_args[0] );
		return true;
	}
	else if ( event->m_action == "searchQuery" )
	{
		if ( event->m_args.size() != 1 )
			qFatal( "handleUserEvent: event searchQuery must receive 1 arg" );

		actionSwitchToSearchTab();
		m_navPanel->executeQueryInSearch( event->m_args[0] );
		return true;
	}
	else
		qWarning( "Unknown user event received: %s", qPrintable( event->m_action ) );

	return false;
}


void MainWindow::runAutoTest()
{
	switch ( m_autoteststate )
	{
	case STATE_INITIAL:
		m_autoteststate = STATE_OPEN_INDEX;

		QTimer::singleShot ( 500, this, SLOT( runAutoTest() ) );
		break; // allow to finish the initialization sequence

	case STATE_OPEN_INDEX:
		if ( hasIndex() )
			m_navPanel->setActive( NavigationPanel::TAB_INDEX );

		m_autoteststate = STATE_SHUTDOWN;
		QTimer::singleShot ( 500, this, SLOT( runAutoTest() ) );
		break;

	case STATE_SHUTDOWN:
		qApp->quit();
		break;

	default:
		break;
	}
}

void MainWindow::showInStatusBar( const QString& text )
{
	statusBar()->showMessage( text, 2000 );
}

void MainWindow::actionNavigateBack()
{
	currentBrowser()->navigateBack();
}

void MainWindow::actionNavigateForward()
{
	currentBrowser()->navigateForward();
}

void MainWindow::actionNavigateHome()
{
	currentBrowser()->navigateHome();
}

void MainWindow::actionOpenFile()
{
#if defined (USE_KDE)
	QString fn = KFileDialog::getOpenFileName( pConfig->m_lastOpenedDir,
				 i18n( "*.chm|Compressed Help Manual;*.epub|EPUB electronic book" ), this );
#else
	QString fn = QFileDialog::getOpenFileName( this,
				 i18n( "Open a chm file" ),
				 pConfig->m_lastOpenedDir,
				 i18n( "Electronic books (*.chm *.epub)" ),
				 0,
				 QFileDialog::DontResolveSymlinks );
#endif

	if ( !fn.isEmpty() )
		loadFile( fn );
}

void MainWindow::actionPrint()
{
	currentBrowser()->printCurrentPage();
}

void MainWindow::actionEditCopy()
{
	currentBrowser()->clipCopy();
}

void MainWindow::actionEditSelectAll()
{
	currentBrowser()->clipSelectAll();
}

void MainWindow::actionFindInPage()
{
	m_viewWindowMgr->onActivateFind();
}

void MainWindow::actionChangeSettings()
{
	DialogSetup dlg ( this );

	dlg.exec();
}


void MainWindow::actionExtractCHM()
{
	QList< QUrl > files;

#if defined (USE_KDE)
	QString outdir = KFileDialog::getExistingDirectory (
						 KUrl(),
						 this,
						 i18n( "Choose a directory to store CHM content" ) );
#else
	QString outdir = QFileDialog::getExistingDirectory (
						 this,
						 i18n( "Choose a directory to store CHM content" ),
						 QString::null,
						 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
#endif

	if ( outdir.isEmpty() )
		return;

	outdir += "/";

	// Enumerate all the files in archive
	if ( !m_ebookFile || !m_ebookFile->enumerateFiles( files ) )
		return;

	KQProgressModalDialog progress( i18n( "Extracting CHM content" ),
									i18n( "Extracting files..." ),
									i18n( "Abort" ),
									files.size(),
									this );

	for ( int i = 0; i < files.size(); i++ )
	{
		progress.setValue( i );

		if ( ( i % 3 ) == 0 )
		{
			qApp->processEvents();

			if ( progress.wasCancelled() )
				break;
		}

		// Extract the file
		QByteArray buf;

		if ( m_ebookFile->getFileContentAsBinary( buf, files[i] ) )
		{
			// Split filename to get the list of subdirectories
			QStringList dirs = files[i].path().split( '/' );

			// Walk through the list of subdirectories, and create them if needed
			// dirlevel is used to detect extra .. and prevent overwriting files
			// outside the directory (like creating the file images/../../../../../etc/passwd
			int i, dirlevel = 0;
			QStringList dirlist;

			for ( i = 0; i < dirs.size() - 1; i++ )
			{
				// Skip .. which lead too far above
				if ( dirs[i] == ".." )
				{
					if ( dirlevel > 0 )
					{
						dirlevel--;
						dirlist.pop_back();
					}
				}
				else
				{
					dirlist.push_back( dirs[i] );

					QDir dir ( outdir + dirlist.join( "/" ) );

					if ( !dir.exists() )
					{
						if ( !dir.mkdir( dir.path() ) )
							qWarning( "Could not create subdir %s\n", qPrintable( dir.path() ) );
					}
				}
			}

			QString filename = outdir + dirlist.join( "/" ) + "/" + dirs[i];
			QFile wf( filename );

			if ( !wf.open( QIODevice::WriteOnly ) )
			{
				qWarning( "Could not write file %s\n", qPrintable( filename ) );
				continue;
			}

			wf. write( buf );
			wf.close();
		}
		else
			qWarning( "Could not get file %s\n", qPrintable( files[i].toString() ) );
	}

	progress.setValue( files.size() );
}

void MainWindow::actionFontSizeIncrease()
{
	currentBrowser()->zoomIncrease();
}

void MainWindow::actionFontSizeDecrease()
{
	currentBrowser()->zoomDecrease();
}

void MainWindow::actionViewHTMLsource()
{
	QString text;

	if ( !m_ebookFile->getFileContentAsString( text, currentBrowser()->getOpenedPage() ) || text.isEmpty() )
		return;

	if ( pConfig->m_advUseInternalEditor )
	{
		QTextEdit* editor = new QTextEdit ( 0 );
		editor->setPlainText( text );
		editor->setWindowTitle( i18n( "HTML source" ) );
		editor->resize( 800, 600 );
		editor->show();
	}
	else
	{
		QTemporaryFile* tf = new QTemporaryFile();
		m_tempFileKeeper.append( tf );

		if ( !tf->open() )
		{
			qWarning( "Cannot open created QTemporaryFile: something is wrong with your system" );
			return;
		}

		tf->write( text.toUtf8() );
		tf->seek( 0 );

		// Run the external editor
		QStringList arguments;
		arguments.push_back( tf->fileName() );

		if ( !QProcess::startDetached( pConfig->m_advExternalEditorPath, arguments, "." ) )
		{
			QMessageBox::warning( 0,
								  "Cannot start external editor",
								  tr( "Cannot start external editor %1.\nMake sure the path is absolute!" ) .arg( pConfig->m_advExternalEditorPath ) );
			delete m_tempFileKeeper.takeLast();
		}
	}
}

void MainWindow::actionToggleFullScreen()
{
	bool fullscreen = view_Toggle_fullscreen_action->isChecked();

	if ( fullscreen )
	{
		if ( !isFullScreen() )
		{
			showFullScreen ();

			// Hiding menu bar disables menu actions. Probably a bug in Qt.
			//menuBar()->hide();
			statusBar()->hide();
		}
	}
	else
	{
		if ( isFullScreen() )
		{
			showNormal();
			menuBar()->show();
			statusBar()->show();
		}
	}
}

void MainWindow::actionShowHideNavigator( bool toggle )
{
	if ( toggle )
		m_navPanel->show();
	else
		m_navPanel->hide();
}

void MainWindow::navigatorVisibilityChanged( bool visible )
{
	view_Show_navigator_window->setChecked( visible );
}

void MainWindow::actionLocateInContentsTab()
{
	if ( m_navPanel->findUrlInContents( currentBrowser()->getOpenedPage() ) )
		m_navPanel->setActive( NavigationPanel::TAB_CONTENTS );
	else
		statusBar()->showMessage( i18n( "Could not locate opened topic in content window" ), 2000 );
}


void MainWindow::actionAboutApp()
{
#if QT_VERSION >= 0x050000
	QString info = QString( "<br>Built for %1 arch using %2 ABI<br>Running on %3, Qt version %4" )
				   .arg( QSysInfo::buildCpuArchitecture() )
				   .arg( QSysInfo::buildAbi() )
				   .arg( QSysInfo::prettyProductName() )
				   .arg( qVersion() );
#else
	QString info = QString( "<br>Using Qt version %1" ) .arg( qVersion() );
#endif

	QString abouttext = i18n( "<html><b>kchmviewer version %1.%2</b>%3<br><br>"
							  "Copyright (C) George Yunaev, 2004-2015<br>"
							  "<a href=\"mailto:gyunaev@ulduzsoft.com\">gyunaev@ulduzsoft.com</a><br>"
							  "<a href=\"http://www.ulduzsoft.com/kchmviewer\">http://www.ulduzsoft.com/kchmviewer</a><br><br>"
							  "Licensed under GNU GPL license version 3.</html>" )
						.arg( APP_VERSION_MAJOR ) .arg( APP_VERSION_MINOR ) .arg( info );

	// It is quite funny that the argument order differs
#if defined (USE_KDE)
	KMessageBox::about( this, abouttext, i18n( "About kchmviewer" ) );
#else
	QDialog dlg;
	Ui::DialogAbout ui;

	ui.setupUi( &dlg );
	ui.lblAbout->setText( abouttext );
	dlg.exec();
#endif
}

void MainWindow::actionAboutQt()
{
	QMessageBox::aboutQt( this, QCoreApplication::applicationName() );
}

void MainWindow::actionSwitchToContentTab()
{
	m_navPanel->setActive( NavigationPanel::TAB_CONTENTS );
}

void MainWindow::actionSwitchToIndexTab()
{
	m_navPanel->setActive( NavigationPanel::TAB_INDEX );
}

void MainWindow::actionSwitchToSearchTab()
{
	m_navPanel->setActive( NavigationPanel::TAB_SEARCH );
}

void MainWindow::actionSwitchToBookmarkTab()
{
	m_navPanel->setActive( NavigationPanel::TAB_BOOKMARK );
}


void MainWindow::setupActions()
{
	// File menu
	connect( file_Open_action, SIGNAL( triggered() ), this, SLOT( actionOpenFile() ) );
	connect( file_Print_action, SIGNAL( triggered() ), this, SLOT( actionPrint() ) );
	connect( file_ExtractCHMAction, SIGNAL( triggered() ), this, SLOT( actionExtractCHM() ) );
	connect( file_exit_action, SIGNAL( triggered() ), qApp, SLOT( closeAllWindows() ) );

	// Edit
	connect( edit_Copy_action, SIGNAL( triggered() ), this, SLOT( actionEditCopy() ) );
	connect( edit_SelectAll_action, SIGNAL( triggered() ), this, SLOT( actionEditSelectAll() ) );
	connect( edit_FindAction, SIGNAL( triggered() ), this, SLOT( actionFindInPage() ) );

	// Bookmarks
	connect( bookmark_AddAction, SIGNAL( triggered() ), m_navPanel, SLOT( addBookmark() ) );

	// View
	connect( view_Increase_font_size_action, SIGNAL( triggered() ), this, SLOT( actionFontSizeIncrease() ) );
	connect( view_Decrease_font_size_action, SIGNAL( triggered() ), this, SLOT( actionFontSizeDecrease() ) );
	connect( view_View_HTML_source_action, SIGNAL( triggered() ), this, SLOT( actionViewHTMLsource() ) );
	connect( view_Toggle_fullscreen_action, SIGNAL( triggered() ), this, SLOT( actionToggleFullScreen() ) );
	connect( view_Show_navigator_window, SIGNAL( triggered( bool ) ), this, SLOT( actionShowHideNavigator( bool ) ) );
	connect( view_Locate_in_contents_action, SIGNAL( triggered() ), this, SLOT( actionLocateInContentsTab() ) );

	// Settings
	connect( settings_SettingsAction, SIGNAL( triggered() ), this, SLOT( actionChangeSettings() ) );
	connect( actionEdit_toolbars, SIGNAL( triggered() ), this, SLOT( actionEditToolbars() ) );
	connect( actionCheck_for_updates, SIGNAL( triggered() ), this, SLOT( checkNewVersionAvailable() ) );

	// Help menu
	connect( actionAbout_kchmviewer, SIGNAL( triggered() ), this, SLOT( actionAboutApp() ) );
	connect( actionAbout_Qt, SIGNAL( triggered() ), this, SLOT( actionAboutQt() ) );
	menuHelp->addSeparator();

	// Navigation toolbar
	connect( nav_action_Back, SIGNAL( triggered() ), this, SLOT( actionNavigateBack() ) );
	connect( nav_actionForward, SIGNAL( triggered() ), this, SLOT( actionNavigateForward() ) );
	connect( nav_actionHome, SIGNAL( triggered() ), this, SLOT( actionNavigateHome() ) );
	connect( nav_actionPreviousPage, SIGNAL( triggered() ), m_navPanel, SLOT( showPrevInToc() ) );
	connect( nav_actionNextPageToc, SIGNAL( triggered() ), m_navPanel, SLOT( showNextInToc() ) );

	// m_viewWindowMgr fills and maintains 'Window' menu
	m_viewWindowMgr->createMenu( this, menu_Windows, action_Close_window );

	m_navPanel->setBookmarkMenu( menu_Bookmarks );

	// Close Window goes directly to the window manager
	connect( action_Close_window, SIGNAL( triggered() ), m_viewWindowMgr, SLOT( onCloseCurrentWindow() ) );

	// Navigation panel visibility
	connect( m_navPanel, SIGNAL( visibilityChanged( bool ) ), this, SLOT( navigatorVisibilityChanged( bool ) ) );

	// "What's this" action
	QAction* whatsthis = QWhatsThis::createAction( this );
	menuHelp->addAction( whatsthis );
	viewToolbar->addAction( whatsthis );

	// Tab switching actions
	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+1" ) ),
							this,
							SLOT( actionSwitchToContentTab() ),
							SLOT( actionSwitchToContentTab() ),
							Qt::ApplicationShortcut );

	( void )  new QShortcut( QKeySequence( i18n( "Ctrl+2" ) ),
							 this,
							 SLOT( actionSwitchToIndexTab() ),
							 SLOT( actionSwitchToIndexTab() ),
							 Qt::ApplicationShortcut );

	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+3" ) ),
							this,
							SLOT( actionSwitchToSearchTab() ),
							SLOT( actionSwitchToSearchTab() ),
							Qt::ApplicationShortcut );

	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+4" ) ),
							this,
							SLOT( actionSwitchToBookmarkTab() ),
							SLOT( actionSwitchToBookmarkTab() ),
							Qt::ApplicationShortcut );

	// Find (/) global shortcut
	( void ) new QShortcut( QKeySequence( i18n( "/" ) ),
							m_viewWindowMgr,
							SLOT( onActivateFind() ),
							SLOT( onActivateFind() ),
							Qt::ApplicationShortcut );

	// Find next global shortcuts
	( void ) new QShortcut( QKeySequence( i18n( "F3" ) ),
							m_viewWindowMgr,
							SLOT( onFindNext() ),
							SLOT( onFindNext() ),
							Qt::ApplicationShortcut );

	( void ) new QShortcut( QKeySequence( QKeySequence::FindNext ),
							m_viewWindowMgr,
							SLOT( onFindNext() ),
							SLOT( onFindNext() ),
							Qt::ApplicationShortcut );

	// Find prev global shortcut
	( void ) new QShortcut( QKeySequence( QKeySequence::FindPrevious ),
							m_viewWindowMgr,
							SLOT( onFindPrevious() ),
							SLOT( onFindPrevious() ),
							Qt::ApplicationShortcut );

	// Open next page in TOC global shortcut
	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+Right" ) ),
							m_navPanel,
							SLOT( showNextInToc() ),
							SLOT( showNextInToc() ),
							Qt::ApplicationShortcut );

	// Open next page in TOC global shortcut
	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+Left" ) ),
							m_navPanel,
							SLOT( showPrevInToc() ),
							SLOT( showPrevInToc() ),
							Qt::ApplicationShortcut );

	// Copy current URL to clipboard global shortcut
	( void ) new QShortcut( QKeySequence( i18n( "Ctrl+I" ) ),
							m_viewWindowMgr,
							SLOT( copyUrlToClipboard() ),
							SLOT( copyUrlToClipboard() ),
							Qt::ApplicationShortcut );

	// Context menu
	m_contextMenu = new QMenu( this );

	m_contextMenu->addAction ( "&Open this link in a new tab",
							   this,
							   SLOT( onOpenPageInNewTab() ),
							   QKeySequence( "Shift+Enter" ) );

	m_contextMenu->addAction ( "&Open this link in a new background tab",
							   this,
							   SLOT( onOpenPageInNewBackgroundTab() ),
							   QKeySequence( "Ctrl+Enter" ) );
}

void MainWindow::updateToolbars()
{
	// Toolbars configuration
	Qt::ToolButtonStyle buttonstyle = Qt::ToolButtonIconOnly;
	QSize iconsize = QSize( 32, 32 );

	switch ( pConfig->m_toolbarMode )
	{
	case Config::TOOLBAR_SMALLICONS:
		iconsize = QSize( 16, 16 );
		break;

	case Config::TOOLBAR_LARGEICONS:
		break;

	case Config::TOOLBAR_LARGEICONSTEXT:
		buttonstyle = Qt::ToolButtonTextUnderIcon;
		break;

	case Config::TOOLBAR_TEXTONLY:
		buttonstyle = Qt::ToolButtonTextOnly;
		break;
	}

	mainToolbar->setIconSize( iconsize );
	mainToolbar->setToolButtonStyle( buttonstyle );
	navToolbar->setIconSize( iconsize );
	navToolbar->setToolButtonStyle( buttonstyle );
	viewToolbar->setIconSize( iconsize );
	viewToolbar->setToolButtonStyle( buttonstyle );
}


void MainWindow::navSetBackEnabled( bool enabled )
{
	nav_action_Back->setEnabled( enabled );
}

void MainWindow::navSetForwardEnabled( bool enabled )
{
	nav_actionForward->setEnabled( enabled );
}

void MainWindow::actionOpenRecentFile( const QString& file )
{
	loadFile( file );
}

void MainWindow::setupLangEncodingMenu()
{
	// Create the language selection menu.
	QMenu* encodings = new QMenu( this );

	// Create the action group
	m_encodingActions = new QActionGroup( this );

	// Get the supported languages and encodings
	QStringList languages, qencodings;
	TextEncodings::getSupported( languages, qencodings );

	for ( int idx = 0; idx < qencodings.size(); idx++ )
	{
		QAction* action = new QAction( this );

		QString text = i18n( "%1 ( %2 )" ) .arg( languages[idx] ) .arg( qencodings[idx] );
		action->setText( text );
		action->setData( qVariantFromValue( qencodings[idx] ) );
		action->setCheckable( true );

		// Add to the action group, so only one is checkable
		m_encodingActions->addAction( action );

		// Add to the menu
		encodings->addAction( action );
	}

	// Set up the Select Codepage action
	view_Set_encoding_action->setMenu( encodings );

	// Connect the action group signal
	connect( m_encodingActions,
			 SIGNAL( triggered ( QAction* ) ),
			 this,
			 SLOT( actionEncodingChanged( QAction* ) ) );
}


void MainWindow::actionEncodingChanged( QAction* action )
{
	QString encoding = action->data().toString();
	setTextEncoding( encoding );
}


QMenu* MainWindow::tabItemsContextMenu()
{
	return m_contextMenu;
}

void MainWindow::setupPopupMenu( QMenu* menu )
{
	menu->addAction( action_Close_window );
	menu->addSeparator();
	menu->addAction( nav_action_Back );
	menu->addAction( nav_actionForward );
	menu->addAction( nav_actionHome );
	menu->addSeparator();
	menu->addAction( nav_actionPreviousPage );
	menu->addAction( nav_actionNextPageToc );
	menu->addSeparator();
	menu->addAction( view_Increase_font_size_action );
	menu->addAction( view_Decrease_font_size_action );
	menu->addSeparator();
	menu->addAction( edit_Copy_action );
	menu->addAction( edit_FindAction );
}

bool MainWindow::hasTableOfContents() const
{
	return m_ebookFile && m_ebookFile->hasFeature( EBook::FEATURE_TOC );
}

bool MainWindow::hasIndex() const
{
	return m_ebookFile && m_ebookFile->hasFeature( EBook::FEATURE_INDEX );
}

const QPixmap* MainWindow::getEBookIconPixmap( EBookTocEntry::Icon imagenum )
{
	if ( m_builtinIcons[imagenum].isNull() )
	{
		QString resicon = QString( ":/chm_icons/icon_%1.png" ) .arg( imagenum );

		if ( !m_builtinIcons[imagenum].load( resicon ) )
			qFatal( "Could not initialize the internal icon %d as %s", imagenum, qPrintable( resicon ) );
	}

	return &( m_builtinIcons[imagenum] );
}

void MainWindow::updateActions()
{
	bool enabled = m_ebookFile != 0;

	file_Print_action->setEnabled( enabled );
	edit_Copy_action->setEnabled( enabled );
	edit_SelectAll_action->setEnabled( enabled );
	edit_FindAction->setEnabled( enabled );
	file_ExtractCHMAction->setEnabled( enabled );
	bookmark_AddAction->setEnabled( enabled );
	view_Increase_font_size_action->setEnabled( enabled );
	view_Decrease_font_size_action->setEnabled( enabled );
	view_View_HTML_source_action->setEnabled( enabled );
	view_Locate_in_contents_action->setEnabled( enabled );
	view_Set_encoding_action->setEnabled( enabled );
	action_Close_window->setEnabled( enabled );
	nav_action_Back->setEnabled( enabled );
	nav_actionForward->setEnabled( enabled );
	nav_actionHome->setEnabled( enabled );
	nav_actionPreviousPage->setEnabled( enabled );
	nav_actionNextPageToc->setEnabled( enabled );
	m_navPanel->setEnabled( enabled );
}

void MainWindow::newVerAvailError( int  )
{
	statusBar()->showMessage( tr( "Unable to check whether a new version is available" ), 2000 );
}

void MainWindow::newVerAvailable( NewVersionMetaMap metadata )
{
	QSettings().setValue( "advanced/lastupdate", QDateTime::currentDateTime() );

	// What is the latest version?
	QString current = QString( "%1.%2" ) .arg( APP_VERSION_MAJOR ) .arg( APP_VERSION_MINOR );

	if ( metadata["Version"].toFloat() > current.toFloat() )
	{
		if ( QMessageBox::question( 0,
									tr( "New version available" ),
									tr( "<html>A new version <b>%1</b> of Kchmviewer is available!<br><br>"
										"You are currently using version %3.<br>"
										"Do you want to visit the application web site %2?" )
									.arg( metadata["Version"] )
									.arg( metadata["URL"] )
									.arg( current ),
									QMessageBox::Yes | QMessageBox::No,
									QMessageBox::Yes ) == QMessageBox::No )
			return;

		QDesktopServices::openUrl ( QUrl( metadata["URL"] ) );
	}
	else
		statusBar()->showMessage( tr( "Checked for updates; you are using the latest version of kchmviewer" ), 2000 );
}

void MainWindow::actionEditToolbars()
{
	m_toolbarMgr->editDialog();
}
