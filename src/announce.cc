#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <signal.h>
#include <unistd.h>

#include <dns_sd.h>
#include <ev++.h>

#include "build_version.h"
#include "dnsstring.h"
#include "error.h"

static void CallBack(DNSServiceRef, DNSRecordRef, const DNSServiceFlags, DNSServiceErrorType errorCode, void *context)
{
    std::string *name = reinterpret_cast<std::string*>(context);

    std::cout << "Successfully registered: " << *name << std::endl;
    check_dnsservice_errors(errorCode, "CallBack");
}

void ev_mdns_watcher(ev::io &w, int) {
    DNSServiceRef serviceRef = reinterpret_cast<DNSServiceRef>(w.data);

    DNSServiceErrorType error = kDNSServiceErr_NoError;
    error = DNSServiceProcessResult(serviceRef);

    check_dnsservice_errors(error, "ev_mdns_watcher(DNSServiceProcessResult)");
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

    std::vector<std::tuple<std::string, std::string>> _domains;
    std::vector<DNSRecordRef> _records;
    std::string hostname;

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
            if (line.length() != 0) {
                std::stringstream l(line);

                std::string domain, cname;

                std::getline(l, domain, ':');
                std::getline(l, cname);

                if (cname == std::string("$self")) {
                    if (hostname.length() == 0) {
                        char _hostname[256] = {0};

                        if (gethostname(_hostname, sizeof(_hostname)) == -1) {
                            throw std::runtime_error("gethostname: Failed to get current hostname...");
                        }

                        hostname = std::string(_hostname);
                    }

                    cname = hostname;
                }

                _domains.push_back(std::make_tuple(domain, cname));
            }
        }

        DNSServiceErrorType error = kDNSServiceErr_NoError;

        // Setup the mDNS context...
        error = DNSServiceCreateConnection(&serviceRef);
        check_dnsservice_errors(error, "DNSServiceCreateConnection");

        for (auto &dtocname : _domains) {
            std::string &d = std::get<0>(dtocname);
            std::string &c = std::get<1>(dtocname);
            std::string scname = to_dnsstring(c);

            std::cout << "Registering Domain: " << d << " -> " << c << std::endl;
            DNSRecordRef record = 0;

            error = DNSServiceRegisterRecord(
                    serviceRef,                     // DNSServiceRef
                    &record,                        // DNSRecordRef
                    kDNSServiceFlagsShareConnection | kDNSServiceFlagsUnique,         // DNSServiceFlags
                    kDNSServiceInterfaceIndexAny,   // uint32_t interfaceIndex
                    d.c_str(),                      // fullname
                    kDNSServiceType_CNAME,          // rrtype
                    kDNSServiceClass_IN,            // rrclass
                    scname.length(),                // rdlen
                    scname.c_str(),                 // rdata (const void)
                    0,                              // uint32_t ttl
                    CallBack,                       // Callback
                    reinterpret_cast<void*>(&d)     // Context
                    );

            check_dnsservice_errors(error, "DNSServiceRegisterRecord");
            _records.push_back(record);

        }

        dns_sd_fd = DNSServiceRefSockFD(serviceRef);

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

        ev::io mdns_watch;
        mdns_watch.set<ev_mdns_watcher>(reinterpret_cast<void *>(serviceRef));
        mdns_watch.set(dns_sd_fd, ev::READ);
        mdns_watch.start();

        // start the libev loop
        ev::default_loop loop;
        loop.run(0);
    } catch (std::exception const& e) {
        std::cerr << "Caught an exception: " << e.what() << std::endl;
    }

    DNSServiceRefDeallocate(serviceRef);

    return 0;
}
