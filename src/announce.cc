#include <iostream>
#include <dns_sd.h>
#include <ev++.h>

#include "build_version.h"


inline void check_dnsservice_errors(DNSServiceErrorType& e, const std::string& func_name) {
    std::string error(func_name);
    error += ": ";

    switch (e) {
        case kDNSServiceErr_NoError:
            return;
        case kDNSServiceErr_NameConflict:
            error += "Name in use, please choose another";
            break;
        case kDNSServiceErr_Invalid:
            error += "An invalid index or character was passed";
            break;
        case kDNSServiceErr_BadReference:
            error += "Bad reference";
            break;
        case kDNSServiceErr_BadParam:
            error += "Bad parameter value passed to function.";
            break;
        default:
            error += "Unknown error code... ";
            error += std::to_string(e);
    }

    throw std::runtime_error(error.c_str());
}

int main(int argc, char *argv[]) {
    if (2 != argc) {
        std::cerr << argv[0] << " <announce file>" << std::endl;
        std::cerr << "Version: " << VERSION << std::endl;
        return -1;
    }
    return 0;
}
