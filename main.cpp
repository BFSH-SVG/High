#include "asyncLogging.h"
#include <iostream>

int main() {
    auto& log = muduowebserv::AsyncLogging::instance();
    log.start();

    std::string s = "hello async log!\n";
    log.append(s.c_str(), s.size());

    log.stop();
    std::cout << "done" << std::endl;
    return 0;
}
