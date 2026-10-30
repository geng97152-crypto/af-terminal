#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <mutex>
#include <thread>
#include <vector>
#include <string.h>
#include "signal_slot.h"
#include "publisher.h"  
#include "abstractModule.h"
#define PORT 8888

using namespace std;

class TcpServer : public BaseModule
{
public:
    TcpServer(ModuleParams params = {});
    ~TcpServer();

    void stop();

    void send_msg(const std::string& msg);

    // 发送服务器端接收到的消息
    Signal<std::string> server_signal;

private:
    int serverSocket;
    int clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    bool quit_flag;
    std::mutex mutex;


    std::thread* server_thread;
    std::vector<std::thread> client_threads;


    void server_thread_func();
    void client_handler(int clientSocket);

    void notify(std::string msg) override;
    void update(Publisher* publisher, string msg) override;
};

#endif