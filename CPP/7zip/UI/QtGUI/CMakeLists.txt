cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Inherit the version from the top-level project()
set(APP_VERSION ${PROJECT_VERSION})

set(SOURCES
    main.cpp
    MainWindow.cpp
    ArchiveModel.cpp
)

add_executable(7zz-gui ${SOURCES})

target_compile_definitions(7zz-gui PRIVATE
    APP_VERSION="${APP_VERSION}"
)

target_link_libraries(7zz-gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_options(7zz-gui PRIVATE -Wall -Wextra)

install(TARGETS 7zz-gui
    RUNTIME DESTINATION bin
)

install(FILES 7zz-gui.desktop
    DESTINATION share/applications
)
