cmake_minimum_required(VERSION 3.22)
project(SubStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt meta-object system
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(substudio WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

target_link_libraries(substudio PRIVATE Qt6::Widgets)

# --- Post-build step: run windeployqt automatically on Windows ---
if (WIN32)
    set(QT_DEPLOY_TOOL "C:/Qt/6.9.3/mingw_64/bin/windeployqt.exe")

    add_custom_command(TARGET substudio POST_BUILD
        COMMAND "${QT_DEPLOY_TOOL}" "$<TARGET_FILE:substudio>"
        COMMENT "Running windeployqt to deploy Qt dependencies..."
    )
endif()
