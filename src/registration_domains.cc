#include <iostream>

#include "registration_domains.h"
#include "error.h"

void callback(
        DNSServiceRef,
        DNSServiceFlags,
        uint32_t,
        DNSServiceErrorType error,
        const char *replyDomain,
        void *context) {
    check_dnsservice_errors(error, "RegistrationDomains callback");

    RegistrationDomains* _rd = reinterpret_cast<RegistrationDomains*>(context);

    if (_rd != 0) _rd->_start_registration(replyDomain);
}

RegistrationDomains::RegistrationDomains(decltype(_start_registration) sregs, ev::loop_ref loop) : _watcher(new ev::io()), _ev_loop(loop), _start_registration(sregs) {
    DNSServiceErrorType error = kDNSServiceErr_NoError;

    error = DNSServiceEnumerateDomains (
            &_service_ref, // DNSServiceRef
            kDNSServiceFlagsRegistrationDomains, // Flags
            kDNSServiceInterfaceIndexAny, // Interface
            callback, // callback function
            reinterpret_cast<void*>(this) // context
            );

    check_dnsservice_errors(error, "RegistrationDomains::RegistrationDomains");

    _dns_sd_fd = DNSServiceRefSockFD(_service_ref);

    // Set up the watcher ...
    _watcher->set(_ev_loop);
    _watcher->set<RegistrationDomains, &RegistrationDomains::dns_sd_callback>(this);
    _watcher->start(_dns_sd_fd, ev::READ);
}

RegistrationDomains::RegistrationDomains(decltype(_start_registration) sregs) : RegistrationDomains(sregs, ev::get_default_loop()) {}

RegistrationDomains::~RegistrationDomains() {
    if (_service_ref != 0) DNSServiceRefDeallocate(_service_ref);
}

void RegistrationDomains::dns_sd_callback(ev::io&, int revents) {
    if (EV_ERROR & static_cast<unsigned int>(revents)) {
        throw std::runtime_error("libev failed.");
    }

    if (static_cast<unsigned int>(revents) & ev::READ) {
        DNSServiceErrorType error = kDNSServiceErr_NoError;
        error = DNSServiceProcessResult(_service_ref);

        check_dnsservice_errors(error, "RegistrationDomains::dns_sd_callback");
    }
}

