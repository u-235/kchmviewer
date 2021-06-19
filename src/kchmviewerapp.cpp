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

#include "kchmviewerapp.h"
#include "mainwindow.h"

KchmviewerApp::KchmviewerApp( int& argc, char** argv, int version )
	: QApplication( argc, argv, version )
{
}

KchmviewerApp::~KchmviewerApp()
{
}

bool KchmviewerApp::event( QEvent* ev )
{
	if ( ev->type() == QEvent::FileOpen )
	{
		m_nResend = 0;
		m_filePath = static_cast<QFileOpenEvent*>( ev )->file();
		onTimer();
		return true;
	}

	return QApplication::event( ev );
}

void KchmviewerApp::onTimer()
{
	MainWindow* main;

	foreach ( QWidget* widget, QApplication::topLevelWidgets() )
	{
		main = dynamic_cast<MainWindow*>( widget );

		if ( main != 0 )
		{
			break;
		}
	}

	if ( main == 0 )
	{
		qWarning( "resending %s", m_filePath.toStdString().c_str() );

		if ( m_nResend >= 30 )
		{
			qWarning( "aborting loading of %s", m_filePath.toStdString().c_str() );
			return;
		}

		QTimer::singleShot( 250, this, SLOT( onTimer() ) );
		++m_nResend;
		return;
	}

	main->loadFile( m_filePath );
}
