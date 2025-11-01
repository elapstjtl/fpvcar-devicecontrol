#pragma once
#include <string>
#include <tl/expected.hpp>
#include "fpvcar-motor/config.hpp" // 引入 FpvCarPinConfig

namespace fpvcar::device_control::config {
    /**
     * @brief 应用配置结构体
     * @param pins 小车电机引脚配置，包含四个电机的控制引脚和待机引脚
     * @param ipc_socket_path IPC 通信使用的 Unix 域套接字文件路径，默认值为 /tmp/fpvcar_control.sock
     */
    struct AppConfig {
        fpvcar::config::FpvCarPinConfig pins; // 小车电机引脚配置
        std::string ipc_socket_path = "/tmp/fpvcar_control.sock";
    };

    /**
     * @brief 从 JSON 文件中加载并解析应用配置
     * @param file_path 配置文件路径
     * @return 成功返回解析后的 AppConfig 对象，失败返回错误信息字符串
     * @note JSON 文件必须包含 "pins" 对象，其中包含所有必需的引脚配置
     * @note 如果缺少某些可选字段（如 ipc_socket_path），将使用默认值
     */
    tl::expected<AppConfig, std::string> load_config(const std::string& file_path);
}