
#include <iostream>
#include "Test.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "swscale.lib")
//avcodec.lib
//avdevice.lib
//avfilter.lib
//avformat.lib
//avutil.lib
//postproc.lib
//swresample.lib
//swscale.lib

int main()
{
    std::cout << "Test FFmpeg" << std::endl;
    std::cout << avcodec_configuration() << std::endl;
    Test* test = new Test();
    //test->test01();
    //test->testRtsp();
    test->testCamera();
    getchar();
}
