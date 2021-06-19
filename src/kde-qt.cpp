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

#include "kde-qt.h"

#if defined (USE_KDE)
KQProgressModalDialog::KQProgressModalDialog ( const QString& captionText, const QString& labelText,
		const QString& cancelButtonText, int totalSteps, QWidget* creator )
	: KProgressDialog( creator, captionText, labelText )
{
	setAllowCancel( true );
	showCancelButton( true );
	setAutoClose( true );
	setButtonText( cancelButtonText );
	progressBar()->setMaximum( totalSteps );
	setMinimumDuration( 1 );
}
#else
KQProgressModalDialog::KQProgressModalDialog ( const QString& captionText, const QString& labelText,
		const QString& cancelButtonText, int totalSteps, QWidget* creator )
	: QProgressDialog( labelText, cancelButtonText, 0, totalSteps, creator )
{
	setWindowTitle( captionText );
	setMinimumDuration( 1 );
}
#endif
