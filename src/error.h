#ifndef ERROR_H_5C0CD7EC64028D
#define ERROR_H_5C0CD7EC64028D

#include <string>

#include <dns_sd.h>

void check_dnsservice_errors(DNSServiceErrorType& e, const std::string& func_name);

#endif /* ERROR_H_5C0CD7EC64028D */

