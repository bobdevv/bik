cmake_minimum_required(VERSION 3.15)
project(bik VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(ZLIB REQUIRED)

# Core library
add_library(bik_core STATIC
    src/core/BackupManager.cpp
    src/core/BackupManager.h
    src/core/ProjectConfig.cpp
    src/core/ProjectConfig.h
    src/core/ZipUtils.cpp
    src/core/ZipUtils.h
)

target_include_directories(bik_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(bik_core PUBLIC ZLIB::ZLIB)

# CLI executable
add_executable(bik
    src/cli/main.cpp
    src/cli/CommandHandler.cpp
    src/cli/CommandHandler.h
)

target_link_libraries(bik PRIVATE bik_core)

# GUI executable (optional, requires Qt or FLTK)
option(BUILD_GUI "Build GUI application" ON)

if(BUILD_GUI)
    # Try to find FLTK for simple retro GUI
    find_package(FLTK)
    
    if(FLTK_FOUND)
        add_executable(bik_gui
            src/gui/main.cpp
            src/gui/MainWindow.cpp
            src/gui/MainWindow.h
        )
        
        target_include_directories(bik_gui PRIVATE ${FLTK_INCLUDE_DIR})
        target_link_libraries(bik_gui PRIVATE bik_core ${FLTK_LIBRARIES})
        
        install(TARGETS bik_gui DESTINATION bin)
    else()
        message(WARNING "FLTK not found. GUI will not be built. Install FLTK to enable GUI.")
    endif()
endif()

# Installation
install(TARGETS bik DESTINATION bin)
