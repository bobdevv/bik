cmake_minimum_required(VERSION 3.15)
project(bik VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(ZLIB REQUIRED)

# Try to find a cross-platform zip library
set(BIK_HAVE_LIBZIP OFF)
set(BIK_HAVE_MINIZIP OFF)

find_package(libzip QUIET)
if(libzip_FOUND)
    set(BIK_HAVE_LIBZIP ON)
endif()

# Some environments provide MiniZip
find_package(MiniZip QUIET)
if(MiniZip_FOUND)
    set(BIK_HAVE_MINIZIP ON)
endif()

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

if(BIK_HAVE_LIBZIP)
    target_link_libraries(bik_core PUBLIC libzip::zip)
    target_compile_definitions(bik_core PUBLIC BIK_HAVE_LIBZIP)
elseif(BIK_HAVE_MINIZIP)
    target_link_libraries(bik_core PUBLIC MiniZip::minizip)
    target_compile_definitions(bik_core PUBLIC BIK_HAVE_MINIZIP)
else()
    message(FATAL_ERROR "No zip backend found. Please install libzip or minizip.")
endif()

# CLI executable
add_executable(bik
    src/cli/main.cpp
    src/cli/CommandHandler.cpp
    src/cli/CommandHandler.h
)

target_link_libraries(bik PRIVATE bik_core)

# Installation
install(TARGETS bik DESTINATION bin)
