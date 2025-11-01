#include "fpvcar_device_control/request_handler.hpp"
#include <nlohmann/json.hpp>
#include <functional>

using nlohmann::json;

namespace fpvcar::device_control {

RequestHandler::RequestHandler(fpvcar::control::FpvCarController& car)
    : m_controller(car) {}

std::string RequestHandler::handle_request(const std::string& json_request) {
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

    // Lambda 辅助函数：安全执行控制器操作并统一错误处理
    auto exec = [&](auto&& call, const char* ok_msg) {
        auto res = safe_call(call);
        if (!res) return create_error_response("HARDWARE_ERROR", res.error());
        return create_success_response(ok_msg);
    };

    // 根据 action 类型执行相应的控制器操作
    if (action == "moveForward") {
        return exec([&]{ m_controller.moveForward(); }, "moveForward executed");
    } else if (action == "moveBackward") {
        return exec([&]{ m_controller.moveBackward(); }, "moveBackward executed");
    } else if (action == "turnLeft") {
        return exec([&]{ m_controller.turnLeft(); }, "turnLeft executed");
    } else if (action == "turnRight") {
        return exec([&]{ m_controller.turnRight(); }, "turnRight executed");
    } else if (action == "stopAll") {
        return exec([&]{ m_controller.stopAll(); }, "stopAll executed");
    }

    // 未知的 action，返回错误响应
    return create_error_response("INVALID_ACTION", std::string("Unknown action: ") + action);
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

}


