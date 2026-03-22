cmake_minimum_required(VERSION 3.5)
project(dashboard_qt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network WebSockets)

add_executable(dashboard_qt
    main.cpp

    mainwindow.cpp
    resizehandle.cpp
    overviewpage.cpp
    taskspage.cpp
    alertspage.cpp
    settingspage.cpp
    dashboardclient.cpp

    mainwindow.h
    resizehandle.h
    overviewpage.h
    taskspage.h
    alertspage.h
    settingspage.h
    dashboardclient.h

    mainwindow.ui
    overview_page.ui
    tasks_page.ui
    alerts_page.ui
    settings_page.ui
)

target_link_libraries(dashboard_qt
    Qt5::Widgets
    Qt5::Network
    Qt5::WebSockets
)