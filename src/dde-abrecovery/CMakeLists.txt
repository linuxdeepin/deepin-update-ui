# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(ROLLBACK_EXEC_NAME dde-rollback)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")


# Find the library
find_package(PkgConfig REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets DBus REQUIRED)

file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "*.qrc"
    "../common/*.h"
    "../common/*.cpp"
)

add_executable(${ROLLBACK_EXEC_NAME} ${SRCS})

target_include_directories(${ROLLBACK_EXEC_NAME} PUBLIC
    "../common"
    "../common/common"
)

target_link_libraries(${ROLLBACK_EXEC_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk6::Widget
)

set(TRANSLATION_FILES "")
foreach(LANG ${TRANSLATION_LANGUAGES})
    list(APPEND TRANSLATION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/dde-rollback_${LANG}.ts")
endforeach()

# can generate ts and qm file
qt_add_translations(${ROLLBACK_EXEC_NAME}
    SOURCE_TARGETS ${ROLLBACK_EXEC_NAME}
    TS_FILES ${TRANSLATION_FILES}
    SOURCES ${SRCS}
    LUPDATE_OPTIONS -no-obsolete -no-ui-lines -locations none
    QM_FILES_OUTPUT_VARIABLE TR_QM_FILES
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/translations
)

include(GNUInstallDirs)

## qm files
install(FILES ${TR_QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations)

install(TARGETS ${ROLLBACK_EXEC_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
# 需要在greeter之前启动的配置和脚本
install(FILES "misc/98_rollback_conf.json" DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-session-shell/greeters.d/launch.conf.d)
install(PROGRAMS "misc/rollback_check.sh" DESTINATION ${CMAKE_INSTALL_BINDIR})
