# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(PLUGIN_NAME "dock-update-plugin")

project(${PLUGIN_NAME})
include(GNUInstallDirs)
# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)
# Sources files
file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "widget/*.h"
    "widget/*.cpp"
    "resources/plugin-update.qrc"
)
set(QT_VERSION_MAJOR 6)
set(DTK_VERSION_MAJOR 6)

find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Widgets Core LinguistTools)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Core Gui)
find_package(DdeDock REQUIRED)

add_library(${PLUGIN_NAME} SHARED ${SRCS})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Core
    Dtk${DTK_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)

file(GLOB TS_FILES "translations/*.ts")

qt_add_translations(${PLUGIN_NAME}_language
    TS_FILES ${TS_FILES}
    SOURCES ${SRCS}
    LUPDATE_OPTIONS -no-obsolete -no-ui-lines -locations none
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)
add_custom_target(${PLUGIN_NAME}_language ALL DEPENDS ${QM_FILES})

install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PLUGIN_NAME}/translations)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/plugins)
install(FILES "resources/dcc-plugin-update.dci" DESTINATION share/dde-dock/icons/dcc-setting)
