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
#include "config.h"
#include "dialog_setup.h"
#include "mainwindow.h"
#include "version.h"


DialogSetup::DialogSetup( QWidget* parent )
	: QDialog( parent ), Ui::DialogSetup()
{
	setupUi( this );

	connect( btnBrowse, SIGNAL( clicked() ), this, SLOT( browseExternalEditor() ) );

	// Set up the parameters
	switch ( pConfig->m_startupMode )
	{
	case Config::STARTUP_DO_NOTHING:
		rbStartWithNothing->setChecked( true );
		break;

	case Config::STARTUP_LOAD_LAST_FILE:
		m_radioOnBeginOpenLast->setChecked( true );
		break;

	case Config::STARTUP_POPUP_OPENFILE:
		m_radioOnBeginOpenDialog->setChecked( true );
		break;
	}

	m_historySize->setValue ( pConfig->m_numOfRecentFiles );
	m_rememberHistoryInfo->setChecked ( pConfig->m_HistoryStoreExtra );

	m_radioExtLinkOpenAlways->setChecked ( pConfig->m_onExternalLinkClick == Config::ACTION_ALWAYS_OPEN );
	m_radioExtLinkAsk->setChecked ( pConfig->m_onExternalLinkClick == Config::ACTION_ASK_USER );
	m_radioExtLinkOpenNever->setChecked ( pConfig->m_onExternalLinkClick == Config::ACTION_DONT_OPEN );
	m_highlightSearchResults->setChecked( pConfig->m_browserHighlightSearchResults );

	m_radioNewChmOpenAlways->setChecked ( pConfig->m_onNewChmClick == Config::ACTION_ALWAYS_OPEN );
	m_radioNewChmAsk->setChecked ( pConfig->m_onNewChmClick == Config::ACTION_ASK_USER );
	m_radioNewChmOpenNever->setChecked ( pConfig->m_onNewChmClick == Config::ACTION_DONT_OPEN );

	m_advExternalProgramName->setText( pConfig->m_advExternalEditorPath );
	m_advViewSourceExternal->setChecked ( !pConfig->m_advUseInternalEditor );
	m_advViewSourceInternal->setChecked ( pConfig->m_advUseInternalEditor );

	m_numOfRecentFiles = pConfig->m_numOfRecentFiles;

	boxAutodetectEncoding->setChecked( pConfig->m_advAutodetectEncoding );
	boxLayoutDirectionRL->setChecked( pConfig->m_advLayoutDirectionRL );

	// Browser settings
	m_enableImages->setChecked( pConfig->m_browserEnableImages );
	m_enableJS->setChecked( pConfig->m_browserEnableJS );
	m_enableJava->setChecked( pConfig->m_browserEnableJava );
	m_enablePlugins->setChecked( pConfig->m_browserEnablePlugins );
	m_enableOfflineStorage->setChecked( pConfig->m_browserEnableOfflineStorage );
	m_enableLocalStorage->setChecked( pConfig->m_browserEnableLocalStorage );
	m_openAllTOCEntries->setChecked( pConfig->m_tocOpenAllEntries );
	boxUseSingleClick->setChecked( pConfig->m_tabUseSingleClick );

	switch ( pConfig->m_toolbarMode )
	{
	case Config::TOOLBAR_SMALLICONS:
		rbToolbarSmall->setChecked( true );
		break;

	case Config::TOOLBAR_LARGEICONS:
		rbToolbarLarge->setChecked( true );
		break;

	case Config::TOOLBAR_LARGEICONSTEXT:
		rbToolbarLargeText->setChecked( true );
		break;

	case Config::TOOLBAR_TEXTONLY:
		rbToolbarText->setChecked( true );
		break;
	}

	cbCheckForUpdates->setChecked( pConfig->m_advCheckNewVersion );
}

DialogSetup::~DialogSetup()
{
}


static void inline Check_Need_Restart( QCheckBox* box, bool* confsetting, bool* need_restart )
{
	if ( *confsetting != box->isChecked() )
	{
		*need_restart = true;
		*confsetting = box->isChecked();
	}
}

