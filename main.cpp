#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <ratio>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <print>
#include <sys/ioctl.h>

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

int packets_sent{0};
char* hostname;
double total_time_spent{0};
void sigint_handler(int) {
    std::println("\b\b--- {} statistics ---", hostname);
    std::println("Packages sent: {}, Packages Received: {}\nTime spent: {:.2f}ms", packets_sent, packets_sent, total_time_spent);
    exit(0);
}

template <typename Action>
struct defer {
    Action a;
    ~defer() {
        a();
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("Usage: pingger host.com");
        return 0;
    }
    signal(SIGINT, sigint_handler);

    hostname = argv[1]; // Host which we wanna ping
    icmp s_icmp{}; // Self-explanotary?
    icmp r_icmp{}; // Init to zeros

    SocketWrapper sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP); // Createing raw socket, so OS doesn't add TCP/UDP headersr
    if (sock < 0) {
        std::cerr << "Could not create a socket!" << std::endl;
        return -1;
    }
    
    addrinfo* hostname_info; // A list of addresses of "hostname" host, it is a linked list kinda actually
    addrinfo hints{};
    hints.ai_family = AF_INET; // We're gonna look only for ipv4 addressses
    if (getaddrinfo(hostname, nullptr, &hints, &hostname_info) != 0) {
        std::cerr << "Unable to get address info from this hostname. Service not known" << std::endl;
        return -1;
    }
    defer def{[hostname_info]{
        freeaddrinfo(hostname_info);
    }};

    sockaddr_in* address_of_host;
    for (auto* ptr = hostname_info; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr) { // Check if valid then add
            address_of_host = (sockaddr_in*)ptr->ai_addr;
            break;
        }
    }

    if (connect(sock, (const sockaddr*)address_of_host, sizeof(*address_of_host)) < 0) {
        std::cerr << "Unable to connect\n";
        return -1;
    }

    auto current_seq = 1;

    s_icmp.icmp_type = ICMP_ECHO;
    s_icmp.icmp_code = 0;
    s_icmp.icmp_id = 1433;

    while (true) {
        s_icmp.icmp_seq = htons(current_seq);
        s_icmp.icmp_cksum = 0;
        s_icmp.icmp_cksum = checksum(&s_icmp, sizeof(s_icmp));

        auto start = std::chrono::high_resolution_clock::now();
        if (send(sock, &s_icmp, sizeof(s_icmp), 0) < 0) {
            perror("Could not send");
            return -1;
        }

        sockaddr_in recv;
        socklen_t socklen = sizeof(recv);
        ssize_t rd_bytes = recvfrom(sock, &r_icmp, sizeof(r_icmp), 0, (sockaddr*)&recv, &socklen);
        if (rd_bytes < 0) {
            perror("Could not receive");
            return -1;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration<double, std::milli>{end - start};

        if (r_icmp.icmp_type == ICMP_ECHOREPLY) {
        std::println("Size: {}, IP: {}; Ping - {:.2f}ms, Sequence: {}",
            rd_bytes,
            inet_ntoa(recv.sin_addr),
            dur.count(),
            current_seq
        );
        } // Else just skip it

        ++current_seq;
        ++packets_sent;
        total_time_spent += dur.count();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
