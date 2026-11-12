cmake_minimum_required(VERSION 3.24)

set(PROJECT_NAME "recommendation_client")
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_EXTENSIONS OFF)   #关闭 禁止使用编译器扩展

find_package(Qt6 6.8 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Network
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(QWINDOWKIT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../3rdParty/debug/qwindowkit")
else()
    set(QWINDOWKIT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../3rdParty/release/qwindowkit")
endif()

set(QWindowKit_DIR "${QWINDOWKIT_PREFIX}/lib/cmake/QWindowKit")
if(NOT EXISTS "${QWindowKit_DIR}/QWindowKitConfig.cmake")
    message(FATAL_ERROR
            "QWindowKit is not prepared for ${CMAKE_BUILD_TYPE}. "
            "Run scripts/Prepare-QWindowKit.ps1 from the repository root."
    )
endif()

find_package(QWindowKit 1.5.1 CONFIG REQUIRED COMPONENTS Widgets)

#一键应用 Qt 项目的标准默认配置
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/singleapplication.cpp
    src/singleapplication.h
    resources/resources.qrc

    src/app/app.cpp
    src/app/app.h
    src/app/app.ui

    src/auth/steamauthapi.cpp
    src/auth/steamauthapi.h
    src/auth/steamauthprotocol.h

    src/core/apiclient.cpp
    src/core/apiclient.h
    src/core/apierror.h
    src/core/apijsonreply.h
    src/core/apijsonreply.cpp

    src/dialogs/generalpopup/generalpopup.h
    src/dialogs/generalpopup/generalpopup.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network

        QWindowKit::Widgets
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:QWindowKit::Core>
        $<TARGET_FILE:QWindowKit::Widgets>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QWINDOWKIT_PREFIX}/bin/QWKCored.pdb"
            "${QWINDOWKIT_PREFIX}/bin/QWKWidgetsd.pdb"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
