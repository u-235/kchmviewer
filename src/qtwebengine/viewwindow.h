/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2004-2016 George Yunaev, gyunaev@ulduzsoft.com
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

#ifndef QTWEBENGINE_VIEWWINDOW_H
#define QTWEBENGINE_VIEWWINDOW_H

#include <QWebEngineView>

#include "../kde-qt.h"
#include "dataprovider.h"

class ViewWindow : public QWebEngineView
{
		Q_OBJECT

	public:
		ViewWindow( QWidget* parent );
		virtual ~ViewWindow();

		//! Open a page from current chm archive
		bool	openUrl ( const QUrl& url );

		QUrl	getOpenedPage() const
		{
			return url();
		}
		QUrl	getNewTabLink() const
		{
			return m_newTabLinkKeeper;
		}

	signals:
		void    dataLoaded( ViewWindow* window );

		// This signal is emitted whenever the user clicks on a link.
		void linkClicked( const QUrl& url );

	public:
		// Apply the configuration settings (JS enabled etc) to the web renderer
		static  void    applySettings();

		//! Invalidate current view, doing all the cleanups etc.
		void	invalidate();

		//! Popups the print dialog, and prints the current page on the printer.
		bool	printCurrentPage();

		//! Return current ZoomFactor.
		qreal	getZoomFactor() const;

		//! Sets ZoomFactor. The value returned by getZoomFactor(), given to this function, should give the same result.
		void	setZoomFactor( qreal zoom );

		/*!
		* Return current scrollbar position in view window. Saved on program exit.
		* There is no restriction on returned value, except that giving this value to
		* setScrollbarPosition() should move the scrollbar in the same position.
		*/
		int		getScrollbarPosition();

		//! Sets the scrollbar position.
		void	setScrollbarPosition( int pos, bool force = false );

		//! Select the content of the whole page
		void	clipSelectAll();

		//! Copies the selected content to the clipboard
		void	clipCopy();

		//! Updates the history toolbar icon status
		void	updateHistoryIcons();

		//! Returns the window title
		QString	title() const;

		//! Navigation stuff
		void	navigateBack();
		void	navigateHome();
		void	navigateForward();

		//! Keeps the tab URL between link following
		void	setTabKeeper ( const QUrl& link );

	public slots:
		void	zoomIncrease();
		void	zoomDecrease();

	protected:
		bool			openPage ( const QUrl& url );
		void			handleStartPageAsImage( QUrl& link );

		QMenu* 		getContextMenu( const QUrl& link, QWidget* parent );
		QMenu* 		createStandardContextMenu( QWidget* parent );

		// Overriden to change the source
		void			setSource ( const QUrl& name );

		// Overloaded to provide custom context menu
		void 			contextMenuEvent( QContextMenuEvent* e );
		void            showContextMenu( const QPoint* point, const QString link );
		//void			mouseReleaseEvent ( QMouseEvent * event );

	private slots:
		// Used to restore the scrollbar position and the navigation button status
		void			onLoadFinished ( bool ok );
		void            onLinkClicked( const QUrl& url );

	private:
		QMenu* 					m_contextMenu;
		QMenu* 					m_contextMenuLink;

		// This member keeps a "open new tab" link between getContextMenu()
		// call and appropriate slot call
		QUrl					m_newTabLinkKeeper;

		// Keeps the scrollbar position to move after the page is loaded
		int						m_storedScrollbarPosition;
};

#endif // QTWEBENGINE_VIEWWINDOW_H
