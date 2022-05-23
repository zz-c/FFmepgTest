#include <iostream>
#include <thread>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
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
* ��AVFrame(YUV420��ʽ)����ΪJPEG��ʽ��ͼƬ
*
* @param width YUV420�Ŀ�
* @param height YUV42�ĸ�
*
*/
int saveFrame2JPEG(AVFrame* pFrame, int width, int height, int iIndex) {
	AVCodecContext* pCodeCtx = NULL;


	AVFormatContext* pFormatCtx = avformat_alloc_context();
	// ��������ļ���ʽ
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

	// ��������ʼ�����AVIOContext
	if (avio_open(&pFormatCtx->pb, "test.jpg", AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// ����һ����stream
	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	AVCodecParameters* parameters = pAVStream->codecpar;
	parameters->codec_id = pFormatCtx->oformat->video_codec;
	parameters->codec_type = AVMEDIA_TYPE_VIDEO;
	parameters->format = AV_PIX_FMT_YUVJ420P;
	parameters->width = pFrame->width;
	parameters->height = pFrame->height;

	AVCodec* pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);

	if (!pCodec) {
		printf("Could not find encoder ");
		return -1;
	}

	pCodeCtx = avcodec_alloc_context3(pCodec);
	if (!pCodeCtx) {
		fprintf(stderr, "Could not allocate video codec context ");
		exit(1);
	}

	if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context ",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return -1;
	}
	AVRational Rat;
	Rat.num = 1;
	Rat.den = 25;
	pCodeCtx->time_base = Rat;

	if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	int ret = avformat_write_header(pFormatCtx, NULL);
	if (ret < 0) {
		printf("write_header fail ");
		return -1;
	}

	int y_size = width * height;

	//Encode
	// ��AVPacket�����㹻��Ŀռ�
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// ��������
	ret = avcodec_send_frame(pCodeCtx, pFrame);
	if (ret < 0) {
		printf("Could not avcodec_send_frame.");
		return -1;
	}

	// �õ����������
	ret = avcodec_receive_packet(pCodeCtx, &pkt);
	if (ret < 0) {
		printf("Could not avcodec_receive_packet");
		return -1;
	}

	ret = av_write_frame(pFormatCtx, &pkt);

	if (ret < 0) {
		printf("Could not av_write_frame");
		return -1;
	}

	av_packet_unref(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);


	avcodec_close(pCodeCtx);
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}


