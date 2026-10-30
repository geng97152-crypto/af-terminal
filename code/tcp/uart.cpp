#include "uart.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

Uart::Uart(int port) {
    port_num = port;

     // 打开串口3
    sprintf(port_path,"/dev/ttyS%d", port_num);
    serial_fd = open(port_path, O_RDWR);
    if (serial_fd == -1) {
        LOG_ERROR("Failed to open serical port\n");
        return;
    }

    // 配置串口
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    // tcgetattr 函数获取当前串口的属性
    if (tcgetattr(serial_fd, &tty) != 0) {
        LOG_ERROR("Failed to get serial port attributes\n");
        return;
    }

    // 设置串口属性
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag &= ~PARENB;     // 禁用奇偶校验
    tty.c_cflag &= ~CSTOPB;     // 清除 CSTOPB 标志 使用一个停止位
    tty.c_cflag &= ~CSIZE;      // 清除数据位
    tty.c_cflag |= CS8;         // 数据位为8位
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        LOG_ERROR("Failed to set serial port attributes\n");
        return;
    }

    receive_thread = new std::thread(&Uart::receive_thread_func, this);

    quit_flag = false;

    LOG_DEBUG("********************************Usart%d Serial port opened successfully\n",port_num);
}
Uart::~Uart() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        quit_flag = true;    
    }
    close(serial_fd);
}

// receive STM32 data
void Uart::receive_thread_func()
{
    LOG_INFO("********************************Usart%d receive thread started\n",port_num);
    while (!quit_flag) {
        int bytes_read = read(serial_fd, recv_buf, sizeof(recv_buf));
        // 过滤回车
        if (bytes_read > 1) {
            uart_signal.emit(recv_buf);
            memset(recv_buf, 0, sizeof(recv_buf));
        }
    }    
}

// RV1106 send to STM32
 void Uart::sendData(const std::string& data)
 {
    LOG_DEBUG("********************************Usart%d sendData: %s\n",port_num, data.c_str());
    int bytes_written = write(serial_fd, data.c_str(), data.size());
    if (bytes_written == -1) {
        LOG_ERROR("Failed to send data to serial port\n");
        return ;
    }
    LOG_DEBUG("Sent %d bytes: %s\n", bytes_written, data.c_str());
 }