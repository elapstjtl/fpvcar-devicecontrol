#include "fpvcar_device_control/request_handler.hpp"
#include <nlohmann/json.hpp>
#include <functional>

using nlohmann::json;

namespace fpvcar::device_control {

RequestHandler::RequestHandler(fpvcar::control::FpvCarController& car)
    : m_controller(car),
      m_watchdog(std::chrono::milliseconds(5000), [this]() { m_controller.stopAll(); }) {
    // "注册"所有可用的动作
    m_action_map["moveForward"]  = [this]() { m_controller.moveForward(); };
    m_action_map["moveBackward"] = [this]() { m_controller.moveBackward(); };
    m_action_map["turnLeft"]     = [this]() { m_controller.turnLeft(); };
    m_action_map["turnRight"]    = [this]() { m_controller.turnRight(); };
    m_action_map["stopAll"]      = [this]() { m_controller.stopAll(); };
    
    // 启动看门狗监控
    m_watchdog.start();
}

std::string RequestHandler::handle_request(const std::string& json_request) {
    // 喂看门狗：每次收到请求时重置超时计时器
    m_watchdog.feed();
    
    // 解析 JSON 请求，禁用异常机制，通过 is_discarded 判断解析失败
    auto data = json::parse(json_request, nullptr, /*allow_exceptions=*/false);
    if (data.is_discarded()) {
        return create_error_response("INVALID_JSON", "Failed to parse JSON");
    }

    // 提取 action 字段，如果不存在则使用空字符串
    const std::string action = data.value("action", "");
    if (action.empty()) {
        return create_error_response("INVALID_JSON", "Missing 'action' field");
    }

    // 定义一个 lambda 函数，用于执行函数对象，并返回结果
    auto exec = [&](auto&& call, const char* ok_msg) {
        auto res = safe_call(call);
        if (!res) return create_error_response("HARDWARE_ERROR", res.error());
        return create_success_response(ok_msg);
    };

    // 查找 action 对应的函数对象，如果不存在则执行 stopAll 并返回错误响应
    auto it = m_action_map.find(action);
    if (it == m_action_map.end()) {
        // 收到无效动作时，为了安全起见，执行 stopAll 停止所有动作
        auto stop_res = safe_call([this]() { m_controller.stopAll(); });
        if (!stop_res) {
            return create_error_response("HARDWARE_ERROR", "Unknown action and Failed to stop: " + stop_res.error());
        }
        return create_error_response("INVALID_ACTION", "Unknown action: " + action + ", stopped all movements for safety");
    }

    std::string success_message = action + " executed"; 
    return exec(it->second, success_message.c_str());
}

std::string RequestHandler::create_success_response(const std::string& message) {
    // 构建成功响应的 JSON 对象
    json j;
    j["status"] = "ok";
    j["message"] = message;
    return j.dump();
}

std::string RequestHandler::create_error_response(const std::string& code, const std::string& message) {
    // 构建错误响应的 JSON 对象，包含错误代码和错误消息
    json j;
    j["status"] = "error";
    j["error_code"] = code;
    j["message"] = message;
    return j.dump();
}

tl::expected<void, std::string> RequestHandler::safe_call(const std::function<void()>& fn) {
    try {
        // 执行函数，如果成功则返回 void
        fn();
        return {};
    } catch (const std::exception& e) {
        // 捕获所有标准异常，转换为错误返回（包含异常信息）
        return tl::unexpected(std::string("Exception caught: ") + e.what());
    }
}

}// namespace fpvcar::device_control


