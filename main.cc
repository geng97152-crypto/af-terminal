#include "videoRTSP.h"	// videoRTSP
#include "videoYOLO.h"	// videoYOLO
#include "videoLCD.h"	// videoLCD
#include "display.h"	// lcd
#include "video_factory.h"
#include "displayFactory.h"
#include "ptzFactory.h"
#include "pantilt.h"	// pantilt
#include "log.h"		// log
#include "param.h"		// param
#include "control.h"
#include <unistd.h>
#include "onvif_server.h"
#include "server.h"
#include "uart.h"
#include <atomic>


#define ONVIF_SERVER_ENABLE 	0//ONVIF 服务模块。用于让摄像机被支持 ONVIF 的软件发现和控制

#define PTZ_ENABLE 				1//云台控制模块
#define DISPLAY_ENABLE 			1//LCD 设备显示模块，负责打开 /dev/fb0，把图像写入 LCD framebuffer。

#define VIDEORTSP_ENABLE 		1//RTSP 视频流模块
#define VIDEODISPLAY_ENABLE 	1//LCD 图像采集和处理模块，是从摄像头视频通道获取帧
#define VIDEOYOLO_ENABLE 		1//YOLO 目标检测模块

#define UART_ENABLE 			0//串口通信模块
#define TCP_SERVER_ENABLE 		0//普通 TCP 控制服务器

char ini_path[] = "rkipc.ini";
int rkipc_log_level = LOG_LEVEL_DEBUG;

std::atomic<bool> g_quit(false);	//全局标志位

static void sigterm_handler(int sig) {
	if (sig == SIGINT || sig == SIGTERM) {
        g_quit = true; // 设置标志位
		std::cout << "**************quit success sig:" << sig << std::endl;
    }
}

int main(int argc, char *argv[]) 
{
  	system("RkLunch-stop.sh");
	
	// 初始化基类工厂指针
	BaseFactory* factory = nullptr;

	// 初始化参数。OSD 等模块会读取该配置，必须在创建模块前完成。
    if (rk_param_init(ini_path) != 0) {
        LOG_ERROR("Failed to initialize parameters\n");
        return EXIT_FAILURE;
    }

	// Ctrl-c quit
	signal(SIGINT, sigterm_handler);
	signal(SIGTERM, sigterm_handler);

#if ONVIF_SERVER_ENABLE
	LOG_INFO("onvif Module init\n");
	onvif_server_init();
#endif

#if TCP_SERVER_ENABLE
	ModuleParams TcpParam;// 准备模块参数
	TcpParam.name = "TcpServer";// 给模块取名
	TcpServer* tcpServer = new TcpServer(TcpParam);
	LOG_INFO("tcpServer Module init\n");
#endif


#if PTZ_ENABLE
	// 俯仰旋转控制
	LOG_INFO("PTZ Module init\n");
	ModuleParams PTZParam;
	PTZParam.name = "PTZ";
	factory = new PtzFactory();//创建云台工厂
	BaseModule* ptz = factory->createModule(PTZParam);//工厂创建模块
#if TCP_SERVER_ENABLE
	ptz->subscribe(tcpServer);
#endif
#endif

#if DISPLAY_ENABLE
	LOG_INFO("display Module init\n");
	factory = new DisplayFactory();
	BaseModule* display = factory->createModule();
#endif

#if VIDEORTSP_ENABLE
	LOG_INFO("VideoRTSP Module init\n");
	factory = new VideoRTSPFactory();
	BaseModule* videoRTSP = factory->createModule();
#endif

#if VIDEODISPLAY_ENABLE
	LOG_INFO("VideoDisplay Module init\n");
	factory = new VideoDisplayFactory();
	BaseModule* videoDisplay = factory->createModule();
#endif

#if VIDEOYOLO_ENABLE
	LOG_INFO("VideoYOLO Module init\n");
	factory = new VideoYOLOFactory();
	BaseModule* videoYOLO = factory->createModule();
#endif

#if VIDEODISPLAY_ENABLE && DISPLAY_ENABLE
	LOG_INFO("videoDisplay connect to display\n");
	// 使用lambda表达式连接信号与槽
	dynamic_cast<VideoLCD*>(videoDisplay)->video_frame_signal.connect(
    [display](const cv::Mat& frame) {
        dynamic_cast<Display*>(display)->push_frame(frame);
    }
	);
	// 使用lambda表达式连接信号与槽
	// dynamic_cast<VideoLCD*>(videoDisplay)->video_frame_signal.connect(dynamic_cast<Display*>(display.get()), &Display::push_frame);
	// videoDisplay->video_frame_signal.connect(dynamic_cast<Display*>(display.get()), &Display::push_frame);
#endif

#if VIDEOYOLO_ENABLE && VIDEODISPLAY_ENABLE
	LOG_INFO("videoYOLO connect to videoDisplay\n");
	dynamic_cast<VideoYOLO*>(videoYOLO)->detection_signal.connect(
		[videoDisplay](const std::vector<LcdDetection>& detections) {
			dynamic_cast<VideoLCD*>(videoDisplay)->update_detections(detections);
		}
	);
#endif

#if VIDEOYOLO_ENABLE && PTZ_ENABLE
	LOG_INFO("videoYOLO connect to PTZ\n");
	dynamic_cast<VideoYOLO*>(videoYOLO)->signal_adjust_pantilt.connect(
		[ptz](int delta_pan, int delta_tilt) {
			dynamic_cast<Pantilt*>(ptz)->onAjustPantilt(delta_pan, delta_tilt);
		}
	);
#endif



#if UART_ENABLE && TCP_SERVER_ENABLE
	// 绑定tcp数据到串口发送
	uart->uart_signal.connect(tcpServer, &TcpServer::send_msg);	
	// 绑定串口数据发送到TCP客户端
	tcpServer->server_signal.connect(uart, &Uart::sendData);
#endif

  	while(!g_quit)
	{	
		std::this_thread::sleep_for(std::chrono::milliseconds(500));	// 休眠500ms
		// LOG_INFO("hello wordld\n");
	}

 	LOG_INFO("Program starting exited\n");

try {
	#if VIDEOYOLO_ENABLE
		delete videoYOLO;
		videoYOLO = nullptr;
	#endif

	// Stop the detection producer before destroying its PTZ callback target.
	#if PTZ_ENABLE
		delete ptz;
		ptz = nullptr;
	#endif

	#if VIDEODISPLAY_ENABLE
		delete videoDisplay;
		videoDisplay = nullptr;
	#endif

	#if DISPLAY_ENABLE
		delete display;
		display = nullptr;
	#endif

	// VideoRTSP 拥有共享的 ISP、MPI、VI 和 RTSP 资源，必须最后销毁。
	#if VIDEORTSP_ENABLE
		delete videoRTSP;
		videoRTSP = nullptr;
	#endif

	#if CONTROLLER_ENABLE
		delete controllor;
		controllor = nullptr;
	#endif

	#if UART_ENABLE
		delete uart;
		uart = nullptr;
	#endif

	#if TCP_SERVER_ENABLE
		delete tcpServer;
		tcpServer = nullptr;
	#endif

	if(factory) {
		delete factory;
		factory = nullptr;
	}

	} catch (std::exception &e) {
        LOG_ERROR("Exception: %s\n", e.what());
    }

	// delete tcpServer;
	rk_param_deinit();
    LOG_INFO("Program exited\n");
	return 0;
}
