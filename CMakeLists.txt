cmake_minimum_required(VERSION 3.22)
project(SubStudio VERSION 0.1.0 LANGUAGES CXX)

# --- C++ standard configuration ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt meta-object system ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/cmake_autogen")

# --- Find Qt6 Widgets ---
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# --- Main executable ---
add_executable(substudio WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/subtitlemodel.cpp
    src/subtitlemodel.h
)

# --- Automatic Qt properties ---
#set_target_properties(substudio PROPERTIES
#    AUTOMOC ON
#    AUTOUIC ON
#    AUTORCC ON
#)

# --- Compiler options ---
#target_compile_options(substudio PRIVATE
#    $<$<CXX_COMPILER_ID:MSVC>:/std:c++23 /Zc:__cplusplus>
#)

# --- Link Qt libraries ---
target_link_libraries(substudio PRIVATE Qt6::Widgets)

# --- Post-build step: run windeployqt automatically on Windows ---
if (WIN32)
    if (MSVC)
        set(QT_DEPLOY_TOOL "C:/Qt/6.9.3/msvc2022_64/bin/windeployqt.exe")
    else()
        set(QT_DEPLOY_TOOL "C:/Qt/6.9.3/mingw_64/bin/windeployqt.exe")
    endif()

    add_custom_command(TARGET substudio POST_BUILD
        COMMAND "${QT_DEPLOY_TOOL}" "$<TARGET_FILE:substudio>"
        COMMENT "Running windeployqt to deploy Qt dependencies..."
    )
endif()

# --- Set default startup project when opening the solution in Visual Studio ---
if (MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT substudio)
endif()
