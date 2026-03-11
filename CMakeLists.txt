cmake_minimum_required(VERSION 3.16)
project(haiclaude LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_resources(RESOURCES resources.qrc)

qt_add_executable(haiclaude
    main.cpp
    LauncherWindow.cpp
    LauncherWindow.h
    ${RESOURCES}
)

target_link_libraries(haiclaude PRIVATE Qt6::Widgets)

install(TARGETS haiclaude RUNTIME DESTINATION bin)
