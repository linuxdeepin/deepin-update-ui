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

find_package(Qt6 COMPONENTS Core Gui DBus Network LinguistTools Concurrent REQUIRED)
find_package(Dtk6 COMPONENTS Core Gui REQUIRED)

pkg_check_modules(QGSettings REQUIRED gsettings-qt)

message(STATUS "dde-control-center version:" ${DdeControlCenter_VERSION})

# 比较版本号
if(DdeControlCenter_VERSION VERSION_GREATER "5.7.5")
  add_definitions(-DDCC_MODULE_CATEGORY_VALID)
  message(STATUS "dcc module category is valid")
endif()

file(GLOB_RECURSE SRCS
        "operation/*.cpp"
        "operation/*.h"
        "operation/qrc/update.qrc"
)

add_library(${PROJECT_NAME} MODULE
        ${SRCS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
        Dde::Control-Center
)

set(DccUpdate_Libraries
        Qt6::DBus
        Qt6::Gui
        Dtk6::Gui
        Qt6::Concurrent
        # Qt6::Quick
        Qt::Network
        Dtk6::Core
        Dde::Control-Center
)


target_link_libraries(${PROJECT_NAME} PRIVATE
        ${DccUpdate_Libraries}
)

#target_include_directories(${PROJECT_NAME} PRIVATE
#    ${Qt6Core_INCLUDE_DIRS}
#    ${Qt6Widgets_INCLUDE_DIRS}
#    ${Qt6DBus_INCLUDE_DIRS}
#    ${Qt6Network_INCLUDE_DIRS}
#    ${Qt6Concurrent_INCLUDE_DIRS}
#    ${QGSettings_INCLUDE_DIRS}
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
#    ${QGSettings_LIBRARIES}
#    ${DtkCore_LIBRARIES}
#    ${DtkWidget_LIBRARIES}
#    ${DtkGui_LIBRARIES}
#    ${DdeControlCenter_LIBRARIES}
#)

install(FILES "misc/org.deepin.dde.control-center.update.json" DESTINATION share/dsg/configs/org.deepin.dde.control-center)

dcc_install_plugin(NAME ${PROJECT_NAME} TARGET ${PROJECT_NAME})

dcc_handle_plugin_translation(NAME ${PROJECT_NAME} )


#install(TARGETS ${PROJECT_NAME} DESTINATION lib/dde-control-center/modules)
