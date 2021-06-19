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

#ifndef CHECKNEWVERSION_H
#define CHECKNEWVERSION_H

// This class implements checking whether a new application version is available,
// and reports its availability via signal. It does not use Qt networking stuff,
// and therefore avoids linking with QtNetwork (savings of 1Mb!).
// All processing is done in a separate thread, so it does not block the app.
// It does not use GUI stuff, so should be safe.
//
// The proper way to use this class:
//
// CheckNewVersion * pN = new CheckNewVersion();
// connect( pN, SIGNAL( newVersionAvailable( const QMap<QString>&) ), this, SLOT( newVersionAvailable( const QMap<QString>&) ) );
//
// pN->setUrl( "http://www.example.com/latestversion.txt" );
// pN->setCurrentVersion( "1.12" );
// pN->start();
//
// The text file must have the following format:
// <field name>:<field value>
// Two field names are required (Signature and Version). Any other names are optional. Any name may be added.
// A multiline value string should have all line feed characters replaced by \n, and all single backlashes replaced by two
// Signature must be the first field, and must contain the "CheckNewVersion1" value
//
// An example file with extra fields "URL" and "Changes" added:
//
// Signature:CheckNewVersion1
// Version:1.12
// URL: http://example.com/latestversion.zip
// Changes: new functionality added.\nA bar function added to package foo.\n\nZeta now works.
//
//

#include <QThread>
#include <QMetaType>
#include <QMap>

typedef QMap<QString, QString>	NewVersionMetaMap;

class CheckNewVersion : public QThread
{
		Q_OBJECT

	public:
		enum
		{
			Status_Resolving,
			Status_Connecting,
			Status_SendingRequest,
			Status_ReceivingResponse,
			Status_Proceeding,
			Status_Finished,
		};

		enum
		{
			Error_URL_Invalid,
			Error_Name_Lookup,
			Error_System,
			Error_Connecting,
			Error_Sending,
			Error_Receiving,
			Error_HTTPerror,
			Error_InvalidFormat,
			Error_InvalidSignature
		};

		CheckNewVersion();

		// Sets the full URL to get the latest version information from.
		void	setUrl( const QString& url );

		// Sets the current version. newVersionAvailable() will only be emitted
		// if current version does not match the version in the downloaded file.
		// If not called, newVersionAvailable() will be always emitted.
		void	setCurrentVersion( const QString& version );

	signals:
		void	newVersionAvailable( const NewVersionMetaMap& metadata );
		void	statusChanged( int newstatus );
		void	error( int errorcode );

	private:
		// Reimplemented
		void	run();

		// All those functions on error generate the event, and shut down the thread.
		void	fatalError( int code );

		// Read the line from socket (or m_inputBuffer).
		void	reportStatus( int status );

		// Read the data from socket up to length. May return less than length.
		QString readLine();

		// Closing the socket
		void		closeSocket();

	private:
		QString			m_url;
		QString			m_currentversion;

		int             m_sockfd;
		unsigned int    m_timeout;
		int				m_inputOffset;  // in m_inputBuffer
		QByteArray      m_inputBuffer;	// for socket input
};

Q_DECLARE_METATYPE( NewVersionMetaMap );

#endif // CHECKNEWVERSION_H
