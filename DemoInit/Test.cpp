#include <iostream>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
}

void Test::test01() {
	std::cout << "test01..." << std::endl;

	//const char* path = "v1080.mp4";
	////初始化封装库
	//av_register_all();

	////初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	//avformat_network_init();

	////参数设置
	//AVDictionary* opts = NULL;
	////设置rtsp流已tcp协议打开
	//av_dict_set(&opts, "rtsp_transport", "tcp", 0);

	////网络延时时间
	//av_dict_set(&opts, "max_delay", "500", 0);


	////解封装上下文
	//AVFormatContext* ic = NULL;
	//int re = avformat_open_input(
	//	&ic,
	//	path,
	//	0,  // 0表示自动选择解封器
	//	&opts //参数设置，比如rtsp的延时时间
	//);
	//if (re != 0)
	//{
	//	char buf[1024] = { 0 };
	//	av_strerror(re, buf, sizeof(buf) - 1);
	//	std::cout << "open " << path << " failed! :" << buf << std::endl;
	//	
	//}
	//std::cout << "open " << path << " success! " << std::endl;
}

