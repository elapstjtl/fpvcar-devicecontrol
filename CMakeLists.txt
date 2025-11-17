cmake_minimum_required(VERSION 3.14)

project(fpvcar-devicecontrol VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# FPV devicecontrol CMakeLists.txt
# =============================================================================
#
# 构建模式说明：
# 1. 开发环境：cmake ..
#    - 使用 git submodule 获取第三方库（fpvcar-motor）
#    - fpvcar-motor 本身包含 expected 作为 submodule
#    - 需要先执行: git submodule update --init --recursive
#    - 此时要求系统已安装 libgpiod (sudo apt install libgpiod-dev)
#    - 用于本地开发和测试
#
# 2. 生产环境（Yocto）：cmake ..
#    - 使用 Yocto sysroot 中的库
#    - 需要在 .bb 文件中添加:
#      DEPENDS += "nlohmann-json libgpiod-cxx"
#    - fpvcar-motor 通过 submodule 引入
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

# --- 1. 处理第三方依赖 ---
# 使用 git submodule 引入 fpvcar-motor
# fpvcar-motor 本身包含 expected 作为 submodule
set(FPVCAR_MOTOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/fpvcar-motor)

if(NOT EXISTS ${FPVCAR_MOTOR_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "fpvcar-motor submodule not found. Please run: git submodule update --init --recursive")
endif()

message(STATUS "Using fpvcar-motor from submodule: ${FPVCAR_MOTOR_DIR}")
add_subdirectory(${FPVCAR_MOTOR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/libs/fpvcar-motor)

# nlohmann_json: 使用 git submodule 引入
set(NLOHMANN_JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/nlohmann_json)

if(NOT EXISTS ${NLOHMANN_JSON_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "nlohmann_json submodule not found. Please run: git submodule update --init --recursive")
endif()

message(STATUS "Using nlohmann_json from submodule: ${NLOHMANN_JSON_DIR}")
add_subdirectory(${NLOHMANN_JSON_DIR} ${CMAKE_CURRENT_BINARY_DIR}/libs/nlohmann_json)


# --- 定义可执行程序目标 ---
add_executable(fpvcar-devicecontrol
    src/main.cpp
    src/device_control_service.cpp
    src/ipc_server.cpp
    src/request_handler.cpp
    src/config.cpp
    src/watch_dog.cpp
    src/control_loop.cpp
    src/desired_state.cpp
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
)

# nlohmann_json 链接
target_link_libraries(fpvcar-devicecontrol PRIVATE nlohmann_json::nlohmann_json)