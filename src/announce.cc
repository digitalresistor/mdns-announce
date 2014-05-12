#include <iostream>
#include <dns_sd.h>
#include <ev++.h>

#include "build_version.h"

int main(int argc, char *argv[]) {
    if (2 != argc) {
        std::cerr << argv[0] << " <announce file>" << std::endl;
        std::cerr << "Version: " << VERSION << std::endl;
        return -1;
    }
    return 0;
}
