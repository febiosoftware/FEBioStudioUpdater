cmake_minimum_required(VERSION 3.1.0)

project(FEBioStudioUpdater)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_FILES_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER Autogen)


# Change to 6 if looking to build with Qt6.
set(Qt_Ver 6 CACHE STRING "Major version number of Qt.")
set(Qt_Version Qt${Qt_Ver})

if(NOT Qt_Root)
    set(Qt_Root "" CACHE PATH "Path to the root Qt directory (e.g. /opt/Qt)")
endif()

if(NOT CMAKE_PREFIX_PATH)
    
    if(WIN32)
        find_path(Qt_TEMP lib/cmake/Qt6/Qt6Config.cmake
            PATHS ${Qt_Root}/ ${Qt_Root}/* ${Qt_Root}/*/* C:/Qt/* C:/Qt*/* $ENV{HOME}/Qt/* $ENV{HOME}/*/Qt/* /Qt/* /Qt*/* 
            PATH_SUFFIXES "msvc2015_64" "msvc2017_64" "msvc2019_64"
            DOC "Qt CMake Directory")
    elseif(APPLE)
        find_path(Qt_TEMP lib/cmake/Qt6/Qt6Config.cmake
            PATHS ${Qt_Root}/ ${Qt_Root}/* ${Qt_Root}/*/* /opt/Qt/* /opt/Qt*/* /usr/local/Qt/* /usr/local/Qt*/* 
                $ENV{HOME}/Qt/* $ENV{HOME}/*/Qt/* /Qt/* /Qt*/* 
            PATH_SUFFIXES "clang_64"
            DOC "${Qt_Version} CMake Directory")
    else()
        find_path(Qt_TEMP lib/cmake/Qt6/Qt6Config.cmake
            PATHS ${Qt_Root}/ ${Qt_Root}/* ${Qt_Root}/*/* /opt/Qt/* /opt/Qt*/* /usr/local/Qt/* /usr/local/Qt*/* 
                $ENV{HOME}/Qt/* $ENV{HOME}/*/Qt6 $ENV{HOME}/*/Qt/* /Qt/* /Qt*/* 
            PATH_SUFFIXES "gcc_64"
            DOC "Qt CMake Directory")
    endif()
    
    if(Qt_TEMP)
        set(CMAKE_PREFIX_PATH ${Qt_TEMP} CACHE INTERNAL "Path to Qt6 installation prefix.")
        unset(Qt_TEMP CACHE)
    else()
        unset(Qt_TEMP CACHE)
    endif()
    
    if(NOT CMAKE_PREFIX_PATH)
        message(FATAL_ERROR "Unable to locate ${Qt_Version}Config.cmake. Please check the value of ${Qt_Version}_Root and set it to the root directory of your ${Qt_Version} installation (e.g. /opt/Qt).")
    endif()
endif()


if(WIN32)
    add_definitions(-DWIN32)
    #~ set(Qt6_DIR "C:/Qt/5.14.2/msvc2017_64")
    set(FEBIOSTUDIO_DIR "C:/Users/Steve/source/repos/FEBioStudio")
elseif(APPLE)
    add_definitions(-D__APPLE__)
    #~ set(Qt6_DIR "/home/mherron/Resources/Qt/5.14.1/gcc_64/lib/cmake/Qt5")
    set(FEBIOSTUDIO_DIR "/Users/mherron/Projects/FEBioStudio")
else()
	add_definitions(-DLINUX)
    #~ set(CMAKE_PREFIX_PATH "/home/mherron/Resources/Qt6/6.0.0/gcc_64/" CACHE INTERNAL)
    set(FEBIOSTUDIO_DIR "/home/mherron/Projects/FEBioStudio")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt6 COMPONENTS Widgets Network REQUIRED)

include_directories(.)
include_directories(${FEBIOSTUDIO_DIR})

file(GLOB SOURCES "*.cpp")

add_executable(FEBioStudioUpdater ${SOURCES} FEBioStudioUpdater.qrc ${FEBIOSTUDIO_DIR}/FEBioStudio/UpdateChecker.cpp)

add_executable(mvUtil "mvUtil/mvUtil.cpp")

if(WIN32)
    SET_TARGET_PROPERTIES(FEBioStudioUpdater PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
    SET_TARGET_PROPERTIES(mvUtil PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator'\"")
endif()

if (WIN32)
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/VS2017/x64/Release/XML.lib)
elseif(APPLE)
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/build/lib/libXML.a)
else()
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/build/Release/lib/libXML.a)
endif()

target_link_libraries(FEBioStudioUpdater Qt6::Widgets Qt6::Network ${FEBIOSTUDIO_LIBS})
