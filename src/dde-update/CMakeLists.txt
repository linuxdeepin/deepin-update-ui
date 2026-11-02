# SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(UPDATE_EXEC_NAME dde-update)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

install(TARGETS ${UPDATE_EXEC_NAME} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/deepin)

install(FILES "misc/99_system_upgrade_conf.json"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-session-shell/greeters.d/launch.conf.d)

# DConfig: 更新完成欢迎界面配置
install(FILES "misc/org.deepin.dde.update.json"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dsg/configs/org.deepin.dde.update)

install(FILES "misc/dde-update-autostart.desktop"
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart)

install(FILES "misc/copy-update-log.sh" "misc/dde-update-env.sh"
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/deepin-update-ui
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES "misc/deepin-update-log-copy@.service"
        DESTINATION lib/systemd/system
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(FILES "misc/dde-update-env.service"
        DESTINATION lib/systemd/user
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(FILES "misc/52-deepin-update-ui.rules"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/rules.d/
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(PROGRAMS "misc/98deepin-upgrade-check"
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/X11/Xsession.d)

# treeland prestart hook：等价于 X11 的 greeters.d/launch.conf.d
#   10-system_upgrade_check.sh -> 99_system_upgrade_conf.json（dde-update）
# 编号从 10 起，给更早执行的钩子留空间；
# 钩子在锁屏加载之前串行执行，脚本内部前台 exec，GUI 退出后才返回。
install(PROGRAMS
        "misc/10-system_upgrade_check.sh"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin/treeland/prestart.d/blocking)

# 登录后系统检查（AfterGreeterCheck）：等价 X11 的 /etc/X11/Xsession.d/98deepin-upgrade-check
# 挂在 dde-session-pre.target 下且 Before=dde-session-pre.target，
# 使 dde-shell（After=pre.target）等检查完成后再启动。
install(PROGRAMS "misc/dde-update-after-login-check.sh" DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "misc/dde-update-after-login-check.service"
        DESTINATION lib/systemd/user
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
set(_after_login_wants_dir "${CMAKE_CURRENT_BINARY_DIR}/systemd-wants")
file(MAKE_DIRECTORY "${_after_login_wants_dir}")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ../dde-update-after-login-check.service
        "${_after_login_wants_dir}/dde-update-after-login-check.service")
install(FILES "${_after_login_wants_dir}/dde-update-after-login-check.service"
        DESTINATION lib/systemd/user/dde-session-pre.target.wants)

install(PROGRAMS 
        "misc/dde-update"
        "misc/system_upgrade_check.sh"
        "misc/run-kwayland-check-wrapper.sh"
        "misc/run-kwayland-check.sh"
        "misc/launch-dde-update"
        DESTINATION ${CMAKE_INSTALL_BINDIR})
