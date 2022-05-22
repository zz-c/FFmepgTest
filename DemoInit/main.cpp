
#include <iostream>
#include "Test.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

int main()
{
    std::cout << "Test FFmpeg" << std::endl;
    std::cout << avcodec_configuration() << std::endl;
    //av_register_all();
    //getchar();
    //Test* test = new Test();
    //test->test01();
}
