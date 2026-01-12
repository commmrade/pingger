#include "utils.hpp"

uint16_t checksum(void *addr, int count) {
    uint32_t sum = 0;
    auto* ptr = reinterpret_cast<uint16_t*>(addr);
    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }
    if (count > 0)
        sum += *(uint8_t*)ptr;
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}
