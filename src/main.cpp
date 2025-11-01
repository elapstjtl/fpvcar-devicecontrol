#include "fpvcar_device_control/device_control_service.hpp"
#include "fpvcar_device_control/config.hpp"
#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>

// 全局关闭标志，用于优雅地处理信号中断
std::atomic<bool> g_shutdown_request{false};

/**
 * @brief 信号处理函数，设置关闭标志以触发优雅退出
 * @param signum 信号编号（未使用，但符合信号处理函数签名）
 */
void signal_handler(int) {
    g_shutdown_request = true;
}

/**
 * @brief 主函数：程序入口点
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 成功返回 0，失败返回 1
 * 
 * 程序流程：
 * 1. 注册信号处理器（SIGINT 和 SIGTERM）以支持优雅关闭
 * 2. 从配置文件加载应用配置
 * 3. 创建并初始化设备控制服务
 * 4. 启动服务（在后台线程中运行 IPC 服务器）
 * 5. 进入主循环，等待关闭信号
 * 6. 收到信号后优雅地停止服务并退出
 */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // 注册信号处理器：捕获 Ctrl+C (SIGINT) 和终止信号 (SIGTERM)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 从默认配置文件加载配置
    auto cfg_res = fpvcar::device_control::config::load_config("config/default_config.json");
    if (!cfg_res) {
        std::cerr << "FATAL ERROR: " << cfg_res.error() << std::endl;
        return 1;
    }

    // 创建设备控制服务实例
    auto svc_res = fpvcar::device_control::DeviceControlService::create(*cfg_res);
    if (!svc_res) {
        std::cerr << "FATAL ERROR: " << svc_res.error() << std::endl;
        return 1;
    }

    // 启动服务：准备 IPC 服务器并在后台线程中运行
    auto start_res = (*svc_res)->start();
    if (!start_res) {
        std::cerr << "FATAL ERROR: " << start_res.error() << std::endl;
        return 1;
    }
    std::cout << "fpvcar-devicecontrol service started. Press Ctrl+C to stop." << std::endl;

    // 主循环：等待关闭信号，每隔 100ms 检查一次标志
    while (!g_shutdown_request.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 收到关闭信号后，优雅地停止服务
    std::cout << "Shutting down service..." << std::endl;
    (*svc_res)->stop();
    std::cout << "Service stopped." << std::endl;

    return 0;
}


