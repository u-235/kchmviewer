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

#include <QDir>

#include "dataprovider.h"
#include "../viewwindow.h"
#include "../config.h"
#include "../mainwindow.h"
#include "../mimehelper.h"


KCHMNetworkReply::KCHMNetworkReply( const QNetworkRequest& request, const QUrl& url )
{
	setRequest( request );
	setOpenMode( QIODevice::ReadOnly );

	m_data = loadResource( url );
	m_length = m_data.length();

	setHeader( QNetworkRequest::ContentLengthHeader, QByteArray::number( m_data.length() ) );
	QMetaObject::invokeMethod( this, "metaDataChanged", Qt::QueuedConnection );

	if ( m_length )
		QMetaObject::invokeMethod( this, "readyRead", Qt::QueuedConnection );

	QMetaObject::invokeMethod( this, "finished", Qt::QueuedConnection );
}

qint64 KCHMNetworkReply::bytesAvailable() const
{
	return m_data.length() + QNetworkReply::bytesAvailable();
}

void KCHMNetworkReply::abort()
{
}

qint64 KCHMNetworkReply::readData( char* buffer, qint64 maxlen )
{
	qint64 len = qMin( qint64( m_data.length() ), maxlen );

	if ( len )
	{
		memcpy( buffer, m_data.constData(), len );
		m_data.remove( 0, len );
	}

	return len;
}

QByteArray KCHMNetworkReply::loadResource( const QUrl& url )
{
	//qDebug("loadResource %s", qPrintable(url.toString()) );

	// Retreive the data from ebook file
	QByteArray buf;

	if ( !::mainWindow->chmFile()->getFileContentAsBinary( buf, url ) )
	{
		qWarning( "Could not resolve file %s\n", qPrintable( url.toString() ) );

	}

	if ( MimeHelper::mimeType( url, buf ) == "text/html" )
		setHeader( QNetworkRequest::ContentTypeHeader,
				   QString( "text/html; charset=%1" ) .arg( ::mainWindow->chmFile()->currentEncoding() ) );

	return buf;
}


KCHMNetworkAccessManager::KCHMNetworkAccessManager( QObject* parent )
	: QNetworkAccessManager( parent )
{
}

QNetworkReply* KCHMNetworkAccessManager::createRequest( Operation op, const QNetworkRequest& request,
		QIODevice* outgoingData )
{
	//qDebug("KCHMNetworkAccessManager::createRequest %s", qPrintable( request.url().toString()) );

	if ( ::mainWindow->chmFile()->isSupportedUrl( request.url() ) )
		return new KCHMNetworkReply( request, request.url() );

	if ( pConfig->m_browserEnableRemoteContent )
		return QNetworkAccessManager::createRequest( op, request, outgoingData );
	else
		return QNetworkAccessManager::createRequest( QNetworkAccessManager::GetOperation, QNetworkRequest( QUrl() ) );
}
