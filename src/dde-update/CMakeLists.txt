
set(PROJECT_NAME dde-update)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)
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

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE
    "../common"
    "../common/common"
)

target_sources(${PROJECT_NAME} PUBLIC ${PowerSrc})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk6::Widget
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

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
