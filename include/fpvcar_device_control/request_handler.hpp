#pragma once
#include <string>
#include <tl/expected.hpp>
#include <functional>
#include "fpvcar_device_control/control_loop.hpp"

namespace fpvcar::device_control {
    class RequestHandler {
    public:
        /**
            * @brief 构造函数，初始化请求处理器
            * @param desired_state_manager 期望状态管理器引用，用于更新期望状态
            * @note 请求处理器负责解析IPC请求并更新期望状态，不直接操作硬件。硬件操作由 control_loop 线程执行
        */
        explicit RequestHandler(DesiredStateManager& desired_state_manager);
        
        /**
         * @brief 处理来自客户端的 JSON 请求
         * @param json_request JSON 格式的请求字符串，必须包含 "action" 字段
         * @return JSON 格式的响应字符串，包含 "status" 字段（"ok" 或 "error"）
         * @note 支持的 action 包括: moveForward, moveBackward, turnLeft, turnRight, stopAll
         * @note 如果 JSON 解析失败或 action 无效，返回错误响应
         * @note 此方法只更新期望状态并立即返回ACK，不等待硬件执行。硬件操作由 control_loop 线程异步执行
         */
        std::string handle_request(const std::string& json_request);

    private:
        DesiredStateManager& m_desired_state_manager;
        /**
         * @brief 创建成功响应的 JSON 字符串
         * @param message 成功消息
         * @return JSON 格式字符串，包含 status="ok" 和 message 字段
         */
        std::string create_success_response(const std::string& message);
        
        /**
         * @brief 创建错误响应的 JSON 字符串
         * @param code 错误代码，如 "INVALID_JSON", "HARDWARE_ERROR" 等
         * @param message 错误描述信息
         * @return JSON 格式字符串，包含 status="error", error_code 和 message 字段
         */
        std::string create_error_response(const std::string& code, const std::string& message);
    };
}


