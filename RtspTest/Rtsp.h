#pragma once
extern "C" {
#include "libavutil/log.h"
	//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h" 

#include "libavutil/time.h"
}

/*
将帧保存到文件

*/
void saveFrame2Ppm(AVFrame* pFrame, int width, int height, int iFrame) {

	FILE* pFile;		//文件指针
	char szFilename[32];//文件名（字符串）
	int y;				//

	sprintf(szFilename, "rtspToPpm%04d.ppm", iFrame);	//生成文件名
	pFile = fopen(szFilename, "wb");			//打开文件，只写入
	if (pFile == NULL) {
		return;
	}

	//getch();

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);//在文档中加入，必须加入，不然PPM文件无法读取

	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
	}

	fclose(pFile);

}
/**
* 将AVFrame(YUV420格式)保存为JPEG格式的图片
*
* @param width YUV420的宽
* @param height YUV42的高
*
*/
int saveFrame2JPEG(AVFrame* pFrame, int width, int height, int iIndex) {
	// 输出文件路径

	char out_file[1000] = { 0 };
	sprintf(out_file, "rtspToJpg%04d.jpg", iIndex);
	printf("out_file:%s", out_file);
	// 分配AVFormatContext对象
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	// 设置输出文件格式
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	// 创建并初始化一个和该url相关的AVIOContext
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// 构建一个新stream
	//Using AVStream.codec to pass codec parameters to muxers is deprecated, use AVStream.codecpar instead |||AVCodecParametersstream->codecpar

	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	// 设置该stream的信息
	AVCodecContext* pCodecCtx = pAVStream->codec;

	pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// Begin Output some information
	av_dump_format(pFormatCtx, 0, out_file, 1);
	// End Output some information

	// 查找解码器
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		printf("Codec not found.");
		return -1;
	}
	// 设置pCodecCtx的解码器为pCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	// 给AVPacket分配足够大的空间
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	//
	int got_picture = 0;
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if (ret < 0) {
		printf("Encode Error.\n");
		return -1;
	}
	if (got_picture == 1) {
		//pkt.stream_index = pAVStream->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);

	printf("Encode Successful.\n");

	if (pAVStream) {
		avcodec_close(pAVStream->codec);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}

int testRtspToPicture()
{
	// Allocate an AVFormatContext
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	// open rtsp: Open an input stream and read the header. The codecs are not opened
	const char* url = "rtsp://admin:ruijie123@172.26.1.33";
	int ret = -1;
	ret = avformat_open_input(&pFormatCtx, url, nullptr, nullptr);
	if (ret != 0) {
		fprintf(stderr, "fail to open url: %s, return value: %d\n", url, ret);
		return -1;
	}

	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(pFormatCtx, nullptr);
	if (ret < 0) {
		fprintf(stderr, "fail to get stream information: %d\n", ret);
		return -1;
	}

	// audio/video stream index
	int video_stream_index = -1;
	int audio_stream_index = -1;
	fprintf(stdout, "Number of elements in AVFormatContext.streams: %d\n", pFormatCtx->nb_streams);
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		const AVStream* stream = pFormatCtx->streams[i];
		fprintf(stdout, "type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		}
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			fprintf(stdout, "audio sample format: %d\n", stream->codecpar->format);
		}
	}

	if (video_stream_index == -1) {
		fprintf(stderr, "no video stream\n");
		return -1;
	}
	if (audio_stream_index == -1) {
		fprintf(stderr, "no audio stream\n");
	}
	printf("文件格式：%s\n", pFormatCtx->iformat->name);
	printf("文件时长：%d秒\n", (pFormatCtx->duration) / 1000000);

	//只有知道视频的编码方式，才能够根据编码方式去找到解码器
	//获取视频流中的编解码上下文
	AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
	//根据编解码上下文中的编码id查找对应的解码
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "找不到解码器\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "解码器无法打开\n");
		return -1;
	}
	//输出视频信息
	printf("视频的pix_fmt：%d\n", pCodecCtx->pix_fmt);
	printf("视频的宽高：%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("视频解码器的名称：%s\n", pCodec->name);

	AVCodecContext* pCodecCtx2 = pFormatCtx->streams[audio_stream_index]->codec;
	AVCodec* pCodec2 = avcodec_find_decoder(pCodecCtx2->codec_id);
	//打开解码器
	if (avcodec_open2(pCodecCtx2, pCodec2, NULL) < 0)
	{
		printf("%s", "解码器2无法打开\n");
		return -1;
	}
	//输出音频信息
	printf("音频解码器2的名称：%s\n", pCodec2->name);
	printf("音频解码器2的channels：%d\n", pCodecCtx2->channels);
	printf("音频解码器2的sample_fmt：%d\n", pCodecCtx2->sample_fmt);
	printf("音频解码器2的frame_size：%d\n", pCodecCtx2->frame_size);
	printf("音频解码器2的channel_layout：%d\n", pCodecCtx2->channel_layout);


	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();


	//分配视频帧需要内存 (存放原始数据用)
	int numBytes;		//需要的内存大小
	uint8_t* buffer = NULL;
	//获取需要的内存大小
	/*
	1. av_image_fill_arrays 函数来关联 frame 和我们刚才分配的内存
	2. av_malloc 是一个 FFmpeg 的 malloc，
	主要是对 malloc 做了一些封装来保证地址对齐之类的事情，
	它会保证你的代码不发生内存泄漏、多次释放或其他 malloc 问题
	*/
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);//获取需要的内存大小
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//关联frame和刚才分配的内容
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);



	//初始化SWS上下文
	//SwrContext *swrCtx = swr_alloc();
	struct SwsContext* sws_ctx = NULL;
	sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	//sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);
	int frameFinished;
	int cnt = 0;

	while (1) {
		if (cnt > 10) break;
		ret = av_read_frame(pFormatCtx, packet);
		if (ret < 0) {
			fprintf(stderr, "error or end of file: %d\n", ret);
			continue;
		}
		if (packet->stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", packet->size);

			ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
			if (ret < 0) {
				printf("%s", "解码完成");
			}
			if (frameFinished) {
				if (cnt == 2) {
					//将视频帧原来的格式pCodecCtx->pix_fmt转换成RGB
					sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					saveFrame2Ppm(pFrameRGB, pCodecCtx->width, pCodecCtx->height, cnt);
					//saveFrame2JPEG(pFrame, pCodecCtx->width, pCodecCtx->height, cnt);
					frameFinished = 0;
				}
				cnt++;
			}



		}
		if (packet->stream_index == audio_stream_index) {
			fprintf(stdout, "audio stream, packet size: %d\n", packet->size);
		}
		av_packet_unref(packet);
	}

	avformat_free_context(pFormatCtx);
	printf("完成");
	return 0;
}