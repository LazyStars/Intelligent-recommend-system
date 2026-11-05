cmake_minimum_required(VERSION 3.24)

set(PROJECT_NAME "recommendation_client")
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   #关闭 禁止使用编译器扩展

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Network
)

#一键应用 Qt 项目的标准默认配置
qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    resources/resources.qrc

    src/app/app.cpp
    src/app/app.h
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
)