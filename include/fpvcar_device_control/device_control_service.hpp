#pragma once
#include "fpvcar_device_control/config.hpp"
#include "fpvcar_device_control/request_handler.hpp"
#include "fpvcar_device_control/ipc_server.hpp"
#include "fpvcar-motor/fpvcar_controller.hpp"
#include "fpvcar_device_control/control_loop.hpp"
#include <thread>
#include <memory>
#include <tl/expected.hpp>

namespace fpvcar::device_control {
    class DeviceControlService {
    public:
        /**
         * @brief 构造函数，初始化设备控制服务
         * @param config 应用配置，包含 GPIO 引脚配置和 IPC 套接字路径
         * @note 此构造函数会初始化电机控制器、请求处理器和 IPC 服务器
         */
        explicit DeviceControlService(const config::AppConfig& config);
        
        /**
         * @brief 析构函数，自动停止服务并清理资源
         */
        ~DeviceControlService();
        
        /**
         * @brief 工厂方法：创建并返回 DeviceControlService 的唯一指针
         * @param config 应用配置
         * @return 成功返回服务实例的唯一指针，失败返回错误信息字符串
         * @note 此方法会捕获初始化过程中的异常并转换为错误返回
         */
        static tl::expected<std::unique_ptr<DeviceControlService>, std::string> create(const config::AppConfig& config);
        
        /**
         * @brief 启动服务：准备 IPC 服务器并在后台线程中运行
         * @return 成功返回 void，失败返回错误信息字符串
         * @note 服务器将在独立线程中运行，此方法不会阻塞
         */
        tl::expected<void, std::string> start();
        
        /**
         * @brief 停止服务：停止 IPC 服务器并等待服务器线程结束
         * @note 线程安全，可被多次调用
         */
        void stop();

    private:
        config::AppConfig m_config; // 应用配置
        DesiredStateManager m_desired_state_manager; // 期望状态管理器
        fpvcar::control::FpvCarController m_controller; // 控制器
        ControlLoop m_control_loop; // 控制循环
        RequestHandler m_handler; // 请求处理器
        IpcServer m_server; // IPC 服务器
        std::thread m_server_thread; // 服务器线程
    };
}


