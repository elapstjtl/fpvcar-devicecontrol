#pragma once
#include <string>
#include <tl/expected.hpp>
#include <functional>
#include "fpvcar-motor/fpvcar_controller.hpp"

namespace fpvcar::device_control {
    class RequestHandler {
    public:
        /**
         * @brief 构造函数，初始化请求处理器
         * @param car 小车控制器引用，用于执行具体的设备控制操作
         */
        explicit RequestHandler(fpvcar::control::FpvCarController& car);
        
        /**
         * @brief 处理来自客户端的 JSON 请求
         * @param json_request JSON 格式的请求字符串，必须包含 "action" 字段
         * @return JSON 格式的响应字符串，包含 "status" 字段（"ok" 或 "error"）
         * @note 支持的 action 包括: moveForward, moveBackward, turnLeft, turnRight, stopAll
         * @note 如果 JSON 解析失败或 action 无效，返回错误响应
         * @note 所有硬件操作都会被异常捕获，返回错误响应而不是抛出异常
         */
        std::string handle_request(const std::string& json_request);

    private:
        fpvcar::control::FpvCarController& m_controller;

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
        
        /**
         * @brief 安全调用函数，捕获所有异常并转换为错误返回
         * @param fn 要执行的无参数函数对象
         * @return 成功返回 void，失败返回异常信息字符串
         * @note 用于包装硬件操作，防止异常向上传播
         */
        tl::expected<void, std::string> safe_call(const std::function<void()>& fn);
    };
}


