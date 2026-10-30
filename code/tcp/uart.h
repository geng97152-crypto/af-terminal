#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>
#include <thread>
#include <mutex>
#include "signal_slot.h"
#include <string>

class Uart{
public:
    Uart(int port_num);
    ~Uart();

    char recv_buf[1024];

    void sendData(const std::string& data);

    // 发送串口数据
    Signal<std::string> uart_signal;

private:
    int port_num;
    int serial_fd;
    char port_path[15];
   

    bool quit_flag;
    std::mutex mutex;

    std::thread* receive_thread;
    void receive_thread_func();


};

#endif