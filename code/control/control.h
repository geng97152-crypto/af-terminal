#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include "signal_slot.h"

#include "log.h"
#include "param.h"

// 控制信号结构体
struct ControlSignal {
    int module_id;      // 模块ID
    int opcode;         // 操作码
    int param;          // 参数
};

enum ModuleID {
    ID_UNKNOWN = 0,
    ID_PANTILT,         // 摄像头控制模块
    ID_DISPLAY,         // 显示模块
    ID_VIDEO            // 视频模块
};

enum PantiltOpcode {
    OP_PANTILT_UP = 0,      // 上
    OP_PANTILT_DOWN,        // 下
    OP_PANTILT_LEFT,        // 左
    OP_PANTILT_RIGHT,       // 右
    OP_PANTILT_RESET,       // 重置
    OP_PANTILT_MAX          // 摄像头控制操作码最大值
};

enum DisplayOpcode {
    OP_DISPLAY_PAUSE = 0,   // 暂停
    OP_DISPLAY_RESUME,      // 恢复
    OP_DISPLAY_MAX          // 显示操作码最大值
};

enum VideoOpcode {
    OP_VIDEO_PIPE0_START = 0,   // 开启管道0
    OP_VIDEO_PIPE0_STOP,        // 关闭管道0
    OP_VIDEO_PIPE0_RESTART,     // 重启管道0
    OP_VIDEO_PIPE1_START,       // 开启管道1
    OP_VIDEO_PIPE1_STOP,        // 关闭管道1
    OP_VIDEO_PIPE1_RESTART,     // 重启管道1
    OP_VIDEO_MAX                // 视频操作码最大值
};

class Control {
public:
    Control();
    ~Control();

    // 控制信号接收槽函数
    Signal<ControlSignal> signal_control_received;          

    // 接收网络数据槽函数
    void onNetworkReceived(const std::string& data);

    // 注册控制函数
    void registerControlFunction(int module_id, int opcode, std::function<void(int)> func);

private:
    // 控制信号解析函数
    ControlSignal parseSignal(const std::string& data);

    // 控制函数（采用函数指针数组）进行回调
    void dispatchSignal(const ControlSignal& signal);

    // 映射各个模块的控制函数，使用哈希表存储
    std::unordered_map<int, std::unordered_map<int, std::function<void(int)>>> control_functions;
};

