#include "fpvcar_device_control/ipc_server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <vector>

namespace fpvcar::device_control {

namespace {
    /**
     * @brief 从套接字读取指定长度的数据
     * @param fd 文件描述符
     * @param buffer 缓冲区
     * @param size 要读取的字节数
     * @return 成功返回读取的字节数，失败返回-1，连接关闭返回0
     */
    ssize_t read_exact(int fd, void* buffer, size_t size) {
        char* ptr = static_cast<char*>(buffer);
        size_t left = size;
        while (left > 0) {
            ssize_t n = ::read(fd, ptr, left);
            if (n < 0) {
                if (errno == EINTR) continue; // 被信号中断，重试
                return -1; // 读取错误
            }
            if (n == 0) return 0; // 连接关闭
            ptr += n;
            left -= static_cast<size_t>(n);
        }
        return static_cast<ssize_t>(size);
    }

    /**
     * @brief 向套接字写入指定长度的数据
     * @param fd 文件描述符
     * @param buffer 缓冲区
     * @param size 要写入的字节数
     * @return 成功返回写入的字节数，失败返回-1
     */
    ssize_t write_exact(int fd, const void* buffer, size_t size) {
        const char* ptr = static_cast<const char*>(buffer);
        size_t left = size;
        while (left > 0) {
            ssize_t n = ::write(fd, ptr, left);
            if (n < 0) {
                if (errno == EINTR) continue; // 被信号中断，重试
                return -1; // 写入错误
            }
            ptr += n;
            left -= static_cast<size_t>(n);
        }
        return static_cast<ssize_t>(size);
    }

    /**
     * @brief 读取一条带长度前缀的消息
     * @param fd 文件描述符
     * @return 成功返回消息内容，失败返回空字符串
     */
    std::string read_message(int fd) {
        // 先读取4字节长度前缀（网络字节序）
        uint32_t length_net;
        ssize_t n = read_exact(fd, &length_net, sizeof(length_net));
        if (n != sizeof(length_net)) {
            return ""; // 读取失败或连接关闭
        }
        
        // 转换为主机字节序
        uint32_t length = ntohl(length_net);
        
        // 检查长度是否合理（防止恶意请求）
        if (length > 1024 * 1024) { // 最大1MB
            return ""; // 长度过大，拒绝
        }
        
        // 读取消息内容
        std::vector<char> buffer(length);
        n = read_exact(fd, buffer.data(), length);
        if (n != static_cast<ssize_t>(length)) {
            return ""; // 读取失败或连接关闭
        }
        
        return std::string(buffer.data(), length);
    }

    /**
     * @brief 写入一条带长度前缀的消息
     * @param fd 文件描述符
     * @param message 消息内容
     * @return 成功返回true，失败返回false
     */
    bool write_message(int fd, const std::string& message) {
        // 将长度转换为网络字节序
        uint32_t length = static_cast<uint32_t>(message.size());
        uint32_t length_net = htonl(length);
        
        // 先写入长度前缀
        if (write_exact(fd, &length_net, sizeof(length_net)) != sizeof(length_net)) {
            return false;
        }
        
        // 再写入消息内容
        if (write_exact(fd, message.data(), message.size()) != static_cast<ssize_t>(message.size())) {
            return false;
        }
        
        return true;
    }
}

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

        // 在单个连接上循环处理多个请求（保持长连接）
        while (m_running.load()) {
            // 读取带长度前缀的请求消息
            std::string request = read_message(client_fd);
            if (request.empty()) {
                // 读取失败或连接关闭，退出内层循环
                break;
            }

            // 调用回调函数处理请求，捕获所有异常
            std::string response;
            try {
                response = m_callback ? m_callback(request) : std::string("{\"status\":\"error\",\"error_code\":\"NO_HANDLER\",\"message\":\"No handler set\"}");
            } catch (const std::exception& e) {
                // 如果回调函数抛出异常，返回服务器错误响应
                response = std::string("{\"status\":\"error\",\"error_code\":\"SERVER_ERROR\",\"message\":\"") + e.what() + "\"}";
            }

            // 写入带长度前缀的响应消息
            if (!write_message(client_fd, response)) {
                // 写入失败，连接可能已关闭，退出内层循环
                break;
            }
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


