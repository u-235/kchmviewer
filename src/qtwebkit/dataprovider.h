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

#ifndef QWEBVIEWNETWORK_H
#define QWEBVIEWNETWORK_H

#include <QNetworkReply>
#include <QNetworkRequest>

//
// A network reply to emulate data transfer from CHM file
//
class KCHMNetworkReply : public QNetworkReply
{
	public:
		KCHMNetworkReply( const QNetworkRequest& request, const QUrl& url );
		virtual qint64 bytesAvailable() const;
		virtual void abort();

	protected:
		virtual qint64 readData( char* buffer, qint64 maxlen );
		QByteArray loadResource( const QUrl& url );

	private:
		QByteArray	m_data;
		qint64 		m_length;
};


//
// A network manager to emulate data transfer from CHM file
//
class KCHMNetworkAccessManager : public QNetworkAccessManager
{
	public:
		KCHMNetworkAccessManager( QObject* parent );

	protected:
		virtual QNetworkReply* createRequest( Operation op, const QNetworkRequest& request, QIODevice* outgoingData = 0 );
};


#endif // QWEBVIEWNETWORK_H
