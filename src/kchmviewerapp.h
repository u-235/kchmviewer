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

#ifndef KCHMVIEWERAPP_H
#define KCHMVIEWERAPP_H

#include <QApplication>
#include <QFileOpenEvent>

class KchmviewerApp : public QApplication
{
		Q_OBJECT
	public:
		KchmviewerApp( int& argc, char** argv, int version = QT_VERSION );
		virtual ~KchmviewerApp();
		bool event( QEvent* );

	private slots:
		void onTimer();

	private:
		QString m_filePath;
		int m_nResend;
};

#endif // KCHMVIEWERAPP_H
