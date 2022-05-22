#pragma once
extern "C" {
#include "libavutil/log.h"
	//����
#include "libavcodec/avcodec.h"
//��װ��ʽ����
#include "libavformat/avformat.h"
//���ش���
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h" 

#include "libavutil/time.h"
}

/*
��֡���浽�ļ�

*/
void saveFrame2Ppm(AVFrame* pFrame, int width, int height, int iFrame) {

	FILE* pFile;		//�ļ�ָ��
	char szFilename[32];//�ļ������ַ�����
	int y;				//

	sprintf(szFilename, "rtspToPpm%04d.ppm", iFrame);	//�����ļ���
	pFile = fopen(szFilename, "wb");			//���ļ���ֻд��
	if (pFile == NULL) {
		return;
	}

	//getch();

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);//���ĵ��м��룬������룬��ȻPPM�ļ��޷���ȡ

	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
	}

	fclose(pFile);

}
/**
* ��AVFrame(YUV420��ʽ)����ΪJPEG��ʽ��ͼƬ
*
* @param width YUV420�Ŀ�
* @param height YUV42�ĸ�
*
*/
int saveFrame2JPEG(AVFrame* pFrame, int width, int height, int iIndex) {
	// ����ļ�·��

	char out_file[1000] = { 0 };
	sprintf(out_file, "rtspToJpg%04d.jpg", iIndex);
	printf("out_file:%s", out_file);
	// ����AVFormatContext����
	AVFormatContext* pFormatCtx = avformat_alloc_context();

	// ��������ļ���ʽ
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
	// ��������ʼ��һ���͸�url��ص�AVIOContext
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// ����һ����stream
	//Using AVStream.codec to pass codec parameters to muxers is deprecated, use AVStream.codecpar instead |||AVCodecParametersstream->codecpar

	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	// ���ø�stream����Ϣ
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

	// ���ҽ�����
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec) {
		printf("Codec not found.");
		return -1;
	}
	// ����pCodecCtx�Ľ�����ΪpCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	//Write Header
	avformat_write_header(pFormatCtx, NULL);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode
	// ��AVPacket�����㹻��Ŀռ�
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
	printf("�ļ���ʽ��%s\n", pFormatCtx->iformat->name);
	printf("�ļ�ʱ����%d��\n", (pFormatCtx->duration) / 1000000);

	//ֻ��֪����Ƶ�ı��뷽ʽ�����ܹ����ݱ��뷽ʽȥ�ҵ�������
	//��ȡ��Ƶ���еı����������
	AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
	//���ݱ�����������еı���id���Ҷ�Ӧ�Ľ���
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "�Ҳ���������\n");
		return -1;
	}
	//�򿪽�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "�������޷���\n");
		return -1;
	}
	//�����Ƶ��Ϣ
	printf("��Ƶ��pix_fmt��%d\n", pCodecCtx->pix_fmt);
	printf("��Ƶ�Ŀ�ߣ�%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("��Ƶ�����������ƣ�%s\n", pCodec->name);

	AVCodecContext* pCodecCtx2 = pFormatCtx->streams[audio_stream_index]->codec;
	AVCodec* pCodec2 = avcodec_find_decoder(pCodecCtx2->codec_id);
	//�򿪽�����
	if (avcodec_open2(pCodecCtx2, pCodec2, NULL) < 0)
	{
		printf("%s", "������2�޷���\n");
		return -1;
	}
	//�����Ƶ��Ϣ
	printf("��Ƶ������2�����ƣ�%s\n", pCodec2->name);
	printf("��Ƶ������2��channels��%d\n", pCodecCtx2->channels);
	printf("��Ƶ������2��sample_fmt��%d\n", pCodecCtx2->sample_fmt);
	printf("��Ƶ������2��frame_size��%d\n", pCodecCtx2->frame_size);
	printf("��Ƶ������2��channel_layout��%d\n", pCodecCtx2->channel_layout);


	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameRGB = av_frame_alloc();


	//������Ƶ֡��Ҫ�ڴ� (���ԭʼ������)
	int numBytes;		//��Ҫ���ڴ��С
	uint8_t* buffer = NULL;
	//��ȡ��Ҫ���ڴ��С
	/*
	1. av_image_fill_arrays ���������� frame �����Ǹղŷ�����ڴ�
	2. av_malloc ��һ�� FFmpeg �� malloc��
	��Ҫ�Ƕ� malloc ����һЩ��װ����֤��ַ����֮������飬
	���ᱣ֤��Ĵ��벻�����ڴ�й©������ͷŻ����� malloc ����
	*/
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);//��ȡ��Ҫ���ڴ��С
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//����frame�͸ղŷ��������
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);



	//��ʼ��SWS������
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
				printf("%s", "�������");
			}
			if (frameFinished) {
				if (cnt == 2) {
					//����Ƶ֡ԭ���ĸ�ʽpCodecCtx->pix_fmtת����RGB
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
	printf("���");
	return 0;
}