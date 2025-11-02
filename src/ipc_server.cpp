#include "fpvcar_device_control/ipc_server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

namespace fpvcar::device_control {

IpcServer::IpcServer(const std::string& socket_path, IpcCallback callback)
    : m_socket_path(socket_path), m_callback(std::move(callback)), m_listen_fd(-1), m_running(false) {}

IpcServer::~IpcServer() {
    stop();
}

tl::expected<void, std::string> IpcServer::prepare() {
    // 删除已存在的套接字文件（如果存在）
    ::unlink(m_socket_path.c_str());

    // 创建 Unix 域流式套接字（AF_UNIX:本机IPC通信  SOCK_STREAM：可靠的TCP）
    m_listen_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_listen_fd < 0) {
        return tl::unexpected(std::string("Failed to create socket: ") + std::strerror(errno));
    }

    // 初始化套接字地址结构
    sockaddr_un addr{};
    std::memset(&addr, 0, sizeof(addr)); // 将addr内存清零
    addr.sun_family = AF_UNIX;
    // 将路径复制到地址结构，注意 sun_path 长度限制
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", m_socket_path.c_str());

    // 绑定套接字到指定路径
    if (::bind(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        int err = errno;
        ::close(m_listen_fd);
        m_listen_fd = -1;
        return tl::unexpected(std::string("Failed to bind socket: ") + std::strerror(err));
    }

    // 开始监听连接，队列长度为 8
    if (::listen(m_listen_fd, 8) < 0) {
        int err = errno;
        ::close(m_listen_fd);
        m_listen_fd = -1;
        ::unlink(m_socket_path.c_str());
        return tl::unexpected(std::string("Failed to listen on socket: ") + std::strerror(err));
    }

    // 标记服务器已准备就绪并开始运行
    m_running.store(true);
    m_prepared = true;
    std::cout << "IPC server listening on " << m_socket_path << std::endl;
    return {};
}

void IpcServer::run() {
    // 检查服务器是否已准备就绪
    if (!m_prepared) return;

    // 主循环：持续接受客户端连接
    while (m_running.load()) {
        // 接受新的客户端连接（阻塞调用）
        int client_fd = ::accept(m_listen_fd, nullptr, nullptr);
        if (client_fd < 0) {
            // 如果服务器已停止，退出循环
            if (!m_running.load()) break; // 如果是stop() 触发的关闭，则退出循环
            // 如果是被信号中断的错误，则继续下一次 accept
            continue;
        }
        // 读取整个请求（每次连接一个请求）
        std::vector<char> buffer(4096);
        std::string request;
        ssize_t n = 0;
        while ((n = ::read(client_fd, buffer.data(), buffer.size())) > 0) {
            request.append(buffer.data(), static_cast<size_t>(n));
            // 如果读取的字节数小于缓冲区大小，说明数据已读完
            if (n < static_cast<ssize_t>(buffer.size())) {
                // 简单策略：认为客户端已发送完
                break;
            }
        }

        // 调用回调函数处理请求，捕获所有异常
        std::string response;
        try {
            response = m_callback ? m_callback(request) : std::string("{\"status\":\"error\",\"error_code\":\"NO_HANDLER\",\"message\":\"No handler set\"}");
        } catch (const std::exception& e) {
            // 如果回调函数抛出异常，返回服务器错误响应
            response = std::string("{\"status\":\"error\",\"error_code\":\"SERVER_ERROR\",\"message\":\"") + e.what() + "\"}";
        }

        // 写回响应：需要处理 write 可能的分片写入情况
        const char* data = response.data();
        size_t left = response.size();
        while (left > 0) {
            ssize_t wrote = ::write(client_fd, data, left);
            if (wrote <= 0) break; // 写入失败或连接关闭，退出循环
            data += wrote;
            left -= static_cast<size_t>(wrote);
        }

        // 关闭客户端连接（双向关闭，确保数据发送完成）
        ::shutdown(client_fd, SHUT_RDWR);
        ::close(client_fd);
    }

    // 清理：关闭监听套接字并删除套接字文件
    if (m_listen_fd >= 0) {
        ::close(m_listen_fd);
        m_listen_fd = -1;
    }
    ::unlink(m_socket_path.c_str());
}

void IpcServer::stop() {
    // 原子地设置运行标志为 false，如果已经是 false 则直接返回（避免重复停止）
    if (!m_running.exchange(false)) return;
    
    if (m_listen_fd >= 0) {
        // 关闭监听套接字，这会中断正在阻塞的 accept 调用
        // 触发 accept 返回错误，从而退出 run() 中的循环
        ::shutdown(m_listen_fd, SHUT_RDWR);
        ::close(m_listen_fd);
        m_listen_fd = -1;
    }
    // 删除套接字文件，清理文件系统资源
    ::unlink(m_socket_path.c_str());
}

}


