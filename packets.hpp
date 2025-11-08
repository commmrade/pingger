#pragma once
#include <netinet/ip_icmp.h>

struct outcoming_packet {
    icmp icmp_header;
};
struct incoming_packet {
    // ip ip_header;
    icmp icmp_header;
    // Don't care about anything else here
};
