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

#ifndef QTWEBENGINE_WEBENGINEPAGE_H
#define QTWEBENGINE_WEBENGINEPAGE_H

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QWebEnginePage>
#include <QWebEngineProfile>

#include "dataprovider.h"
#include "ebook_chm.h"
#include "ebook_epub.h"


#define PRINT_DEBUG ( defined PRINT_DEBUG_ALL || defined PRINT_DEBUG_WEBENGINE || defined PRINT_DEBUG_WEBENGINEPAGE )


class WebEnginePage : public QWebEnginePage
{
		Q_OBJECT

	signals:
		// This signal is emitted whenever the user clicks on a link.
		void linkClicked( const QUrl& url );

	public:
		WebEnginePage( QObject* parent )
			: QWebEnginePage( parent )
		{
			QWebEngineProfile* pf = profile();

			if ( pf->urlSchemeHandler( EBook_CHM::URL_SCHEME_CHM ) == NULL )
			{
				pf->installUrlSchemeHandler( EBook_CHM::URL_SCHEME_CHM, new DataProvider( pf ) );
			}

			if ( pf->urlSchemeHandler( EBook_EPUB::URL_SCHEME_EPUB ) == NULL )
			{
				pf->installUrlSchemeHandler( EBook_EPUB::URL_SCHEME_EPUB, new DataProvider( pf ) );
			}

			connect( this, SIGNAL( linkHovered( const QString& ) ), this, SLOT( onLinkHovered( const QString& ) ) );
		}

		~WebEnginePage()
		{
		}

		// Link click capture. This does not work for the right mouse button.
		virtual bool acceptNavigationRequest( const QUrl& url, NavigationType type, bool isMainFrame ) override
		{
#if PRINT_DEBUG
			qDebug() << "[DEBUG] WebEnginePage::acceptNavigationRequest";
			qDebug() << "  url = " << url.toString();
			qDebug() << "  type  = " << type;
			qDebug() << "  isMainFrame  = " << isMainFrame;
#else
			Q_UNUSED( isMainFrame );
#endif

			if ( type == QWebEnginePage::NavigationTypeLinkClicked )
			{
				emit linkClicked( url );
				return false;
			}

			return true;
		}

		/*
		 * If the link has the "target = _new" attribute, then the WebEngine tries to create
		 * a new page using the createWindow function. However, the old page does not receive
		 * an acceptNavigationRequest unless the Ctrl or Shift key has been pressed.
		 *
		 * The createWindow function is also used from JavaScript to create a new page, but this
		 * does not work in this implementation.
		 */
		QWebEnginePage* createWindow( QWebEnginePage::WebWindowType type )
		{
#if PRINT_DEBUG
			qDebug() << "[DEBUG] WebEnginePage::createWindow";
			qDebug() << "  type = " << type;
#else
			Q_UNUSED( type );
#endif

			if ( !m_url.isEmpty() )
			{
				Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();

				if ( !( mods & ( Qt::ShiftModifier | Qt::ControlModifier ) ) )
					linkClicked( ( m_url ) );
			}

			return 0;
		}

	protected slots:
		void 	onLinkHovered( const QString& url )
		{
#if PRINT_DEBUG
			qDebug() << "[DEBUG] WebEnginePage::linkHovered";
			qDebug() << "  url = " << url;
#endif

			m_url = url;
		}

	protected:
		QString m_url;
};

#undef PRINT_DEBUG

#endif // QTWEBENGINE_WEBENGINEPAGE_H
