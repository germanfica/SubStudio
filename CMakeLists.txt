cmake_minimum_required(VERSION 3.22)
project(SubStudio VERSION 0.1.0 LANGUAGES CXX)

# --- C++ standard configuration ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- wxWidgets configuration (manual paths for 3.2.8 prebuilt binaries) ---
set(wxWidgets_ROOT_DIR "C:/wxWidgets-3.2.8")
set(wxWidgets_LIB_DIR "C:/wxWidgets-3.2.8/lib/vc14x_x64_dll")

# Detect configuration (Debug vs Release)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(wxWidgets_CONFIGURATION mswud)   # Unicode Debug
else()
  set(wxWidgets_CONFIGURATION mswu)    # Unicode Release
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base adv)

# --- Define executable ---
#add_executable(substudio
#  src/main.cpp
#  src/mainwindow.cpp
#  src/mainwindow.h
#  src/subtitlemodel.cpp
#  src/subtitlemodel.h
#  src/cpsrenderer.cpp
#  src/cpsrenderer.h
#)

# --- Automatically collect all sources in /src ---
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# --- Define executable ---
add_executable(substudio ${SOURCES})

# --- Link wxWidgets ---
include(${wxWidgets_USE_FILE})
target_link_libraries(substudio PRIVATE ${wxWidgets_LIBRARIES})

# --- Windows GUI subsystem ---
if(WIN32)
  set_target_properties(substudio PROPERTIES WIN32_EXECUTABLE ON)
endif()

# --- Visual Studio configuration ---
if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT substudio)
  set_target_properties(substudio PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/$<CONFIG>"
  )
endif()

# --- Auto-copy DLLs ---
if(WIN32)
  set(WX_DLL_DIR "C:/wxWidgets-3.2.8/lib/vc14x_x64_dll")

  add_custom_command(TARGET substudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WX_DLL_DIR}/wxbase32u_vc14x_x64.dll"
      "${WX_DLL_DIR}/wxbase32ud_vc14x_x64.dll"
      "${WX_DLL_DIR}/wxmsw32u_core_vc14x_x64.dll"
      "${WX_DLL_DIR}/wxmsw32ud_core_vc14x_x64.dll"
      "${WX_DLL_DIR}/wxmsw32u_adv_vc14x_x64.dll"
      "${WX_DLL_DIR}/wxmsw32ud_adv_vc14x_x64.dll"
      $<TARGET_FILE_DIR:substudio>
    COMMENT "Copying wxWidgets DLLs (release+debug) to output directory..."
  )
endif()
