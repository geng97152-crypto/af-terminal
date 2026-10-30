# RV1106 摄像头终端

本仓库运行在 Luckfox RV1106 开发板上：采集摄像头视频、提供两路 H.264 RTSP、在本地 LCD 显示，并使用 YOLOv5 的 person 类驱动双轴舵机自动追踪。RK3588 Qt 客户端是另一个项目，通过网络拉取本终端的视频；不由本仓库编译。

## 当前数据流

| 输入 | 处理 | 输出 |
| --- | --- | --- |
| VI 通道 0，2304×1296 | VENC 通道 0，H.264；主码流 OSD | RTSP `/live/0` |
| VI 通道 1，720×480 | VENC 通道 1，H.264 | RTSP `/live/1` |
| VI 通道 1，720×480 | 中央裁剪 360×480，缩放 240×320，绘制检测框 | 本地 LCD `/dev/fb0` |
| VI 通道 2，640×640 | RKNN YOLOv5 推理、person 追踪 | 检测框与 PWM 控制指令 |

三个 VI 通道共用同一摄像头和 VI 管道。RTSP 代码虽然创建 `/live/2` 会话，但当前没有 VENC 2 提供编码帧，因此只有 `/live/0` 和 `/live/1` 是实际推流。LCD 使用单独的裁剪和画框路径，与副码流不一定完全相同。

## 与 RK3588 Qt 客户端

```text
RV1106：摄像头 → VI → VENC → RTSP /live/0、/live/1
                                    │ 局域网
                                    ▼
RK3588：Qt 应用 → 拉流 → 解码 → 显示及客户端功能
RV1106：VI 2 → YOLO person → PWM11/PWM10 → 本地云台
```

RK3588 显示的仍是 RV1106 摄像头拍摄的画面。多路显示、录像等客户端功能由 RK3588 项目实现。**以下按 TCP 和 ONVIF 服务均已开启来说明目标系统**：RK3588 可经 RTSP 获取视频，经 TCP 向 RV1106 发送云台命令；ONVIF 为兼容的设备发现与服务入口。RTSP 本身只传视频，不承担云台控制。当前源码的 `main.cc` 中这两个开关仍为 0；启用开关也不等于 RK3588 客户端已对接或 ONVIF PTZ 已连到本地 Pantilt，仍需逐项联调。

### 假设开启时的控制链路

- **TCP 控制**：终端监听 TCP 8888（见 `code/tcp/server.h`）。RK3588 客户端若发送上述 `PTZ:...` 字符串，终端的 `TcpServer::client_handler()` 接收后通知观察者，`Pantilt::update()` 调用相应方向函数。客户端是否使用这些字符串，需要在客户端仓库核对。
- **ONVIF**：终端的发现服务使用 UDP 3702，设备服务使用 8080（见 `code/onvif/server/macro.h`）。目前未在 ONVIF 源码中确认将 ONVIF PTZ 操作接到本地 `Pantilt` 的调用，因此不能把“打开 ONVIF 开关”直接等同于“ONVIF 云台可控”。
- **视频**：无论 TCP/ONVIF 是否打开，RK3588 的画面仍来自 RV1106 的 RTSP `/live/0` 或 `/live/1`。

## 程序流程

1. `main.cc` 执行 `RkLunch-stop.sh` 停止可能占用摄像头的原程序，读取运行目录的 `rkipc.ini`。
2. 创建 PTZ、Display、VideoRTSP、VideoLCD、VideoYOLO 模块；视频和显示模块启动各自线程。
3. 建立三条信号连接：`VideoLCD → Display` 传 LCD 帧；`VideoYOLO → VideoLCD` 传检测框；`VideoYOLO → Pantilt` 传舵机调整量。假设 TCP/ONVIF 开启，还会启动对应服务；TCP 收到 `PTZ:left`、`PTZ:right`、`PTZ:up`、`PTZ:down`、`PTZ:reset` 后，可通过观察者关系交给 Pantilt。
4. `VideoRTSP` 初始化 ISP、MPI、VI 和 RTSP，绑定 VI 0 与 VENC 0，并推送主码流。
5. `VideoLCD` 从 VI 1 取帧：一路裁剪并送 Display 队列显示；另一路编码后推送副码流。Display 转 RGB565 后写入 framebuffer。
6. `VideoYOLO` 从 VI 2 取帧，进行颜色转换、letterbox、RKNN 推理和后处理；把检测框送往 LCD 及主码流 OSD。类别 ID 为 0 的 person 进入追踪器。
7. 追踪器比较 person 检测框与 LCD 可见区域，满足偏差条件时输出水平和俯仰增量；Pantilt 换算 PWM 脉宽，分别驱动 PWM11 和 PWM10。当前帧没有有效 person 时不发新的自动追踪指令。
8. 收到 Ctrl+C 或 SIGTERM 后，先停止检测，再停止 PTZ、LCD 视频和显示，最后释放 VideoRTSP 持有的共享资源。

