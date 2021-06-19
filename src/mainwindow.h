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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSharedMemory>
#include "kde-qt.h"
#include "ebook.h"
#include "viewwindow.h"
#include "checknewversion.h"

#include "ui_mainwindow.h"


//! Those events could be sent to main window to do useful things. See handleUserEvents()
class UserEvent : public QEvent
{
	public:
		UserEvent( const QString& action, const QStringList& args = QStringList() )
			: QEvent( QEvent::User ), m_action( action ), m_args( args )
		{
		}

		QString			m_action;
		QStringList		m_args;
};


class RecentFiles;
class ToolbarManager;
class NavigationPanel;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
		Q_OBJECT

	public:
		// "Open page" parameter flags
		enum
		{
			OPF_CONTENT_TREE = 1 << 0,	//! Locate this page in the content tree
			OPF_NEW_TAB = 1 << 2,		//! Open the page in a new tab
			OPF_BACKGROUND 	= 1 << 3	//! Open the page in a new tab in background
		};

	public:
		MainWindow( const QStringList& arguments );
		~MainWindow();

		bool		openPage ( const QUrl& url, unsigned int flags = OPF_CONTENT_TREE );

		EBook*		chmFile() const
		{
			return m_ebookFile;
		}
		const QString&	getOpenedFileName ()
		{
			return m_ebookFilename;
		}
		const QString&	getOpenedFileBaseName ()
		{
			return m_ebookFileBasename;
		}

		ViewWindow* currentBrowser() const;
		Settings*    currentSettings() const
		{
			return m_currentSettings;
		}
		ViewWindowMgr*	viewWindowMgr() const
		{
			return m_viewWindowMgr;
		}
		NavigationPanel* navigator() const
		{
			return m_navPanel;
		}

		void		showInStatusBar ( const QString& text );
		void		setTextEncoding ( const QString& enc );
		QMenu* 	tabItemsContextMenu();
		void        launch();

		// Returns true if there's another instance running with the same token (assuming there's token);
		// also sends the command-line data there so it runs properly.
		bool        hasSameTokenInstance();

		// Called from WindowMgr when another browser tab is activated
		void		browserChanged( ViewWindow* newbrowser );

		// Adds some main window actions to the provided popup menu
		void		setupPopupMenu( QMenu* menu );

		// Returns true if currently opened file has TOC/index
		bool		hasTableOfContents() const;
		bool		hasIndex() const;

		// Gets the appropriate CHM pixmap icon (there are no icons in EPUB).
		const QPixmap* getEBookIconPixmap( EBookTocEntry::Icon imagenum );

	public slots:
		// Navigation toolbar icons
		void		navSetBackEnabled( bool enabled );
		void		navSetForwardEnabled( bool enabled );

		void 		onOpenPageInNewTab();
		void 		onOpenPageInNewBackgroundTab();

		// Actions
		void		actionOpenFile();
		void		actionPrint();
		void		actionEditCopy();
		void		actionEditSelectAll();
		void		actionFindInPage();
		void		actionExtractCHM();
		void		actionChangeSettings();
		void		actionFontSizeIncrease();
		void		actionFontSizeDecrease();
		void		actionViewHTMLsource();
		void		actionToggleFullScreen();
		void		actionShowHideNavigator( bool );
		void		navigatorVisibilityChanged( bool visible );
		void		actionLocateInContentsTab();
		void		actionEditToolbars();

		void		actionNavigateBack();
		void		actionNavigateForward();
		void		actionNavigateHome();

		void		actionAboutApp();
		void		actionAboutQt();

		void		actionSwitchToContentTab();
		void		actionSwitchToIndexTab();
		void		actionSwitchToSearchTab();
		void		actionSwitchToBookmarkTab();

		void		actionOpenRecentFile( const QString& file );
		void		actionEncodingChanged( QAction* action );

		// Link activation
		void 		activateUrl( const QUrl& link );

		void		updateToolbars();
		void		updateActions();

		void		checkNewVersionAvailable();

	protected slots:
		// Called from the timer in main constructor
		void 		firstShow();

		// checknewversion
		void		newVerAvailError( int  );
		void		newVerAvailable( NewVersionMetaMap metadata );

		// single app mode
		void        checkForSharedMemoryMessage();

	protected:
		// Reimplemented functions
		void		closeEvent ( QCloseEvent* e );
		bool		event ( QEvent* e );

	private:
		bool		parseCmdLineArgs( const QStringList& args, bool from_another_app = false );
		void 		setupActions();
		void		setupLangEncodingMenu();

		bool		loadFile( const QString& fileName,  bool call_open_page = true );
		void		closeFile();
		void		refreshCurrentBrowser();

		bool		handleUserEvent( const UserEvent* event );
		void        printHelpAndExit();

	private:
		QString 				m_ebookFilename;
		QString 				m_ebookFileBasename;

		Settings*				m_currentSettings;
		EBook*					m_ebookFile;

		QList<QTemporaryFile*>	m_tempFileKeeper;

		QActionGroup*			m_encodingActions;
		QMenu*					m_contextMenu;

		RecentFiles*				m_recentFiles;

		ViewWindowMgr*			m_viewWindowMgr;
		NavigationPanel*			m_navPanel;
		ToolbarManager*			m_toolbarMgr;

		// For a single instance mode
		QSharedMemory*          m_sharedMemory;

		// Storage for built-in icons
		QPixmap				 	m_builtinIcons[ EBookTocEntry::MAX_BUILTIN_ICONS ];

		// Storage for command-line arguments due to KDE really insisting to use its own way
		// for command-line parsing.
		QStringList             m_arguments;

	private:
		// This is used for application automatic testing
		enum	auto_test_state_t
		{
			STATE_OFF,
			STATE_INITIAL,
			STATE_OPEN_INDEX,
			STATE_SHUTDOWN
		};

		auto_test_state_t			m_autoteststate;

	private slots:
		void	runAutoTest();

};

extern MainWindow* mainWindow;

#endif // MAINWINDOW_H
