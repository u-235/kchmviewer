#ifndef VIEWWINDOW_H
#define VIEWWINDOW_H

// We support both engines
#if defined (USE_WEBENGINE)
	#include "qtwebengine/viewwindow.h"
#else
	#include "qtwebkit/viewwindow.h"
#endif

#endif // VIEWWINDOW_H
