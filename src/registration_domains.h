#ifndef REGISTRATION_DOMAINS_H_A532E0ABCD5F4F
#define REGISTRATION_DOMAINS_H_A532E0ABCD5F4F

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

class RegistrationDomains {
    private:
        std::unique_ptr<ev::io> _watcher;
        ev::loop_ref _ev_loop = ev::get_default_loop();
        DNSServiceRef _service_ref = 0;
        int _dns_sd_fd = -1;

        std::function<void(const std::string&)> _start_registration;

    public:
        RegistrationDomains(decltype(_start_registration) sregs);
        RegistrationDomains(decltype(_start_registration), ev::loop_ref loop);
        virtual ~RegistrationDomains();

        RegistrationDomains(const RegistrationDomains&) = delete;
        RegistrationDomains& operator=(const RegistrationDomains&) = delete;
    protected:
        void dns_sd_callback(ev::io& w, int revents);
        friend void callback(DNSServiceRef, DNSServiceFlags, uint32_t, DNSServiceErrorType, const char*, void*);
};

#endif /* REGISTRATION_DOMAINS_H_A532E0ABCD5F4F */