/**
 * ��ʼ�����װʹ��avformat_open_input��MP4�ļ�����������ʱ������TestDemux
 */
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


	//��ȡ����Ϣ 
	re = avformat_find_stream_info(inAVFormatContext, 0);

	//��ʱ�� ����
	int totalMs = inAVFormatContext->duration / (AV_TIME_BASE / 1000);
	std::cout << "total time ms = " << totalMs << std::endl;
	//��ӡ��Ƶ����ϸ��Ϣ
	av_dump_format(inAVFormatContext, 0, filePath, 0);

	//����Ƶ��������ȡʱ��������Ƶ
	int videoStream = 0;
	int audioStream = 1;

	//��ȡ����Ƶ����Ϣ ��������������ȡ�� videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	for (int i = 0; i < inAVFormatContext->nb_streams; i++)
	{
		AVStream* stream = inAVFormatContext->streams[i];
		std::cout << "codec_id = " << stream->codecpar->codec_id << std::endl;
		std::cout << "format = " << stream->codecpar->format << std::endl;

		//��Ƶ AVMEDIA_TYPE_AUDIO
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			std::cout << i << "��Ƶ��Ϣ" << std::endl;
			std::cout << "sample_rate = " << stream->codecpar->sample_rate << std::endl;
			//AVSampleFormat;
			std::cout << "channels = " << stream->codecpar->channels << std::endl;
			//һ֡���ݣ��� ��ͨ�������� 
			std::cout << "frame_size = " << stream->codecpar->frame_size << std::endl;
			//1024 * 2 * 2 = 4096  fps = sample_rate/frame_size

		}
		//��Ƶ AVMEDIA_TYPE_VIDEO
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			std::cout << i << "��Ƶ��Ϣ" << std::endl;
			std::cout << "width=" << stream->codecpar->width << std::endl;
			std::cout << "height=" << stream->codecpar->height << std::endl;
			//֡�� fps ����ת��
			std::cout << "video fps = " << r2d(stream->avg_frame_rate) << std::endl;
		}
	}

	//��ȡ��Ƶ��
	videoStream = av_find_best_stream(inAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//malloc AVPacket����ʼ��
	AVPacket* pkt = av_packet_alloc();
	for (;;)
	{
		int re = av_read_frame(inAVFormatContext, pkt);
		if (re != 0)
		{
			//ѭ������
			std::cout << "==============================end==============================" << std::endl;
			int ms = 3000; //����λ�� ����ʱ�������������ת��
			long long pos = (double)ms / (double)1000 * r2d(inAVFormatContext->streams[pkt->stream_index]->time_base);
			av_seek_frame(inAVFormatContext, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		std::cout << "pkt->size = " << pkt->size << std::endl;
		//��ʾ��ʱ��
		std::cout << "pkt->pts = " << pkt->pts << std::endl;
		//ת��Ϊ���룬������ͬ��
		std::cout << "pkt->pts ms = " << pkt->pts * (r2d(inAVFormatContext->streams[pkt->stream_index]->time_base) * 1000) << std::endl;
		//����ʱ��
		std::cout << "pkt->dts = " << pkt->dts << std::endl;
		if (pkt->stream_index == videoStream)
		{
			std::cout << "ͼ��" << std::endl;
		}
		if (pkt->stream_index == audioStream)
		{
			std::cout << "��Ƶ" << std::endl;
		}
		//�ͷţ����ü���-1 Ϊ0�ͷſռ�
		av_packet_unref(pkt);
		//XSleep(500);
	}

	av_packet_free(&pkt);

	if (inAVFormatContext)
	{
		//�ͷŷ�װ�����ģ����Ұ�inAVFormatContext��0
		avformat_close_input(&inAVFormatContext);
	}
}

int Test::testRtsp()
{
	std::cout << "testRtsp..." << std::endl;
	//��ʼ����װ��
	av_register_all();
	//��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	avformat_network_init();

	const char* url = "rtsp://admin:zz369369@192.168.1.171:554/stream1";

	AVFormatContext* pFormatCtx = avformat_alloc_context();
	//��������
	AVDictionary* dictionaryOpts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//������ʱʱ��
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);
	
	int ret = -1;
	ret = avformat_open_input(&pFormatCtx, url, 0, &dictionaryOpts);
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
	//��ȡ��Ƶ���еı����������
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;��ʱ
	AVCodecContext*  pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
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


	AVPacket* packet = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();
	int frameFinished;
	while (1) {
		ret = av_read_frame(pFormatCtx, packet);
		if (ret < 0) {
			fprintf(stderr, "error or end of file: %d\n", ret);
			continue;
		}
		if (packet->stream_index == audio_stream_index) {
			fprintf(stdout, "audio stream, packet size: %d\n", packet->size);
			continue;
		}
		if (packet->stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", packet->size);

			//ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
			ret = avcodec_send_packet(pCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) {
				printf("%s", "�������");
			}
			//if (frameFinished) {
				//����Ƶ֡ԭ���ĸ�ʽpCodecCtx->pix_fmtת����RGB
				//saveFrame2JPEG(pFrame, pCodecCtx->width, pCodecCtx->height, cnt);
				//break;
			//}
			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret != 0) {
				fprintf(stderr,"avcodec_receive_frame failed !");
			}
			else {
				saveFrame2JPEG(pFrame, pCodecCtx->width, pCodecCtx->height, 0);
				break;
			}
			//break;
		}
	
		
	}

	avformat_free_context(pFormatCtx);
}

int Test::testCamera()
{
	std::cout << "testCamera..." << std::endl;
	av_register_all();
	avdevice_register_all();

	//�������뷽ʽ
	AVInputFormat* inputFormat = av_find_input_format("dshow");
	AVDictionary* format_opts = nullptr;
	av_dict_set_int(&format_opts, "rtbufsize", 3041280 * 100, 0);//���[video input] too full or near too full Ĭ�ϴ�С3041280
	//av_dict_set(&format_opts, "avioflags", "direct", 0);
	//av_dict_set(&format_opts, "video_size", "1280x720", 0);
	//av_dict_set(&format_opts, "framerate", "30", 0);
	//av_dict_set(&format_opts, "vcodec", "mjpeg", 0);

	AVFormatContext* pFormatCtx = avformat_alloc_context();
	const char* psDevName = "video=USB Camera";

	int ret = avformat_open_input(&pFormatCtx, psDevName, inputFormat, &format_opts);
	if (ret < 0)
	{
		std::cout << "AVFormat Open Input Error!" << std::endl;
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
	fprintf(stdout, "Number of elements in AVFormatContext.streams: %d\n", pFormatCtx->nb_streams);
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		const AVStream* stream = pFormatCtx->streams[i];
		fprintf(stdout, "type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		}
	}
	//��ȡ��Ƶ���еı����������
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;��ʱ
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
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


}