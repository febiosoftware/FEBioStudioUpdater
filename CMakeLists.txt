cmake_minimum_required(VERSION 3.1.0)

project(FEBioStudioUpdater)

set(Qt5_DIR "/home/mherron/Resources/Qt/5.14.1/gcc_64/lib/cmake/Qt5")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

include_directories(.)

file(GLOB SOURCES "[^moc_]*.cpp")

add_executable(FEBioStudioUpdater ${SOURCES})

target_link_libraries(FEBioStudioUpdater Qt5::Widgets Qt5::Network)
