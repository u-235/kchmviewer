
cmake_minimum_required(VERSION 3.0)
# Need to include generated files.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Project files
set(CPP_SOURCES
    config.cpp
    dbus_interface.cpp
    dialog_chooseurlfromlist.cpp
    dialog_setup.cpp
    kde-qt.cpp
    main.cpp
    mainwindow.cpp
    recentfiles.cpp
    settings.cpp
    tab_bookmarks.cpp
    tab_contents.cpp
    tab_index.cpp
    tab_search.cpp
    navigationpanel.cpp
    checknewversion.cpp
    toolbarmanager.cpp
    toolbareditor.cpp
    textencodings.cpp
    treeitem_toc.cpp
    treeitem_index.cpp
    mimehelper.cpp
    )

set(MOC_HEADERS
    dbus_interface.h
    dialog_chooseurlfromlist.h
    dialog_setup.h
    mainwindow.h
    recentfiles.h
    tab_bookmarks.h
    tab_contents.h
    tab_index.h
    tab_search.h
    viewwindowmgr.h
    navigationpanel.h
    checknewversion.h
    toolbarmanager.h
    toolbareditor.h
    )

# UI files
set(UIS
    dialog_about.ui
    dialog_setup.ui
    dialog_topicselector.ui
    mainwindow.ui
    navigatorpanel.ui
    tab_bookmarks.ui
    tab_contents.ui
    tab_index.ui
    tab_search.ui
    toolbareditor.ui
    window_browser.ui
    )

if (${QT_USE_WEBENGINE})
    set(CPP_SOURCES ${CPP_SOURCES}
        qtwebengine/viewwindow.cpp
        qtwebengine/dataprovider.cpp
        qtwebengine/viewwindowmgr.cpp
        )
    set(MOC_HEADERS ${MOC_HEADERS} qtwebengine/viewwindow.h  qtwebengine/webenginepage.h)
    set(QT_WEB_PACKAGE Qt::WebEngine Qt::WebEngineWidgets)
else ()
    set(CPP_SOURCES ${CPP_SOURCES}
        qtwebkit/viewwindow.cpp
        qtwebkit/dataprovider.cpp
        qtwebkit/viewwindowmgr.cpp
        )
    set(MOC_HEADERS ${MOC_HEADERS} qtwebkit/viewwindow.h)
    set(QT_WEB_PACKAGE Qt::WebKit Qt::WebKitWidgets)
endif ()

# Mac OS specific
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(CPP_SOURCES ${CPP_SOURCES} kchmviewerapp.cpp)
    set(MOC_HEADERS ${MOC_HEADERS} kchmviewerapp.h)
endif ()

qt_add_resources(RSRCS resources/images.qrc)

if (${KDE4_FOUND})
    kde4_add_ui_files(CPP_SOURCES ${UIS})
    # automoc in some cases cannot find a suitable header file, at the same time
    # kde4_add_executable always does automoc and this is can't disabled.
    # Therefore, an ugly solution is used here.
    qt_wrap_cpp(MOC_SOURCES viewwindowmgr.h)
    kde4_add_executable(kchmviewer ${CPP_SOURCES} ${RSRCS} ${MOC_SOURCES})
    target_link_libraries(kchmviewer
        ebook
        chm
        ${KDE4_KDEUI_LIBS}
        ${QT_QTWEBKIT_LIBRARY}
        ${KDE4_KFILE_LIBS}
        $<$<BOOL:UNIX>:X11>
        $<$<PLATFORM_ID:Windows>:wsock32>
        )

    install(TARGETS kchmviewer DESTINATION ${KDE4_BIN_INSTALL_DIR})
else ()
    qt_wrap_ui(UI_SOURCES ${UIS})
    qt_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})
    add_executable(kchmviewer ${CPP_SOURCES} ${RSRCS} ${UI_SOURCES} ${MOC_SOURCES})
    target_link_libraries(kchmviewer
        ebook
        chm
        Qt::Core
        Qt::DBus
        Qt::Network
        Qt::PrintSupport
        ${QT_WEB_PACKAGE}
        Qt::Widgets
        Qt::Xml
        $<$<PLATFORM_ID:Linux>:X11>
        $<$<PLATFORM_ID:Windows>:wsock32>
        )

endif ()
