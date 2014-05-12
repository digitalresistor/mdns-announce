#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <tuple>

#include <signal.h>

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

inline void signal_int(ev::sig& sig, int) {
    auto *_data = reinterpret_cast<std::tuple<DNSServiceRef, std::vector<DNSRecordRef>>*>(sig.data);

    std::cerr << std::endl << "Caught SIGINT..." << std::endl;
    std::cout << "Deallocating records";
    for (auto &rec : std::get<1>(*_data)) {
        try {
            DNSServiceErrorType error;
            error = DNSServiceRemoveRecord(
                    std::get<0>(*_data),
                    rec,
                    0
                    );

            check_dnsservice_errors(error, "DNSServiceRemoveRecord");
        } catch (std::exception const& e) {
            std::cerr << "Caught an exception: " << e.what() << std::endl;
        }
    }
    std::cout << " [done]" << std::endl;

    sig.loop.break_loop();
}

int main(int argc, char *argv[]) {
    if (2 != argc) {
        std::cerr << argv[0] << " <announce file>" << std::endl;
        std::cerr << "Version: " << VERSION << std::endl;
        return -1;
    }

    std::vector<std::string> _domains;
    std::vector<DNSRecordRef> _records;

    DNSServiceRef serviceRef = 0;
    int dns_sd_fd = -1;

    std::tuple<DNSServiceRef, std::vector<DNSRecordRef>> _sigint_data;

    try {
        std::ifstream f(argv[1], std::ios::in | std::ios::binary);

        if (!f.is_open()) {
            std::cerr << "Unable to open file: " << argv[1] << std::endl;
            return -2;
        }

        for (std::string line; std::getline(f, line); /**/) {
            if (line.length() != 0) _domains.push_back(line);
        }

        DNSServiceErrorType error = kDNSServiceErr_NoError;

        // Setup the mDNS context...
        error = DNSServiceCreateConnection(&serviceRef);
        check_dnsservice_errors(error, "DNSServiceCreateConnection");

        for (auto &d : _domains) {
            std::cout << "Registering Domain: " << d << std::endl;
            DNSRecordRef record = 0;

            const char cname[] = { 9, 'a', 'l', 'e', 'x', 'a', 'n', 'd', 'r', 'a', 5, 'l', 'o', 'c', 'a', 'l', 0 };

            error = DNSServiceRegisterRecord(
                    serviceRef,                     // DNSServiceRef
                    &record,                        // DNSRecordRef
                    kDNSServiceFlagsShareConnection | kDNSServiceFlagsUnique,         // DNSServiceFlags
                    kDNSServiceInterfaceIndexAny,   // uint32_t interfaceIndex
                    d.c_str(),                      // fullname
                    kDNSServiceType_CNAME,          // rrtype
                    kDNSServiceClass_IN,            // rrclass
                    sizeof(cname),                  // rdlen
                    cname,                          // rdata (const void)
                    0,                              // uint32_t ttl
                    CallBack,                       // Callback
                    reinterpret_cast<void*>(&d)     // Context
                    );

            check_dnsservice_errors(error, "DNSServiceRegisterRecord");
            _records.push_back(record);

        }

        _sigint_data = std::make_tuple(serviceRef, _records);
        // Add event for catching SIGINT, so we can do proper cleanup
        ev::sig sio;
        sio.set<signal_int>(reinterpret_cast<void *>(&_sigint_data));
        sio.start(SIGINT);

        // Ignore SIGPIPE
        struct sigaction act;
        act.sa_handler = SIG_IGN;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGPIPE, &act, 0);

        // start the libev loop
        ev::default_loop loop;
        loop.run(0);
    } catch (std::exception const& e) {
        std::cerr << "Caught an exception: " << e.what() << std::endl;
    }

    DNSServiceRefDeallocate(serviceRef);

    return 0;
}
