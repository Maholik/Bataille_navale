set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
cmake_minimum_required(VERSION 3.5)

project(Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -pedantic-errors")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets )
find_package( Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets )
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(Client
    main.cpp
    clickablewidget.h
    clickablewidget.cpp
    startwindow.h
    startwindow.cpp
    startwindow.ui
    gamewindow.h
    gamewindow.cpp
    gamewindow.ui)

target_link_libraries (Client PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

include(GNUInstallDirs)

install(TARGETS Client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Client)
endif()
