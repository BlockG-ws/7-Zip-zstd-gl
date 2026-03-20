cmake_minimum_required(VERSION 3.16)
project(7zip-zstd-gui VERSION 24.09 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The Qt GUI target is only supported on non-Windows platforms
if(NOT WIN32)
    find_package(Qt6 QUIET COMPONENTS Core Widgets)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
        message(STATUS "Using Qt6")
    else()
        find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
        set(QT_VERSION_MAJOR 5)
        message(STATUS "Using Qt5")
    endif()

    add_subdirectory(CPP/7zip/UI/QtGUI)
else()
    message(STATUS "Windows detected: use CPP/build-it.cmd to build the Windows GUI")
endif()
