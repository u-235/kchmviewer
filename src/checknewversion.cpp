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

#include <QUrl>
#include <QStringList>
#include <QMetaType>

#if !defined (WIN32)
	#include <sys/socket.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <errno.h>
	#include <unistd.h>
#else
	#include <winsock.h>
#endif

#include "checknewversion.h"

// Uncomment this to enable debugging messages
//#define ENABLE_DEBUG_MESSAGES


CheckNewVersion::CheckNewVersion()
	: QThread()
{
	m_sockfd = -1;
	m_timeout = 180;
	m_inputOffset = 0;
	m_inputBuffer.resize( 8192 );

	qRegisterMetaType< NewVersionMetaMap >( "NewVersionMetaMap" );
}

void CheckNewVersion::setUrl( const QString& url )
{
	m_url = url;
}

void CheckNewVersion::setCurrentVersion( const QString& version )
{
	m_currentversion = version;
}

void CheckNewVersion::closeSocket()
{
	if ( m_sockfd == -1 )
		return;

#if defined (WIN32)
	closesocket( m_sockfd );
#else
	close( m_sockfd );
#endif

	m_sockfd = -1;
}

void CheckNewVersion::fatalError( int code )
{
#if defined (ENABLE_DEBUG_MESSAGES)
#define CASE_PRINT(A) case A: qDebug("CheckNewVersion::fatalError( " #A " )"); break;

	switch ( code )
	{
		CASE_PRINT( Error_URL_Invalid );
		CASE_PRINT( Error_Name_Lookup );
		CASE_PRINT( Error_System );
		CASE_PRINT( Error_Connecting );
		CASE_PRINT( Error_Sending );
		CASE_PRINT( Error_Receiving );
		CASE_PRINT( Error_HTTPerror );
		CASE_PRINT( Error_InvalidFormat );
		CASE_PRINT( Error_InvalidSignature );
	}

#undef CASE_PRINT
#endif

	closeSocket();
	emit error( code );
	deleteLater();
}

void CheckNewVersion::reportStatus( int status )
{
#if defined (ENABLE_DEBUG_MESSAGES)
#define CASE_PRINT(A) case A: qDebug("CheckNewVersion::reportStatus( " #A " )"); break;

	switch ( status )
	{
		CASE_PRINT( Status_Resolving );
		CASE_PRINT( Status_Connecting );
		CASE_PRINT( Status_SendingRequest );
		CASE_PRINT( Status_ReceivingResponse );
		CASE_PRINT( Status_Proceeding );
		CASE_PRINT( Status_Finished );
	}

#undef CASE_PRINT
#endif

	emit statusChanged( status );
}