void DialogSetup::accept()
{
	if ( rbStartWithNothing->isChecked() )
		pConfig->m_startupMode = Config::STARTUP_DO_NOTHING;
	else if ( m_radioOnBeginOpenLast->isChecked() )
		pConfig->m_startupMode = Config::STARTUP_LOAD_LAST_FILE;
	else
		pConfig->m_startupMode = Config::STARTUP_POPUP_OPENFILE;

	pConfig->m_numOfRecentFiles = m_historySize->value();
	pConfig->m_HistoryStoreExtra = m_rememberHistoryInfo->isChecked();
	pConfig->m_browserHighlightSearchResults = m_highlightSearchResults->isChecked();
	pConfig->m_tocOpenAllEntries = m_openAllTOCEntries->isChecked();

	if ( m_radioExtLinkOpenAlways->isChecked () )
		pConfig->m_onExternalLinkClick = Config::ACTION_ALWAYS_OPEN;
	else if ( m_radioExtLinkAsk->isChecked () )
		pConfig->m_onExternalLinkClick = Config::ACTION_ASK_USER;
	else
		pConfig->m_onExternalLinkClick = Config::ACTION_DONT_OPEN;

	if ( m_radioNewChmOpenAlways->isChecked () )
		pConfig->m_onNewChmClick = Config::ACTION_ALWAYS_OPEN;
	else if ( m_radioNewChmAsk->isChecked () )
		pConfig->m_onNewChmClick = Config::ACTION_ASK_USER;
	else
		pConfig->m_onNewChmClick = Config::ACTION_DONT_OPEN;

	// Check the changes
	bool need_restart = false;

	Check_Need_Restart( m_enableImages, &pConfig->m_browserEnableImages, &need_restart );
	Check_Need_Restart( m_enableJS, &pConfig->m_browserEnableJS, &need_restart );
	Check_Need_Restart( m_enableJava, &pConfig->m_browserEnableJava, &need_restart );
	Check_Need_Restart( m_enablePlugins, &pConfig->m_browserEnablePlugins, &need_restart );
	Check_Need_Restart( m_enableOfflineStorage, &pConfig->m_browserEnableOfflineStorage, &need_restart );
	Check_Need_Restart( m_enableLocalStorage, &pConfig->m_browserEnableLocalStorage, &need_restart );
	Check_Need_Restart( boxUseSingleClick, &pConfig->m_tabUseSingleClick, &need_restart );

	Config::ToolbarMode newmode;

	if ( rbToolbarSmall->isChecked() )
		newmode = Config::TOOLBAR_SMALLICONS;
	else if ( rbToolbarLarge->isChecked() )
		newmode = Config::TOOLBAR_LARGEICONS;
	else if ( rbToolbarLargeText->isChecked() )
		newmode = Config::TOOLBAR_LARGEICONSTEXT;
	else
		newmode = Config::TOOLBAR_TEXTONLY;

	if ( newmode != pConfig->m_toolbarMode )
	{
		pConfig->m_toolbarMode = newmode;
		::mainWindow->updateToolbars();
	}

	pConfig->m_advExternalEditorPath = m_advExternalProgramName->text();
	pConfig->m_advUseInternalEditor = m_advViewSourceExternal->isChecked();
	pConfig->m_advUseInternalEditor = m_advViewSourceInternal->isChecked();

	if ( pConfig->m_numOfRecentFiles != m_numOfRecentFiles )
		need_restart = true;

	// Autodetect encoding
	Check_Need_Restart( boxAutodetectEncoding, &pConfig->m_advAutodetectEncoding, &need_restart );
	pConfig->m_advCheckNewVersion = cbCheckForUpdates->isChecked();

	// Layout direction management
	bool layout_rl = boxLayoutDirectionRL->isChecked();

	if ( layout_rl != pConfig->m_advLayoutDirectionRL )
	{
		pConfig->m_advLayoutDirectionRL = layout_rl;
		need_restart = true;
	}

	pConfig->save();

	if ( need_restart )
		QMessageBox::information( this,
								  QCoreApplication::applicationName(),
								  i18n( "Changing those options requires restarting the application to take effect." )	);

	QDialog::accept();
}


void DialogSetup::browseExternalEditor()
{
#if defined (USE_KDE)
	QString exec = KFileDialog::getOpenFileName( KUrl(), i18n( "*|Executables" ), this,
				   i18n( "Choose an editor executable" ) );
#else
	QString exec = QFileDialog::getOpenFileName( this,
				   i18n( "Choose an editor executable" ),
				   QString::null,
				   i18n( "Executables (*)" ) );
#endif

	if ( !exec.isEmpty() )
		m_advExternalProgramName->setText( exec );
}
