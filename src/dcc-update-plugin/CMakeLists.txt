set(PROJECT_NAME dcc-update-plugin)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# Find the library
find_package(PkgConfig REQUIRED)
#find_package(DtkWidget REQUIRED)
#find_package(DtkGui REQUIRED)
#find_package(Qt5 COMPONENTS Core Widgets DBus Network Concurrent REQUIRED)
find_package(DdeControlCenter REQUIRED)

find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network LinguistTools Concurrent REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui REQUIRED)

message(STATUS "dde-control-center version:" ${DdeControlCenter_VERSION})

# 比较版本号
if(DdeControlCenter_VERSION VERSION_GREATER "5.7.5")
  add_definitions(-DDCC_MODULE_CATEGORY_VALID)
  message(STATUS "dcc module category is valid")
endif()

set(Update_Name update)

file(GLOB_RECURSE update_SRCS
        "operation/*.cpp"
        "operation/*.h"
        "operation/qrc/update.qrc"
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
        # Qt6::Quick
        Dtk6::Core
        Dtk6::Gui
        Dde::Control-Center
)


target_link_libraries(${Update_Name} PRIVATE
        ${Update_Libraries}
)

#target_include_directories(${PROJECT_NAME} PRIVATE
#    ${Qt6Core_INCLUDE_DIRS}
#    ${Qt6Widgets_INCLUDE_DIRS}
#    ${Qt6DBus_INCLUDE_DIRS}
#    ${Qt6Network_INCLUDE_DIRS}
#    ${Qt6Concurrent_INCLUDE_DIRS}
#    ${DtkCore_INCLUDE_DIRS}
#    ${DtkWidget_INCLUDE_DIRS}
#    ${DtkGui_INCLUDE_DIRS}
#    ${DdeControlCenter_INCLUDE_DIR}
#    "module"
#    "window"
#    "window/widgets"
#)
#
#target_link_libraries(${PROJECT_NAME} PRIVATE
#    ${Qt6Core_LIBRARIES}
#    ${Qt6Widgets_LIBRARIES}
#    ${Qt6DBus_LIBRARIES}
#    ${Qt6Network_LIBRARIES}
#    ${Qt6Concurrent_LIBRARIES}
#    ${DtkCore_LIBRARIES}
#    ${DtkWidget_LIBRARIES}
#    ${DtkGui_LIBRARIES}
#    ${DdeControlCenter_LIBRARIES}
#)

#install(FILES "misc/org.deepin.dde.control-center.update.json" DESTINATION share/dsg/configs/org.deepin.dde.control-center)

dcc_install_plugin(NAME ${Update_Name} TARGET ${Update_Name})

dcc_handle_plugin_translation(NAME ${Update_Name} )


#install(TARGETS ${PROJECT_NAME} DESTINATION lib/dde-control-center/modules)
