#include <iostream>
#include <thread>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
}

double r2d(AVRational r)
{
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}
void xSleep(int ms)
{
	//c++ 11
	std::chrono::milliseconds du(ms);
	std::this_thread::sleep_for(du);
}


/**
 * 初始化解封装使用avformat_open_input打开MP4文件，并设置延时等属性TestDemux
 */
void Test::test01() {
	std::cout << "test01..." << std::endl;
	const char* filePath = "../bin/test.mp4";//输出目录..\bin下
	//初始化封装库
	av_register_all();
	//初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	avformat_network_init();
	//参数设置
	AVDictionary* dictionaryOpts = NULL;
	//设置rtsp流已tcp协议打开
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//网络延时时间
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);

	//解封装上下文
	AVFormatContext* inAVFormatContext = NULL;
	int re = avformat_open_input(
		&inAVFormatContext,
		filePath,
		0,  // 0表示自动选择解封器
		&dictionaryOpts //参数设置，比如rtsp的延时时间
	);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		std::cout << "open " << filePath << " failed! :" << buf << std::endl;
		return;
	}
	std::cout << "open " << filePath << " success! " << std::endl;


	//获取流信息 
	re = avformat_find_stream_info(inAVFormatContext, 0);

	//总时长 毫秒
	int totalMs = inAVFormatContext->duration / (AV_TIME_BASE / 1000);
	std::cout << "total time ms = " << totalMs << std::endl;
	//打印视频流详细信息
	av_dump_format(inAVFormatContext, 0, filePath, 0);

	//音视频索引，读取时区分音视频
	int videoStream = 0;
	int audioStream = 1;

	//获取音视频流信息 （遍历，函数获取） videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	for (int i = 0; i < inAVFormatContext->nb_streams; i++)
	{
		AVStream* stream = inAVFormatContext->streams[i];
		std::cout << "codec_id = " << stream->codecpar->codec_id << std::endl;
		std::cout << "format = " << stream->codecpar->format << std::endl;

		//音频 AVMEDIA_TYPE_AUDIO
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			std::cout << i << "音频信息" << std::endl;
			std::cout << "sample_rate = " << stream->codecpar->sample_rate << std::endl;
			//AVSampleFormat;
			std::cout << "channels = " << stream->codecpar->channels << std::endl;
			//一帧数据？？ 单通道样本数 
			std::cout << "frame_size = " << stream->codecpar->frame_size << std::endl;
			//1024 * 2 * 2 = 4096  fps = sample_rate/frame_size

		}
		//视频 AVMEDIA_TYPE_VIDEO
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			std::cout << i << "视频信息" << std::endl;
			std::cout << "width=" << stream->codecpar->width << std::endl;
			std::cout << "height=" << stream->codecpar->height << std::endl;
			//帧率 fps 分数转换
			std::cout << "video fps = " << r2d(stream->avg_frame_rate) << std::endl;
		}
	}

	//获取视频流
	videoStream = av_find_best_stream(inAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//malloc AVPacket并初始化
	AVPacket* pkt = av_packet_alloc();
	for (;;)
	{
		int re = av_read_frame(inAVFormatContext, pkt);
		if (re != 0)
		{
			//循环播放
			std::cout << "==============================end==============================" << std::endl;
			int ms = 3000; //三秒位置 根据时间基数（分数）转换
			long long pos = (double)ms / (double)1000 * r2d(inAVFormatContext->streams[pkt->stream_index]->time_base);
			av_seek_frame(inAVFormatContext, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		std::cout << "pkt->size = " << pkt->size << std::endl;
		//显示的时间
		std::cout << "pkt->pts = " << pkt->pts << std::endl;
		//转换为毫秒，方便做同步
		std::cout << "pkt->pts ms = " << pkt->pts * (r2d(inAVFormatContext->streams[pkt->stream_index]->time_base) * 1000) << std::endl;
		//解码时间
		std::cout << "pkt->dts = " << pkt->dts << std::endl;
		if (pkt->stream_index == videoStream)
		{
			std::cout << "图像" << std::endl;
		}
		if (pkt->stream_index == audioStream)
		{
			std::cout << "音频" << std::endl;
		}
		//释放，引用计数-1 为0释放空间
		av_packet_unref(pkt);
		//XSleep(500);
	}

	av_packet_free(&pkt);

	if (inAVFormatContext)
	{
		//释放封装上下文，并且把inAVFormatContext置0
		avformat_close_input(&inAVFormatContext);
	}
}

