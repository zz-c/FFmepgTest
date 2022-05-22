#include <iostream>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
}

void Test::test01() {
	std::cout << "test01..." << std::endl;
	const char* filePath = "../bin/test.mp4";//���Ŀ¼..\bin��
	//��ʼ����װ��
	av_register_all();
	//��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	avformat_network_init();
	//��������
	AVDictionary* dictionaryOpts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//������ʱʱ��
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);

	//���װ������
	AVFormatContext* inAVFormatContext = NULL;
	int re = avformat_open_input(
		&inAVFormatContext,
		filePath,
		0,  // 0��ʾ�Զ�ѡ������
		&dictionaryOpts //�������ã�����rtsp����ʱʱ��
	);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		std::cout << "open " << filePath << " failed! :" << buf << std::endl;
		return;
	}
	std::cout << "open " << filePath << " success! " << std::endl;
}

