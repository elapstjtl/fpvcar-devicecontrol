#include "fpvcar_device_control/config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using nlohmann::json;

namespace fpvcar::device_control::config {

tl::expected<AppConfig, std::string> load_config(const std::string& file_path) {
    // 打开配置文件
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        return tl::unexpected(std::string("Failed to open config file: ") + file_path);
    }

    // 解析 JSON，禁用异常（使用 is_discarded 判断失败）
    json j = json::parse(ifs, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) {
        return tl::unexpected(std::string("Failed to parse JSON from: ") + file_path);
    }

    // 创建配置对象，使用默认值初始化
    AppConfig cfg;
    // 初始化通道配置为默认值
    cfg.channels = fpvcar::motorconfig::DEFAULT_CHANNELS;
    
    // 读取可选配置项，如果不存在则使用默认值
    cfg.ipc_socket_path = j.value("ipc_socket_path", cfg.ipc_socket_path);
    cfg.i2c_device_path = j.value("i2c_device_path", cfg.i2c_device_path);
    cfg.pwm_frequency = j.value("pwm_frequency", cfg.pwm_frequency);
    
    // 解析PCA9685地址（可以是整数或十六进制字符串）
    if (j.contains("pca9685_address")) {
        if (j["pca9685_address"].is_number()) {
            cfg.pca9685_address = j["pca9685_address"].get<uint8_t>();
        } else if (j["pca9685_address"].is_string()) {
            // 支持十六进制字符串，如 "0x40"
            std::string addr_str = j["pca9685_address"].get<std::string>();
            cfg.pca9685_address = static_cast<uint8_t>(std::stoul(addr_str, nullptr, 0));
        }
    }

    // 解析通道配置：必须存在且为对象类型
    if (j.contains("channels") && j["channels"].is_object()) {
        const auto& ch = j["channels"]; // ch为j["channels"]对象
        // 解析四个电机的通道配置（前左、前右、后左、后右）
        // 每个电机有3个通道：速度通道、方向通道1、方向通道2
        cfg.channels.fl_channel_speed = ch.value("fl_channel_speed", cfg.channels.fl_channel_speed);
        cfg.channels.fl_channel_1 = ch.value("fl_channel_1", cfg.channels.fl_channel_1);
        cfg.channels.fl_channel_2 = ch.value("fl_channel_2", cfg.channels.fl_channel_2);
        cfg.channels.fr_channel_speed = ch.value("fr_channel_speed", cfg.channels.fr_channel_speed);
        cfg.channels.fr_channel_1 = ch.value("fr_channel_1", cfg.channels.fr_channel_1);
        cfg.channels.fr_channel_2 = ch.value("fr_channel_2", cfg.channels.fr_channel_2);
        cfg.channels.bl_channel_speed = ch.value("bl_channel_speed", cfg.channels.bl_channel_speed);
        cfg.channels.bl_channel_1 = ch.value("bl_channel_1", cfg.channels.bl_channel_1);
        cfg.channels.bl_channel_2 = ch.value("bl_channel_2", cfg.channels.bl_channel_2);
        cfg.channels.br_channel_speed = ch.value("br_channel_speed", cfg.channels.br_channel_speed);
        cfg.channels.br_channel_1 = ch.value("br_channel_1", cfg.channels.br_channel_1);
        cfg.channels.br_channel_2 = ch.value("br_channel_2", cfg.channels.br_channel_2);
    } else {
        // 通道配置是必需的，缺失或格式错误则返回错误
        return tl::unexpected(std::string("Missing or invalid 'channels' object in config: ") + file_path);
    }

    return cfg;
}

}


