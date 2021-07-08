
TEMPLATE = app
TARGET = ../bin/kchmviewer
CONFIG *= c++11 warn_on threads # xml dbus precompile_header
QT += \
    dbus \
    xml \
    network \
    widgets \
    printsupport

HEADERS += \
    config.h \
    dialog_chooseurlfromlist.h \
    dialog_setup.h \
    kde-qt.h \
    mainwindow.h \
    recentfiles.h \
    settings.h \
    tab_bookmarks.h \
    tab_contents.h \
    tab_index.h \
    tab_search.h \
    version.h \
    viewwindow.h \
    viewwindowmgr.h \
    navigationpanel.h \
    checknewversion.h \
    toolbarmanager.h \
    toolbareditor.h \
    textencodings.h \
    treeitem_toc.h \
    treeitem_index.h \
    mimehelper.h

SOURCES += \
    config.cpp \
    dialog_chooseurlfromlist.cpp \
    dialog_setup.cpp \
    kde-qt.cpp \
    main.cpp \
    mainwindow.cpp \
    recentfiles.cpp \
    settings.cpp \
    tab_bookmarks.cpp \
    tab_contents.cpp \
    tab_index.cpp \
    tab_search.cpp \
    navigationpanel.cpp \
    checknewversion.cpp \
    toolbarmanager.cpp \
    toolbareditor.cpp \
    textencodings.cpp \
    treeitem_toc.cpp \
    treeitem_index.cpp \
    mimehelper.cpp

FORMS += tab_bookmarks.ui \
    tab_index.ui \
    tab_contents.ui \
    tab_search.ui \
    dialog_setup.ui \
    dialog_topicselector.ui \
    mainwindow.ui \
    window_browser.ui \
    navigatorpanel.ui \
    dialog_about.ui \
    toolbareditor.ui

RESOURCES += resources/images.qrc

INCLUDEPATH *= ../lib/libebook ../lib/CHMLib/src

LIBS *= \
    -L"../lib/libebook" -lebook \
    -L"../lib" -lchm \
    -lzip

defined(LIBZIP_ROOT_DIR, var): LIBS *= -L"$${LIBZIP_ROOT_DIR}/lib"
defined(LIBCHM_ROOT_DIR, var): LIBS *= -L"$${LIBCHM_ROOT_DIR}/lib"

linux-g++*:{
    LIBS *= -lX11
}

# This is used by cross-build on 64-bit when building a 32-bit version
linux-g++-32: {
       LIBS *= -L.
}

# General per-platform settings
macx:{
    HEADERS += kchmviewerapp.h
    SOURCES += kchmviewerapp.cpp
    QMAKE_INFO_PLIST=resources/Info.plist
    ICON = resources/kchmviewer.icns
}

win32:{
    CONFIG( debug, debug|release ) {
            LIBS *= -L"../lib/libebook/debug" -L"../lib/debug"
    } else {
            LIBS *= -L"../lib/libebook/release" -L"../lib/release"
    }

    LIBS += -lwsock32 -loleaut32
}

unix:!macx: {
    QT += dbus
    HEADERS += dbus_interface.h
    SOURCES += dbus_interface.cpp
    CONFIG += dbus
    #LIBS *= ../lib/libebook/libebook.a
}

defined(USE_WEBENGINE, var) {
    isEqual(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9):error("QtWebEnginew requires at least Qt5.9")

    QT += webengine webenginewidgets
    DEFINES += USE_WEBENGINE
    SOURCES += qtwebengine/viewwindow.cpp qtwebengine/dataprovider.cpp qtwebengine/viewwindowmgr.cpp
    HEADERS += qtwebengine/dataprovider.h qtwebengine/viewwindow.h qtwebengine/webenginepage.h
} else {
    QT += webkit webkitwidgets
    DEFINES += USE_WEBKIT
    SOURCES += qtwebkit/viewwindow.cpp qtwebkit/dataprovider.cpp qtwebkit/viewwindowmgr.cpp
    HEADERS += qtwebkit/dataprovider.h qtwebkit/viewwindow.h
}
