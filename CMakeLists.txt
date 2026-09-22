cmake_minimum_required(VERSION 3.19)
project(ChallengeDXCC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql WebSockets Core)

qt_standard_project_setup()

set(APP_SOURCES
    main.cpp

    core/models.h
    core/adif.h
    core/adif.cpp
    core/dxcc.h
    core/dxcc.cpp
    core/challengeengine.h
    core/challengeengine.cpp

    db/challengedb.h
    db/challengedb.cpp

    backend/backendcommon.h
    backend/backendcommon.cpp
    backend/localdxccresolver.h
    backend/localdxccresolver.cpp
    backend/adifimporter.h
    backend/adifimporter.cpp
    backend/deletedqsowatcher.h
    backend/deletedqsowatcher.cpp
    backend/hamalertclient.h
    backend/hamalertclient.cpp
    backend/localdxclusterserver.h
    backend/localdxclusterserver.cpp
    backend/pskreportermqttclient.h
    backend/pskreportermqttclient.cpp
    backend/genericdxclusterclient.h
    backend/genericdxclusterclient.cpp
    backend/jtdxudplistener.h
    backend/jtdxudplistener.cpp
    backend/logger32bridge.h
    backend/logger32bridge.cpp
    backend/challengebackend.h
    backend/challengebackend.cpp

    gui/servicecard.h
    gui/servicecard.cpp
    gui/tciclient.h
    gui/tciclient.cpp
    gui/mainwindow.h
    gui/mainwindow.cpp
    gui/main_window.ui
)

qt_add_executable(ChallengeDXCC ${APP_SOURCES})

target_include_directories(ChallengeDXCC PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ChallengeDXCC PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::WebSockets
    Qt6::Core
)

if (WIN32)
    target_link_libraries(ChallengeDXCC PRIVATE user32 kernel32)
    target_compile_definitions(ChallengeDXCC PRIVATE CHALLENGEDXCC_WINDOWS=1)
endif()

# Copie les ressources/config à côté de l'exécutable, comme le projet Python.
add_custom_command(TARGET ChallengeDXCC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:ChallengeDXCC>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/config $<TARGET_FILE_DIR:ChallengeDXCC>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/profiles $<TARGET_FILE_DIR:ChallengeDXCC>/profiles
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ChallengeDXCC>/gui
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/styles.qss $<TARGET_FILE_DIR:ChallengeDXCC>/gui/styles.qss
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ChallengeDXCC>/runtime
)
