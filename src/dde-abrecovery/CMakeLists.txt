
set(PROJECT_NAME dde-rollback)
project(${PROJECT_NAME})

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

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
    "../common"
    "../common/common"
)

target_link_libraries(${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk6::Widget
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
# 需要在greeter之前启动的配置和脚本
install(FILES "misc/98_rollback_conf.json" DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-session-shell/greeters.d/launch.conf.d)
install(PROGRAMS "misc/rollback_check.sh" DESTINATION ${CMAKE_INSTALL_BINDIR})
