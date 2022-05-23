#include <iostream>

class Test {
public:
    Test() {
        std::cout << "construct Test..." << std::endl;
    };

    ~Test() {
        std::cout << "destruct Test..." << std::endl;
    }
public:
    void test01();
    int testRtsp();
    int testCamera();
private:

};