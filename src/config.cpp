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

#include <QTextStream>
#include <QFile>
#include <QDir>

#include "kde-qt.h"
#include "config.h"
#include "settings.h"
#include "mainwindow.h"

Config* pConfig;

Config::Config()
{
	// Support for portable app - if the data path is specified in the configuration, use it.
	m_datapath = QCoreApplication::applicationDirPath() + QDir::separator() + "portable";

	if ( QFile( m_datapath ).exists() )
	{
		QSettings::setPath( QSettings::defaultFormat(), QSettings::UserScope, m_datapath );
		m_datapath += QDir::separator() + QString( "data" );
	}
	else
		m_datapath = QDir::homePath () + "/" + ".kchmviewer";

	QSettings settings;

	m_startupMode = ( Config::StartupMode ) settings.value( "settings/onstartup", STARTUP_DO_NOTHING ).toInt();
	m_onNewChmClick = ( Config::choose_action_t ) settings.value( "settings/onnewchm", ACTION_ASK_USER ).toInt();
	m_onExternalLinkClick = ( Config::choose_action_t ) settings.value( "settings/onexternal", ACTION_ASK_USER ).toInt();
	m_numOfRecentFiles = settings.value( "settings/maxrecentfiles", 10 ).toInt();
	m_HistoryStoreExtra = settings.value( "settings/extrahistory", true ).toBool();
	m_advUseInternalEditor = settings.value( "advanced/internaleditor", true ).toBool();
	m_advLayoutDirectionRL = settings.value( "advanced/layoutltr", false ).toBool();
	m_advAutodetectEncoding = settings.value( "advanced/autodetectenc", false ).toBool();
	m_advExternalEditorPath = settings.value( "advanced/editorpath", "/usr/bin/kate" ).toString();
	m_advCheckNewVersion = settings.value( "advanced/checknewver", true ).toBool();
	m_toolbarMode = ( Config::ToolbarMode ) settings.value( "advanced/toolbarmode", TOOLBAR_LARGEICONSTEXT ).toInt();
	m_lastOpenedDir = settings.value( "advanced/lastopendir", "." ).toString();

	m_browserEnableJS = settings.value( "browser/enablejs", true ).toBool();
	m_browserEnableJava = settings.value( "browser/enablejava", false ).toBool();
	m_browserEnablePlugins = settings.value( "browser/enableplugins", true ).toBool();
	m_browserEnableImages  = settings.value( "browser/enableimages", true ).toBool();
	m_browserEnableOfflineStorage = settings.value( "browser/enableofflinestorage", false ).toBool();
	m_browserEnableLocalStorage = settings.value( "browser/enablelocalstorage", false ).toBool();
	m_browserEnableRemoteContent = settings.value( "browser/enableremotecontent", false ).toBool();
	m_browserHighlightSearchResults = settings.value( "browser/highlightsearchresults", true ).toBool();

	m_tocOpenAllEntries = settings.value( "browser/tocopenallentries", true ).toBool();
	m_tabUseSingleClick = settings.value( "browser/tabusesingleclick", true ).toBool();

	QDir dir;
	dir.setPath ( m_datapath );

	if ( !dir.exists() && !dir.mkdir( m_datapath ) )
		qWarning( "Could not create directory %s", qPrintable( m_datapath ) );
}


void Config::save( )
{
	QSettings settings;

	settings.setValue( "settings/onstartup", m_startupMode );
	settings.setValue( "settings/onnewchm", m_onNewChmClick );
	settings.setValue( "settings/onexternal", m_onExternalLinkClick );
	settings.setValue( "settings/maxrecentfiles", m_numOfRecentFiles );
	settings.setValue( "settings/extrahistory", m_HistoryStoreExtra );
	settings.setValue( "advanced/internaleditor", m_advUseInternalEditor );
	settings.setValue( "advanced/layoutltr", m_advLayoutDirectionRL );
	settings.setValue( "advanced/autodetectenc", m_advAutodetectEncoding );
	settings.setValue( "advanced/editorpath", m_advExternalEditorPath );
	settings.setValue( "advanced/checknewver", m_advCheckNewVersion );
	settings.setValue( "advanced/toolbarmode", m_toolbarMode );
	settings.setValue( "advanced/lastopendir", m_lastOpenedDir );

	settings.setValue( "browser/enablejs", m_browserEnableJS );
	settings.setValue( "browser/enablejava", m_browserEnableJava );
	settings.setValue( "browser/enableplugins", m_browserEnablePlugins );
	settings.setValue( "browser/enableimages", m_browserEnableImages );
	settings.setValue( "browser/enableofflinestorage", m_browserEnableOfflineStorage );
	settings.setValue( "browser/enablelocalstorage", m_browserEnableLocalStorage );
	settings.setValue( "browser/enableremotecontent", m_browserEnableRemoteContent );
	settings.setValue( "browser/highlightsearchresults", m_browserHighlightSearchResults );

	settings.setValue( "browser/tocopenallentries", m_tocOpenAllEntries );
	settings.setValue( "browser/tabusesingleclick", m_tabUseSingleClick );
}

QString Config::getEbookSettingFile( const QString& ebookfile ) const
{
	QFileInfo finfo ( ebookfile );
	QString prefix = pConfig->m_datapath + QDir::separator() + finfo.completeBaseName();

	return prefix + ".kchmviewer";
}

QString Config::getEbookIndexFile( const QString& ebookfile ) const
{
	QFileInfo finfo ( ebookfile );
	QString prefix = pConfig->m_datapath + "/" + finfo.completeBaseName();

	return prefix + ".idx";
}
