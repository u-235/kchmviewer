/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2021 Nick Egorrov, nicegorov@yandex.ru
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

#include <QBuffer>
#include <QUrl>
#include <QString>

#include "mimehelper.h"


// Yes, I know about std::isspace(), but it may depend on the locale.
#define isspace(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')

QByteArray MimeHelper::mimeType( const QUrl& url, const QByteArray& buf )
{
	QString path = url.path().toLower();

	// Try to recognize the MIME type by the extension of the file name.
	if ( path.endsWith( ".css" ) )
		return "text/css";
	else if ( path.endsWith( ".js" ) )
		return "text/js";
	else if ( path.endsWith( ".htm" ) | path.endsWith( ".html" ) | path.endsWith( ".xhtml" ) )
		return "text/html";

	// If the first non space character in buf is '<',
	// then we assume that the buffer contains HTML.
	for ( auto iter = buf.begin(); iter != buf.end() ; iter++ )
	{
		char c = *iter;

		if ( c == '<' )
			return "text/html";

		if ( !isspace( c ) )
			break;
	}

	return "application/octet-stream";
}
