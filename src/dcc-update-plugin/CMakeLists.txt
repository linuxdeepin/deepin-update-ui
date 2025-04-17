set(PROJECT_NAME dcc-update-plugin)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(DdeControlCenter REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network LinguistTools Concurrent REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui REQUIRED)

message(STATUS "dde-control-center version:" ${DdeControlCenter_VERSION})

set(Update_Name update)

file(GLOB_RECURSE update_SRCS
        "operation/*.cpp"
        "operation/*.h"
        "operation/qrc/update.qrc"
        "../common/*.h"
        "../common/*.cpp"
)

add_library(${Update_Name} MODULE
        ${update_SRCS}
)

target_include_directories(${Update_Name} PUBLIC
        Dde::Control-Center
)

set(Update_Libraries
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        Qt6::Concurrent
        Qt::Network
        Dtk6::Core
        Dtk6::Gui
        Dde::Control-Center
)

target_link_libraries(${Update_Name} PRIVATE
        ${Update_Libraries}
)

install(FILES "misc/org.deepin.dde.control-center.update.json" DESTINATION share/dsg/configs/org.deepin.dde.control-center)
dcc_install_plugin(NAME ${Update_Name} TARGET ${Update_Name})
dcc_handle_plugin_translation(NAME ${Update_Name} )
