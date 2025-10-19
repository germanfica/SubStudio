cmake_minimum_required(VERSION 3.22)
project(SubStudio VERSION 0.1.0 LANGUAGES CXX)

# --- C++ standard configuration ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Find Qt modules ---
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# --- Standard Qt project setup ---
# This enables AUTOMOC, AUTOUIC, AUTORCC, and other default settings.
qt_standard_project_setup()

# --- Main executable ---
qt_add_executable(substudio
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/subtitlemodel.cpp
    src/subtitlemodel.h
)

# --- Link Qt libraries ---
target_link_libraries(substudio PRIVATE Qt6::Widgets)

# --- Platform-specific properties ---
set_target_properties(substudio PROPERTIES
    WIN32_EXECUTABLE ON
)

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
    # Set the debugger working directory to the project root
    set_target_properties(substudio PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()
