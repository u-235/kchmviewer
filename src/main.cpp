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
#include "config.h"
#include "dbus_interface.h"
#include "version.h"

#if !defined (WIN32)
	#include <QtDBus/QtDBus>
#endif

#if defined (USE_KDE)
	#include <kaboutdata.h>
#endif

#if defined (Q_OS_MAC)
        #include "kchmviewerapp.h"
#else
        typedef QApplication  KchmviewerApp;
#endif

MainWindow * mainWindow;


int main( int argc, char ** argv )
{
#if defined (USE_KDE)
    KAboutData aboutdata ( "kchmviewer",
                           QByteArray(),
                           ki18n("kchmviewer"),
                           qPrintable( QString("%1.%2") .arg(APP_VERSION_MAJOR) .arg(APP_VERSION_MINOR) ),
                           ki18n("CHM file viewer"),
                           KAboutData::License_GPL,
                           ki18n("(c) 2004-2015 George Yunaev, gyunaev@ulduzsoft.com"),
                           ki18n("Please report bugs to kchmviewer@ulduzsoft.com"),
                           "http://www.ulduzsoft.com/kchmviewer",
                           "kchmviewer@ulduzsoft.com");

    KCmdLineArgs::init( &aboutdata );
    KApplication app;
#else
	KchmviewerApp app( argc, argv );

	app.addLibraryPath ( "qt-plugins" );
#endif

	// Set data for QSettings
	QCoreApplication::setOrganizationName("Ulduzsoft");
	QCoreApplication::setOrganizationDomain("kchmviewer.net");
	QCoreApplication::setApplicationName("kchmviewer");

	// Configuration
	pConfig = new Config();

#if !defined (WIN32) && !defined(Q_WS_MAC)
	if ( QDBusConnection::sessionBus().isConnected() )
	{
		if ( QDBusConnection::sessionBus().registerService(SERVICE_NAME) )
		{
			DBusInterface * dbusiface = new DBusInterface();
			QDBusConnection::sessionBus().registerObject( "/", dbusiface, QDBusConnection::ExportAllSlots );
		}
		else
			qWarning( "Cannot register service %s on session bus. Going without D-BUS support.", SERVICE_NAME );
	}
	else
		qWarning( "Cannot connect to the D-BUS session bus. Going without D-BUS support." );
#endif

#if defined (USE_KDE)
    // Because KDE insists of using its KCmdLineArgs class for argument processing, and does not let you just
    // to use QCoreApplication::arguments(), it forces us to write two different process functions. To avoid this,
    // we convert command-line options to arguments ourselves here.
    QStringList arguments;

    for ( int i = 0; i < argc; i++ )
        arguments << argv[i];

    mainWindow = new MainWindow( arguments );
#else
    mainWindow = new MainWindow( QCoreApplication::arguments() );
#endif

    // If we already have the duplicate instance, the data has been already sent to it - quit now
    if ( mainWindow->hasSameTokenInstance() )
        return 0;

	mainWindow->show();
    mainWindow->launch();

	app.connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
	return app.exec();
}
