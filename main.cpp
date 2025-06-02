#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <ratio>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <print>
#include "packets.hpp"

uint16_t checksum(void *addr, int count) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)addr;
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

struct SocketWrapper {
    int sock{-1};
    SocketWrapper(int sock) : sock(sock) {}
    ~SocketWrapper() { if (sock >= 0) close(sock); }
    operator int() {
        return sock;
    }
};


int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("Usage: pingger host.com");
        return 0;
    }
    char* hostname = argv[1]; // Host which we wanna ping

    outcoming_packet send_buffer{}; // Self-explanotary?
    incoming_packet recv_buffer{}; // Init to zeros

    SocketWrapper sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); // Createing raw socket, so OS doesn't add TCP/UDP headersr
    if (sock < 0) {
        std::cerr << "Could not create a RAW socket, please run with sudo!" << std::endl;
        return -1;
    }
    
    sockaddr_in dest_addr{}; // Here we store destination address info like ip addr and yada yada
    addrinfo* hostname_info; // A list of addresses of "hostname" host, it is a linked list kinda actually
    addrinfo hints{};
    hints.ai_family = AF_INET; // We're gonna look only for ipv4 addressses
    if (getaddrinfo(hostname, nullptr, &hints, &hostname_info) != 0) {
        std::cerr << "Unable to get address info from this hostname. Service not known" << std::endl;
        return -1;
    }
    sockaddr_in* address_of_host;
    for (auto* ptr = hostname_info; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr) { // Check if valid then add
            address_of_host = (sockaddr_in*)ptr->ai_addr;
            break;
        }
    }
    if (!address_of_host) {
        freeaddrinfo(hostname_info);
        std::cerr << "Address is null. Why TF?" << std::endl;
    }

    dest_addr.sin_addr = address_of_host->sin_addr;
    dest_addr.sin_family = AF_INET; // Dest address is IPv4
    auto current_seq = 1;
    while (true) {  
        memset(&send_buffer, 0, sizeof(outcoming_packet)); // Clear the buffer

        // Setup icmp headers
        send_buffer.icmp_header.icmp_type = ICMP_ECHO;
        send_buffer.icmp_header.icmp_code = 0; // 0 for echo reply i think
        send_buffer.icmp_header.icmp_id = htons(getuid() & 0xffff);
        send_buffer.icmp_header.icmp_seq = htons(current_seq);
        send_buffer.icmp_header.icmp_cksum = checksum(&send_buffer.icmp_header, sizeof(icmp));

        auto start = std::chrono::high_resolution_clock::now();
        if (sendto(sock, &send_buffer, sizeof(outcoming_packet), 0, (sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            freeaddrinfo(hostname_info);
            perror("Could not send");
            return -1;
        }

        sockaddr_in recv;
        socklen_t socklen = sizeof(recv);
        if (recvfrom(sock, &recv_buffer, sizeof(incoming_packet), 0, (sockaddr*)&recv, &socklen) < 0) {
            freeaddrinfo(hostname_info);
            perror("Could not receive");
            return -1;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration<double, std::milli>{end - start};

        if (recv_buffer.icmp_header.icmp_type == ICMP_ECHOREPLY) {
            std::println("Size: {}, IP: {}; Ping - {}ms, Sequence: {}, TTL: {}", 
                sizeof(ip) + sizeof(icmp), 
                inet_ntoa(recv.sin_addr), 
                dur.count(), 
                current_seq, 
                recv_buffer.ip_header.ip_ttl
            );
        } // Else just skip it
        ++current_seq;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    freeaddrinfo(hostname_info);
}