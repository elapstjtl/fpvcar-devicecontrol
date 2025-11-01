#pragma once
#include <string>
#include <functional>
#include <atomic>
#include <tl/expected.hpp>

namespace fpvcar::device_control {
    using IpcCallback = std::function<std::string(const std::string&)>;

    class IpcServer {
    public:
        /**
         * @brief 构造 IPC 服务器实例
         * @param socket_path Unix 域套接字文件路径
         * @param callback 处理客户端请求的回调函数，接收 JSON 字符串并返回响应 JSON 字符串
         */
        IpcServer(const std::string& socket_path, IpcCallback callback);
        
        /**
         * @brief 析构函数，自动停止服务器并清理资源
         */
        ~IpcServer();

        /**
         * @brief 准备服务器：创建并绑定 Unix 域套接字，开始监听连接
         * @return 成功返回 void，失败返回错误信息字符串
         * @note 如果套接字文件已存在，会先删除它再创建新的
         */
        tl::expected<void, std::string> prepare();
        
        /**
         * @brief 运行服务器主循环，阻塞调用直到 stop() 被调用
         * @note 必须在调用 prepare() 成功后才能调用此函数
         * @note 每个客户端连接处理完一个请求后即关闭连接
         */
        void run();
        
        /**
         * @brief 停止服务器，中断 accept 循环并清理资源
         * @note 线程安全，可被多个线程调用
         */
        void stop();

    private:
        std::string m_socket_path;
        IpcCallback m_callback;
        int m_listen_fd;
        std::atomic<bool> m_running;
        bool m_prepared{false};
    };
}


