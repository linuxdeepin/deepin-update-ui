# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: CC0-1.0

find_package(Qt6 REQUIRED COMPONENTS Test Widgets DBus Network Qml)
find_package(Dtk6 REQUIRED COMPONENTS Core Gui)
find_package(DdeControlCenter REQUIRED)
find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)

add_executable(check-update-test check-update-test.cpp)
target_include_directories(check-update-test PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/dcc-update-plugin"
)
# Link the exact MODULE file (update.so has neither a lib prefix nor SONAME).
# Exercise the actual plugin implementation, including its Qt meta objects.
target_link_libraries(check-update-test PRIVATE
    "-Wl,$<TARGET_FILE:update>"
    Qt6::Test Qt6::Widgets Qt6::DBus Qt6::Network Qt6::Qml
    Dtk6::Core Dtk6::Gui Dde::Control-Center
)
add_dependencies(check-update-test update)
add_test(NAME check-update-lifecycle
    COMMAND ${DBUS_RUN_SESSION}
        --config-file=${CMAKE_CURRENT_SOURCE_DIR}/dbus-test.conf
        -- $<TARGET_FILE:check-update-test>
)
set_tests_properties(check-update-lifecycle PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)
