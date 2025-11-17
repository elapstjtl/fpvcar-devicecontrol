#pragma once
#include <string>
#include <cstdint>
#include <tl/expected.hpp>
#include "fpvcar-motor/config.hpp" // 引入 FpvCarChannelConfig

namespace fpvcar::device_control::config {
    /**
     * @brief 应用配置结构体
     * @param channels 小车电机通道配置，包含四个电机在PCA9685上的通道号
     * @param i2c_device_path I2C设备路径，默认值为 "/dev/i2c-1"
     * @param pwm_frequency PWM频率（Hz），默认值为10000.0
     * @param pca9685_address PCA9685的I2C地址，默认值为0x40
     * @param ipc_socket_path IPC 通信使用的 Unix 域套接字文件路径，默认值为 /tmp/fpvcar_control.sock
     */
    struct AppConfig {
        fpvcar::motorconfig::FpvCarChannelConfig channels; // 小车电机通道配置
        std::string i2c_device_path = fpvcar::motorconfig::I2C_DEVICE_PATH;
        float pwm_frequency = fpvcar::motorconfig::DEFAULT_PWM_FREQUENCY;
        uint8_t pca9685_address = fpvcar::motorconfig::PCA9685_I2C_ADDRESS;
        std::string ipc_socket_path = "/tmp/fpvcar_control.sock";
    };

    /**
     * @brief 从 JSON 文件中加载并解析应用配置
     * @param file_path 配置文件路径
     * @return 成功返回解析后的 AppConfig 对象，失败返回错误信息字符串
     * @note JSON 文件必须包含 "channels" 对象，其中包含所有必需的通道配置
     * @note 如果缺少某些可选字段（如 i2c_device_path, pwm_frequency等），将使用默认值
     */
    tl::expected<AppConfig, std::string> load_config(const std::string& file_path);
}