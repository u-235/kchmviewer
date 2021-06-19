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

#ifndef MIMEHELPER_H
#define MIMEHELPER_H

class QByteArray;
class QUrl;

// Since Qt 5.0 there is a QMimeDatabase class in Qt which does more detailed content analysis.
// But kchmviewer hasn't given up building with Qt 4 yet.
class MimeHelper
{
	public:
		/**
		 * @brief Assumes MIME type by url or content.
		 * @param url File address.
		 * @param buf File contents.
		 * @return "text/css", "text/html", "text/js" or "application/octet-stream".
		 */
		static QByteArray mimeType( const QUrl& url, const QByteArray& buf );
};

#endif // MIMEHELPER_H
