#include <iostream>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
}

void Test::test01() {
	std::cout << "test01..." << std::endl;

	//const char* path = "v1080.mp4";
	////��ʼ����װ��
	//av_register_all();

	////��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	//avformat_network_init();

	////��������
	//AVDictionary* opts = NULL;
	////����rtsp����tcpЭ���
	//av_dict_set(&opts, "rtsp_transport", "tcp", 0);

	////������ʱʱ��
	//av_dict_set(&opts, "max_delay", "500", 0);


	////���װ������
	//AVFormatContext* ic = NULL;
	//int re = avformat_open_input(
	//	&ic,
	//	path,
	//	0,  // 0��ʾ�Զ�ѡ������
	//	&opts //�������ã�����rtsp����ʱʱ��
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

