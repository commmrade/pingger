#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/ioctl.h>
#include <utility>
#include "utils.hpp"

#pragma pack(push, 1)
struct icmp_full {
    icmphdr hdr;
    uint32_t timestamp;
};
#pragma pack(pop)


class netsocket {
public:
    enum class netsocket_options {
        hdrincl = IP_HDRINCL,
    };

    netsocket(int domain, int type, int proto);

    ~netsocket();

    netsocket(const netsocket&) = delete;

    netsocket& operator=(const netsocket&) = delete;

    netsocket(netsocket&& rhs) noexcept {
        std::swap(sock_, rhs.sock_);
    }

    netsocket& operator=(netsocket&&) noexcept = delete;

    void setsockoption(netsocket_options opt, int val);

    ssize_t send(const sockaddr_in& addr, const std::array<char, 32>& packet_buf) ;
    ssize_t recv(std::array<char, 32>& packet_buf);

    void close();
private:
    int sock_;
};


inline std::pair<std::array<char, 32>, sockaddr_in> prepare_send_packet(const std::string_view addr, int seq_n) {
    iphdr send_ip_p{};
    icmp_full send_p{};
    std::array<char, 32> send_buf{};

    send_p.hdr.checksum = 0; // calculate later
    send_p.hdr.type = 8;
    send_p.hdr.code = 0;
    send_p.hdr.un.echo.id = htons(getpid());
    send_p.hdr.un.echo.sequence = htons(seq_n);
    send_p.timestamp = htonl(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    send_p.hdr.checksum = (checksum(&send_p, sizeof(icmp_full)));

    send_ip_p.version = 4;
    send_ip_p.ihl = 5;
    send_ip_p.tos = 0;
    send_ip_p.tot_len = sizeof(iphdr) + sizeof(send_p); // 20 bytes + 12
    send_ip_p.id = htons(getpid());
    send_ip_p.frag_off = htons(0);
    send_ip_p.ttl = 64;
    send_ip_p.protocol = IPPROTO_ICMP;
    send_ip_p.check = htons(0);

    addrinfo *res;
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    int ret = getaddrinfo(nullptr, "3849", &hints, &res);
    if (ret < 0) {
        throw std::runtime_error{std::format("getaddrinfo: {}", std::strerror(errno))};
    }
    send_ip_p.saddr = reinterpret_cast<const sockaddr_in*>(res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);

    ret = inet_pton(AF_INET, addr.data(), &send_ip_p.daddr);
    if (ret < 0) {
        throw std::runtime_error{std::format("inet_pton: {}", std::strerror(errno))};
    }
    send_ip_p.check = htons(checksum(&send_ip_p, 20));

    std::ptrdiff_t offset{0};
    std::memcpy(send_buf.data(), &send_ip_p, sizeof(send_ip_p));
    offset += sizeof(send_ip_p);

    std::memcpy(send_buf.data() + offset, &send_p, sizeof(send_p));
    offset += sizeof(send_p);


    sockaddr_in endpoint{};
    endpoint.sin_addr.s_addr = send_ip_p.daddr;
    endpoint.sin_family = AF_INET;

    return {send_buf, endpoint};
}
