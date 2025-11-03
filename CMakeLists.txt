cmake_minimum_required(VERSION 3.14)

project(fpvcar-devicecontrol VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# FPV motor CMakeLists.txt
# =============================================================================
#
# 构建模式说明：
# 1. 开发环境：cmake -DUSE_FETCHCONTENT=ON ..
#    - 自动下载 tl::expected 和 CLI11 依赖库
#    - 此时要求系统已安装 libgpiod (sudo apt install libgpiod-dev)
#    - 用于本地开发和测试
#
# 2. 生产环境（Yocto）：cmake ..
#    - 使用 Yocto sysroot 中的所有库
#    - 需要在 .bb 文件中添加:
#      DEPENDS += "tl-expected cli11 libgpiod-cxx"
#    - 用于嵌入式系统构建
#
# =============================================================================

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置调试信息路径映射，避免 Yocto 构建时的 buildpaths 警告
if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdebug-prefix-map=${CMAKE_SOURCE_DIR}=/usr/src/debug/fpvcar-motor/1.0")
endif()

# Yocto构建优化：设置适当的编译标志
if(CMAKE_CROSSCOMPILING)
    # 交叉编译时的优化设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
    message(STATUS "Cross-compiling detected, using optimized flags for Yocto")
endif()

# 查找 CMake 依赖发现工具（若需要 pkg-config 的其它系统库可启用）
# find_package(PkgConfig REQUIRED)

# --- 1. 条件化处理 C++/CMake 依赖 ---
# 检查是否启用 FetchContent（用于开发环境）
option(USE_FETCHCONTENT "Use FetchContent to download dependencies" OFF)

if(USE_FETCHCONTENT)
    # 开发环境：使用 FetchContent 下载依赖
    message(STATUS "Using FetchContent")
    include(FetchContent)
    
    # tl::expected
    FetchContent_Declare(
        expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected.git
        GIT_TAG v1.1.0
    )
    FetchContent_MakeAvailable(expected)
    add_library(TartanLlama::expected ALIAS expected)

    # nlohmann_json (header-only)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    
    # 从源码引入 fpvcar-motor（开发环境，使用相对路径）
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../fpvcar-motor ${CMAKE_CURRENT_BINARY_DIR}/_deps/fpvcar-motor)

    
else()
    # 生产/Yocto 环境：使用 Yocto sysroot 中的库
    message(STATUS "Using find_package for external deps (Yocto build)")
    # find_package(expected REQUIRED)
    find_package(nlohmann_json REQUIRED)
    find_package(fpvcar-motor REQUIRED)
endif()


# --- 定义可执行程序目标 ---
add_executable(fpvcar-devicecontrol
    src/main.cpp
    src/device_control_service.cpp
    src/ipc_server.cpp
    src/request_handler.cpp
    src/config.cpp
    src/watch_dog.cpp
)

# 头文件（本项目对外/内部包含路径）
target_include_directories(fpvcar-devicecontrol
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接依赖
target_link_libraries(fpvcar-devicecontrol
    PRIVATE
        fpvcar::motor
        nlohmann_json::nlohmann_json
)

if(USE_FETCHCONTENT)
    target_link_libraries(fpvcar-devicecontrol PRIVATE TartanLlama::expected)
endif()