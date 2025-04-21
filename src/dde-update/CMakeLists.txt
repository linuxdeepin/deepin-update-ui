# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(UPDATE_EXEC_NAME dde-update)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets DBus LinguistTools REQUIRED)

file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "*.qrc"
    "../common/*.h"
    "../common/*.cpp"
)

add_executable(${UPDATE_EXEC_NAME} ${SRCS})

set(TRANSLATION_FILES "")
foreach(LANG ${TRANSLATION_LANGUAGES})
    list(APPEND TRANSLATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/dde-update_${LANG}.ts")
endforeach()

# can generate ts and qm file
qt_add_translations(${UPDATE_EXEC_NAME}
    SOURCE_TARGETS ${UPDATE_EXEC_NAME}
    TS_FILES ${TRANSLATION_FILES}
    SOURCES ${SRCS}
    LUPDATE_OPTIONS -no-obsolete -no-ui-lines -locations none
    QM_FILES_OUTPUT_VARIABLE TR_QM_FILES
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/translations
)

target_include_directories(${UPDATE_EXEC_NAME} PRIVATE
    "../common"
    "../common/common"
)

target_sources(${UPDATE_EXEC_NAME} PUBLIC ${PowerSrc})

target_link_libraries(${UPDATE_EXEC_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk6::Widget
)

include(GNUInstallDirs)

## qm files
install(FILES ${TR_QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)

install(TARGETS ${UPDATE_EXEC_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "misc/99_system_upgrade_conf.json"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-session-shell/greeters.d/launch.conf.d)

install(FILES "misc/dde-update-autostart.desktop"
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)

install(PROGRAMS "misc/98deepin-upgrade-check"
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/X11/Xsession.d)

install(PROGRAMS 
        "misc/system_upgrade_check.sh"
        "misc/run-kwayland-check-wrapper.sh"
        "misc/run-kwayland-check.sh"
        DESTINATION ${CMAKE_INSTALL_BINDIR})
