#include "fpvcar_device_control/request_handler.hpp"
#include <nlohmann/json.hpp>
#include <functional>

using nlohmann::json;

namespace fpvcar::device_control {

RequestHandler::RequestHandler(DesiredStateManager& desired_state_manager)
    :   m_desired_state_manager(desired_state_manager){}

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

    if (action == "moveForward") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_FORWARD);
    } else if (action == "moveBackward") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_BACKWARD);
    } else if (action == "turnLeft") {
        m_desired_state_manager.set_desired_state(DesiredState::TURNING_LEFT);
    } else if (action == "turnRight") {
        m_desired_state_manager.set_desired_state(DesiredState::TURNING_RIGHT);
    } else if (action == "moveForwardAndTurnLeft") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_FORWARD_AND_TURN_LEFT);
    } else if (action == "moveForwardAndTurnRight") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_FORWARD_AND_TURN_RIGHT);
    } else if (action == "moveBackwardAndTurnLeft") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_BACKWARD_AND_TURN_LEFT);
    } else if (action == "moveBackwardAndTurnRight") {
        m_desired_state_manager.set_desired_state(DesiredState::MOVING_BACKWARD_AND_TURN_RIGHT);
    } else if (action == "stopAll") {
        m_desired_state_manager.set_desired_state(DesiredState::STOPPING);
    } else {
        m_desired_state_manager.set_desired_state(DesiredState::STOPPING);
        std::cerr << "Unknown action: " + action << std::endl;
        return create_error_response("INVALID_ACTION", "Unknown action: " + action);
    }

    std::string success_message = action + " executed"; 
    return create_success_response(success_message);
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

}// namespace fpvcar::device_control


