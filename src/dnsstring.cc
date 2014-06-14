#include <iostream>
#include <sstream>
#include <iomanip>
#include "dnsstring.h"

#ifdef DEBUG
std::string string_to_hex(const std::string& in) {
    std::stringstream ss;

    ss << std::setw(2) << std::setfill('0');
    for (unsigned char i : in) {
        if (i < 32) {
            ss << std::hex << static_cast<unsigned int>(static_cast<uint8_t>(i));
        }
        else {
            ss << i;
        }

    }

    return ss.str(); 
}
#endif

std::string to_dnsstring(const std::string& s) {
    std::string _dnsstr;
    auto begin = s.begin(), end = s.end();
    auto cur = begin;

    while (cur != end) {
        cur = std::find(begin, end, '.');

        size_t len = cur - begin;
        _dnsstr += static_cast<unsigned char>(static_cast<uint8_t>(len));
        _dnsstr += std::string(begin, cur);

        begin = cur + 1;
    }

    if (begin < end) {
        size_t len = end - begin;
        _dnsstr += static_cast<unsigned char>(static_cast<uint8_t>(len));
        _dnsstr += std::string(begin, cur);
    }

    // Append extra \0 if needed
    
    if (*(_dnsstr.rbegin()) != 0) {
        _dnsstr += static_cast<unsigned char>(static_cast<uint8_t>(0));
    }
#ifdef DEBUG
    std::cerr << string_to_hex(_dnsstr) << std::endl;
#endif

    return _dnsstr;
}
