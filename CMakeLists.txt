# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.18)

set(PROJECT_NAME deepin-update-ui)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(QT_VERSION_MAJOR 6)

# 增加安全编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all -fPIC")
set(CMAKE_EXE_LINKER_FLAGS  "-z relro -z now -z noexecstack -pie")
add_compile_options(-fPIC)

if (DEFINED ENABLE_MIEEE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mieee")
endif()

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()


include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

set(TRANSLATION_LANGUAGES
    af ak am_ET ar ar_EG ast az bg bn bo br ca cgg cs da de de_CH el en en_AU
    en_GB en_US eo es et eu fa fi fil fr gl_ES he hi_IN hr hu hy id it
    ja kab ka kk km_KH kn_IN ko ku ku_IQ ky ky@Arab la lo lt lv ml mn mr ms
    nb ne nl pa pam pl ps pt pt_BR ro ru sc si sk sl sq sr sv sw ta te th tr
    tzm ug uk ur uz vi zh_CN zh_HK zh_TW
)

add_subdirectory("src/dde-update")
add_subdirectory("src/dde-abrecovery")
add_subdirectory("src/dcc-update-plugin")

