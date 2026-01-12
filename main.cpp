#include <array>
#include <chrono>
#include <csignal>
#include <exception>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <ratio>
#include <string_view>
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
#include "net.hpp"

int packets_sent{0};
std::string_view hostname;
double total_time_spent{0};

void signal_handler([[maybe_unused]] int signal) {
    std::println("\b\b--- {} statistics ---", hostname);
    std::println("Packages sent and received: {}; Time spent: {:.2f}ms", packets_sent, total_time_spent);
    _exit(0); // exit() is not async-signal-safe
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::println("Usage: pingger host.com");
        return 0;
    }
    hostname = argv[1];

    struct sigaction handler{};
    handler.sa_handler = signal_handler;
    int ret = sigaction(SIGINT, &handler, nullptr);
    if (ret < 0) {
        perror("sigaction");
        return -1;
    }

    try {
        netsocket socket{AF_INET, SOCK_RAW, IPPROTO_ICMP};
        socket.setsockoption(netsocket::netsocket_options::hdrincl, 1);

        int seq_n = 0;
        while (true){
            auto [send_buf, send_addr] = prepare_send_packet(hostname, seq_n);
            auto start = std::chrono::high_resolution_clock::now();
            auto sent_bytes = socket.send(send_addr, send_buf);
            if (sent_bytes < 0) {
                perror("send");
                return -1;
            }

            std::array<char, 32> recv_pack{};
            auto recv_bytes = socket.recv(recv_pack);
            auto end = std::chrono::high_resolution_clock::now();
            if (recv_bytes < 0) {
                perror("recv");
                return -1;
            }
            auto diff = std::chrono::duration<double, std::milli>{end - start};
            ++packets_sent;
            total_time_spent += diff.count();

            const iphdr* recv_ip = reinterpret_cast<const iphdr*>(recv_pack.data());
            std::array<char, INET_ADDRSTRLEN> addr_str{};
            const auto* r = inet_ntop(AF_INET, &recv_ip->saddr, addr_str.data(), addr_str.size());
            if (!r) {
                std::println("Failed to get address");
                return -1;
            }

            std::println("{} bytes from {}, seq: {}, time: {}", recv_bytes, std::string_view{addr_str.data(), addr_str.size()}, seq_n, diff.count());
            ++seq_n;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } catch (const std::exception& ex) {
        std::println("Error occured: {}", ex.what());
    }
}