void CheckNewVersion::run()
{
	m_inputOffset = 0;

	// Validate the URL
	QUrl url( m_url );

	if ( !url.isValid() || url.scheme() != "http" || url.host().isEmpty() )
	{
		fatalError( Error_URL_Invalid );
		return;
	}

	// Win32s-specific socket initialization
#if defined (WIN32)
	WORD wVersionRequested = MAKEWORD ( 1, 1 );
	WSADATA wsaData;

	if ( WSAStartup ( wVersionRequested, &wsaData ) != 0 )
	{
		fatalError( Error_System );
		return;
	}

#endif

	// IPv4 address resolving
	struct sockaddr_in saddr;
	memset( &saddr, 0, sizeof( saddr ) );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons( url.port( 80 ) );
	saddr.sin_addr.s_addr = ::inet_addr ( qPrintable( url.host() ) );

	if ( saddr.sin_addr.s_addr == INADDR_NONE )
	{
		reportStatus( Status_Resolving );

		struct hostent* hp;
#if defined HAVE_GETHOSTBYNAME_R
		int tmp_errno;
		struct hostent tmp_hostent;
		char buf[2048];

		if ( ::gethostbyname_r( qPrintable( url.host() ), &tmp_hostent, buf, sizeof( buf ), &hp, &tmp_errno ) )
			hp = 0;

#else
		hp = ::gethostbyname( qPrintable( url.host() ) );
#endif // HAVE_GETHOSTBYNAME_R

		if ( !hp )
		{
			fatalError( Error_Name_Lookup );
			return;
		}

		::memcpy ( &saddr.sin_addr, hp->h_addr, ( size_t ) hp->h_length );
	}

	// create the socket
	m_sockfd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if ( m_sockfd < 0 )
	{
		fatalError( Error_System );
		return;
	}

	reportStatus( Status_Connecting );

	// Connect to the HTTP server
	if ( ::connect( m_sockfd, ( struct sockaddr* ) &saddr, sizeof( saddr ) ) )
	{
		fatalError( Error_Connecting );
		return;
	}

	// Prepare the HTTP request
	QString request = QString( "GET %1 HTTP/1.1\r\n"
							   "Host: %2\r\n"
							   "User-Agent: Qt/New version checker (www.karlyriceditor.com)\r\nConnection: close\r\n\r\n" )
					  .arg( url.path() ) .arg( url.host() );

	// Send the request
	reportStatus( Status_SendingRequest );

	const char* reqmsg = qPrintable( request );
	unsigned int offset = 0, length = strlen( reqmsg );

	while ( offset < length )
	{
		int sentamount = ::send( m_sockfd, reqmsg + offset, length - offset, 0 );

		if ( sentamount <= 0 )
		{
			fatalError( Error_Sending );
			return;
		}

		offset += sentamount;
	}

	// Receive the response
	reportStatus( Status_ReceivingResponse );

	// First, receive the HTTP header
	int contentlen = -1;
	QStringList header;

	while ( 1 )
	{
		QString line = readLine();

		// Has connection closed?
		if ( m_sockfd == -1 )
		{
			fatalError( Error_Receiving );
			return;
		}

		// Empty line separates header and body
		if ( line.isEmpty() )
			break;

		header.push_back( line );
	}

	// Make sure server didn't return error
	if ( header.isEmpty() || header[0].indexOf( QRegExp( "^http/1.\\d\\s+2\\d\\d", Qt::CaseInsensitive ) ) == -1 )
	{
#if defined (ENABLE_DEBUG_MESSAGES)

		if ( !header.isEmpty() )
			qDebug( "CheckNewVersion::run: server returned  invalid header: %s", qPrintable( header[0] ) );

#endif
		fatalError( Error_HTTPerror );
		return;
	}

	// Find content-length
	QRegExp clr( "^content-length: (\\d+)$" );
	clr.setCaseSensitivity( Qt::CaseInsensitive );

	if ( header.indexOf( clr ) != -1 )
		contentlen = clr.cap( 1 ).toInt();

	// Read the rest of content until we have contentlen or connection closed
	while ( contentlen == -1 || contentlen < m_inputOffset )
	{
		int amount = ::recv( m_sockfd, m_inputBuffer.data() + m_inputOffset, m_inputBuffer.size() - m_inputOffset, 0 );

		// connection closed?
		if ( amount == 0 )
			break;

		// read error
		if ( amount < 0 )
		{
			fatalError( Error_Receiving );
			return;
		}

		m_inputOffset += amount;
	}

	closeSocket();
	m_inputBuffer[ m_inputOffset ] = '\0';

	// Remove/replace line ends, and convert to a string
	reportStatus( Status_Proceeding );

	m_inputBuffer.replace( '\r', '\n' );
	QStringList content_list = QString::fromUtf8( m_inputBuffer ).split( '\n', QString::SkipEmptyParts );
	QMap<QString, QString> contentMap;

	// Validate the file, and parse it into map
	for ( int i = 0; i < content_list.size(); i++ )
	{
		QRegExp reg( "^(\\w+)\\s*:(.*)$" );

		if ( content_list[i].indexOf( reg ) == -1 )
		{
#if defined (ENABLE_DEBUG_MESSAGES)
			qDebug( "CheckNewVersion::run: invalid line found: '%s'", qPrintable( content_list[i] ) );
#endif
			fatalError( Error_InvalidFormat );
			return;
		}

		// Decode \n back to 0x0A
		QString value = reg.cap( 2 ).trimmed();
		value.replace( "\\n", "\n" );
		value.replace( "\\\\", "\\" );
		contentMap[ reg.cap( 1 ) ] = value;
	}

	// Validate signature
	if ( !contentMap.contains( "Signature" )
			|| !contentMap.contains( "Version" )
			|| contentMap["Signature"] != "CheckNewVersion1" )
	{
		fatalError( Error_InvalidSignature );
		return;
	}

	contentMap.remove( "Signature" );

	// Do we need to call the callback?
	if ( m_currentversion.isEmpty() || contentMap["Version"] != m_currentversion )
		emit newVersionAvailable( contentMap );

	reportStatus( Status_Finished );
	deleteLater();
}


QString CheckNewVersion::readLine()
{
	while ( 1 )
	{
		// First check if we have a line in buffer already
		if ( m_inputOffset > 0 )
		{
			for ( int i = 0; i < m_inputOffset - 1; i++ )
			{
				if ( m_inputBuffer[i] == '\r' && m_inputBuffer[i + 1] == '\n' )
				{
					// Null-terminate the buffer, and copy the string
					m_inputBuffer[i] = '\0';
					QString line = QString::fromUtf8( m_inputBuffer );

					// Now move the rest of the buffer if something left
					unsigned int amount = i + 2; // removing CRLF too)
					m_inputOffset -= amount;

					if ( m_inputOffset > 0 )
						memmove( m_inputBuffer.data(), m_inputBuffer.data() + amount, m_inputOffset );

					return line;
				}
			}
		}

		// No line in buffer yet
		if ( m_inputOffset + 1 > m_inputBuffer.size() )
			return QString::null;

		int bytes = ::recv( m_sockfd, m_inputBuffer.data() + m_inputOffset, m_inputBuffer.size() - m_inputOffset, 0 );

		// Error; restart on EINTR, abort on anything else
		if ( bytes < 0 )
		{
			if ( errno == EINTR )
				continue;

			break;
		}

		if ( bytes == 0 )
			break;

		m_inputOffset += bytes;
	}

	return QString::null;
}
