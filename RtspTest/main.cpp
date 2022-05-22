
#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
}

int main()
{
    std::cout << "Test Rtsp" << std::endl;
    std::cout << avcodec_configuration() << std::endl;
    getchar();
}
