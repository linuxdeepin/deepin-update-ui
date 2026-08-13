# SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.18)

set(PROJECT_NAME deepin-update-ui)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(QT_VERSION_MAJOR 6)

if (DEFINED ENABLE_MIEEE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif()

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()


include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

set(TRANSLATION_LANGUAGES
    af ak am_ET ar ar_EG ast az bg bn bo br ca cgg cs da de de_CH el en en_AU
    en_GB en_US eo es et eu fa fi fil fr gl_ES he hi_IN hr hu hy id it
    ja kab ka kk km_KH kn_IN ko ku ku_IQ ky ky@Arab la lo lt lv ml mn mr ms
    nb ne nl pa pam pl ps pt pt_BR ro ru sc si sk sl sq sr sv sw ta te th tr
    tzm ug uk ur uz vi zh_CN zh_HK zh_TW
)

add_subdirectory("src/dde-update")
add_subdirectory("src/dde-abrecovery")
add_subdirectory("src/dcc-update-plugin")
add_subdirectory("src/dock-update-plugin")
add_subdirectory("src/private-lastore-tray")


#-------------------------------dde-gather-info-----------------------------
set(GATHERINFO_NAME dde-gather-info)

find_package(Dtk6 COMPONENTS Gui Widget REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets DBus WebChannel WebEngineCore WebEngineWidgets)

file(GLOB_RECURSE DDE-GATHER-INFO_SRCS
    "src/dde-gather-info/*.h"
    "src/dde-gather-info/*.cpp"
    "src/dde-gather-info/*.qrc"
)
list(REMOVE_ITEM DDE-GATHER-INFO_SRCS "${CMAKE_SOURCE_DIR}/src/dde-gather-info/main.cpp")

add_executable(${GATHERINFO_NAME}
    ${DDE-GATHER-INFO_SRCS}
    src/dde-gather-info/main.cpp
)

target_link_libraries(${GATHERINFO_NAME} PRIVATE
    Qt6::Widgets
    Qt6::DBus
    Qt6::WebChannel
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Dtk6::Gui
    Dtk6::Widget
)

install(TARGETS ${GATHERINFO_NAME} DESTINATION bin)


# Install gather-info scripts
install(FILES src/dde-gather-info/start-gather-console-info.sh
    DESTINATION /var/lib/lastore/scripts
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(FILES src/dde-gather-info/lastore-tools-gatherinfo-autostart.desktop
    DESTINATION /etc/xdg/autostart
)
