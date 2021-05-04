cmake_minimum_required(VERSION 3.1.0)

project(FEBioStudioUpdater)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_FILES_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER Autogen)

if(WIN32)
    add_definitions(-DWIN32)
    set(Qt5_DIR "C:/Qt/5.14.2/msvc2017_64")
    set(FEBIOSTUDIO_DIR "C:/Users/Steve/source/repos/FEBioStudio")
elseif(APPLE)
    add_definitions(-D__APPLE__)
    set(Qt5_DIR "/home/mherron/Resources/Qt/5.14.1/gcc_64/lib/cmake/Qt5")
    set(FEBIOSTUDIO_DIR "/home/mherron/Projects/FEBioStudio")
else()
	add_definitions(-DLINUX)
    set(Qt5_DIR "/home/mherron/Resources/Qt/5.14.1/gcc_64/lib/cmake/Qt5")
    set(FEBIOSTUDIO_DIR "/home/mherron/Projects/FEBioStudio")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

include_directories(.)
include_directories(${FEBIOSTUDIO_DIR})

file(GLOB SOURCES "*.cpp")

add_executable(FEBioStudioUpdater ${SOURCES} ${FEBIOSTUDIO_DIR}/FEBioStudio/UpdateChecker.cpp)

add_executable(mvUtil "mvUtil/mvUtil.cpp")

if(WIN32)
    SET_TARGET_PROPERTIES(FEBioStudioUpdater PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS")
    SET_TARGET_PROPERTIES(mvUtil PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator'\"")
endif()

if (WIN32)
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/VS2017/x64/Release/XML.lib)
elseif(APPLE)
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/build/Release/lib/libXML.a)
else()
    set(FEBIOSTUDIO_LIBS ${FEBIOSTUDIO_DIR}/build/Release/lib/libXML.a)
endif()

target_link_libraries(FEBioStudioUpdater Qt5::Widgets Qt5::Network ${FEBIOSTUDIO_LIBS})
