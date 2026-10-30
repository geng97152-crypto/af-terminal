#include "server.h"
#include "log.h"

TcpServer::TcpServer(ModuleParams params): BaseModule(params) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        LOG_ERROR("Error opening socket\n");
        exit(1); 
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Error on binding\n");
        exit(1);
    }

    quit_flag = false;
    server_thread = new std::thread(&TcpServer::server_thread_func, this);
}
TcpServer::~TcpServer() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        quit_flag = true;
    }

    close(serverSocket);

    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    LOG_DEBUG("Server stopped\n");
}
// 服务器等待客户端连接线程
void TcpServer::server_thread_func()
{
    listen(serverSocket, 10);
    LOG_DEBUG("Server started\n");

    socklen_t clientLen = sizeof(clientAddr);

    while (!quit_flag) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            LOG_ERROR("Error on accept\n");
            continue;
        }

        LOG_DEBUG("Client connected\n");
        client_threads.emplace_back(&TcpServer::client_handler, this, clientSocket);
    }
}

// 客户端处理线程
void TcpServer::client_handler(int clientSocket) {
    char buffer[256];
    while (!quit_flag) 
    {
        memset(buffer, 0, 256);
        int n = read(clientSocket, buffer, 255);
        if (n < 0) {
            LOG_INFO("Client disconnected\n");
            break;
        } else if (n == 0) {
            LOG_DEBUG("Client disconnected\n");
            break;
        }

        LOG_INFO("Received message from client:%s\n", buffer);

        // 发送消息到所有观察者
        std::lock_guard<std::mutex> lock(mutex);
        notify(std::string(buffer));

        // 发送服务器接收到客户端的信号
    }
    close(clientSocket);
}

void TcpServer::send_msg(const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& thread : client_threads) {
            if (thread.joinable()) {
                write(clientSocket, msg.c_str(), msg.length());
            }
        }
    }
}

void TcpServer::notify(std::string msg)
{
    cout << "TcpServer observer num:" << m_observers.size() <<endl;

    for(const auto &observer : m_observers)
    {
        // 观察者更新数据
        observer->update(this, msg);
    }
}


void TcpServer::update(Publisher* publisher, string msg)
{

}