
#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
}

int main()
{
    std::cout << "Test FFmpeg" << std::endl;
    std::cout << avcodec_configuration() << std::endl;
    getchar();
}
