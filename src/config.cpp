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
    // 读取 IPC 套接字路径，如果不存在则使用默认值
    cfg.ipc_socket_path = j.value("ipc_socket_path", cfg.ipc_socket_path);

    // 解析引脚配置：必须存在且为对象类型
    if (j.contains("pins") && j["pins"].is_object()) {
        const auto& p = j["pins"]; // p为j["pins"]对象
        // 解析四个电机的引脚配置（前左、前右、后左、后右）
        // 每个电机有两个控制引脚（A 和 B）和一个待机引脚（stby）
        cfg.pins.fl_pin_a = p.value("fl_pin_a", cfg.pins.fl_pin_a);
        cfg.pins.fl_pin_b = p.value("fl_pin_b", cfg.pins.fl_pin_b);
        cfg.pins.fr_pin_a = p.value("fr_pin_a", cfg.pins.fr_pin_a);
        cfg.pins.fr_pin_b = p.value("fr_pin_b", cfg.pins.fr_pin_b);
        cfg.pins.bl_pin_a = p.value("bl_pin_a", cfg.pins.bl_pin_a);
        cfg.pins.bl_pin_b = p.value("bl_pin_b", cfg.pins.bl_pin_b);
        cfg.pins.br_pin_a = p.value("br_pin_a", cfg.pins.br_pin_a);
        cfg.pins.br_pin_b = p.value("br_pin_b", cfg.pins.br_pin_b);
        cfg.pins.stby_pin = p.value("stby_pin", cfg.pins.stby_pin);
    } else {
        // 引脚配置是必需的，缺失或格式错误则返回错误
        return tl::unexpected(std::string("Missing or invalid 'pins' object in config: ") + file_path);
    }

    return cfg;
}

}


