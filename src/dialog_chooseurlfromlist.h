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

#ifndef DIALOG_CHOOSEURLFROMLIST_H
#define DIALOG_CHOOSEURLFROMLIST_H

#include <QDialog>
#include "ui_dialog_topicselector.h"


class DialogChooseUrlFromList : public QDialog, public Ui::DialogTopicSelector
{
		Q_OBJECT

	public:
		DialogChooseUrlFromList( QWidget* parent );

		// Shows the dialog with titles, and let the user to select the title.
		// Obviously urls.size() == titles.size(). Returns the appropriate URL
		// for the selected title, or empty string if no title selected, or dialog canceled.
		QUrl getSelectedItemUrl( const QList<QUrl>& urls, const QStringList& titles );

	private slots:
		void onDoubleClicked( QListWidgetItem* item );
};

#endif
