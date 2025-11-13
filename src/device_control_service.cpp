#include "fpvcar_device_control/device_control_service.hpp"
#include <iostream>
#include <memory>
#include <exception>

namespace fpvcar::device_control {

DeviceControlService::DeviceControlService(const config::AppConfig& config)
    : m_config(config),
      // 初始化 GPIO 控制器，传入 GPIO 芯片名称、引脚配置和消费者名称
        m_controller(
            fpvcar::config::GPIO_CHIP_NAME,
            m_config.pins,
            fpvcar::config::GPIO_CONSUMER_NAME
        ),
        m_desired_state_manager(),
        m_control_loop(m_desired_state_manager, m_controller),
        // 初始化请求处理器，传入期望状态管理器引用
        m_handler(m_desired_state_manager),
        // 初始化 IPC 服务器，使用 lambda 捕获 this 并将请求转发给处理器
        // 当接收到新指令时，处理请求并喂看门狗
        m_server(
            m_config.ipc_socket_path,
            [this](const std::string& req) {
                std::string response = m_handler.handle_request(req);
                // 只有在成功处理请求后才喂看门狗（即使请求格式错误，也算收到了指令）
                m_control_loop.feed_watchdog();
                return response;
            }
        )
{
    std::cout << "DeviceControlService initialized." << std::endl;
}

DeviceControlService::~DeviceControlService() {
    stop();
}

tl::expected<std::unique_ptr<DeviceControlService>, std::string> DeviceControlService::create(const config::AppConfig& config) {
    try {
        // 使用 new 创建对象，因为 unique_ptr 需要在构造后设置
        auto ptr = std::unique_ptr<DeviceControlService>(new DeviceControlService(config));
        return ptr;
    } catch (const std::exception& e) {
        // 捕获构造函数中可能抛出的异常（如 GPIO 初始化失败），转换为错误返回
        return tl::unexpected(std::string("Failed to initialize service: ") + e.what());
    }
}

tl::expected<void, std::string> DeviceControlService::start() {
    // 先启动控制循环
    m_control_loop.start();
    // 再启动 IPC 服务器
    // 准备 IPC 服务器：创建并绑定套接字
    auto prep = m_server.prepare();
    if (!prep) return tl::unexpected(prep.error());
    
    // 在独立线程中运行服务器，避免阻塞调用者
    m_server_thread = std::thread([this]() { m_server.run(); });
    return {};
}

void DeviceControlService::stop() {
    // 关闭控制循环
    m_control_loop.stop();
    // 停止服务器，这会中断 accept 循环
    m_server.stop();
    
    // 等待服务器线程结束，确保资源完全清理
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
}

} // namespace fpvcar::device_control