## 目录

| 路径 | 作用 |
| --- | --- |
| `main.cc` | 程序入口、模块创建、信号连接、退出 |
| `code/video/` | 采集、编码、RTSP、YOLO、后处理和追踪 |
| `code/display/` | LCD 队列及 framebuffer 输出 |
| `code/ptz/` | 双轴舵机、PWM sysfs |
| `code/osd/` | 主码流 OSD |
| `code/param/`、`code/rkipc.ini` | 参数读取及运行配置 |
| `code/factory/`、`code/observer/`、`code/signal_slot.h` | 模块创建和通信 |
| `code/tcp/`、`code/onvif/` | TCP 命令服务与 ONVIF 服务；本文架构按两者开启讨论，当前入口开关仍关闭 |
| `include/`、`lib/`、`3rdparty/` | 头文件、预编译库及第三方配置 |
| `model/` | RKNN 模型和 COCO 类别名称 |
| `build/`、`install/` | 编译中间文件和部署产物 |

## 编译与部署

需要匹配板端系统的 Luckfox Pico SDK。在 Linux/WSL 项目根目录执行：

```sh
cd /home/geng/tool/luckfox-pico/shili/af-terminal
export LUCKFOX_SDK_PATH=/home/geng/tool/luckfox-pico
chmod +x build.sh
./build.sh
```

`build.sh` 重建 `build/`，执行 `cmake ..` 和 `make install`；`./build.sh clean` 只删除 `build/` 和 `install/`。构建脚本不上传程序。

部署目录为 `install/rkipc_terminal_demo/`。程序运行时需要 `rkipc_terminal`、`rkipc.ini`、`model/yolov5.rknn`、`model/coco_80_labels_list.txt` 及所需动态库。CMake 安装规则复制程序、配置、模型和部分第三方库；其余库应根据构建结果与目标板环境核对。

在开发板上进入部署目录，再运行程序，因为配置和模型使用相对路径：

```sh
cd /path/to/rkipc_terminal_demo
chmod +x rkipc_terminal
./rkipc_terminal
```

在另一台设备打开 `rtsp://<板子IP>/live/0` 或 `rtsp://<板子IP>/live/1`。日志默认输出到终端；可用 `./rkipc_terminal 2>&1 | tee /tmp/rkipc.log` 保存。本地 LCD 需要 `/dev/fb0` 可用；云台需要正确的 PWM 引脚复用、供电和接线。

## 自动追踪配置

当前使用 YOLOv5 的 **person 类**，不是专门的人脸模型。参数在 `code/rkipc.ini` 的 `[ptz]`：默认置信度门槛 20%、控制间隔 200 ms、每次最大步长 5°、水平和垂直死区 8%。有有效检测帧即可产生追踪指令，没有目标时不会主动搜索。

PWM11 为水平轴，PWM10 为俯仰轴。当前两轴脉宽为 1000–2000 µs；水平软件范围 −90°～90°，俯仰软件范围 −60°～90°，`pan_direction=1`。启动时程序先配置 PWM，收到首次追踪或手动 PTZ 命令后才使能输出。软件 0° 对应 1500 µs；镜头实际朝向由舵机摇臂安装决定，软件限位也不能越过机械行程和排线限制。

修改 C/C++ 源码后需要重新编译并上传新程序；修改 `code/rkipc.ini` 后需要部署新配置。只修改板端 INI 不需要重新编译。
