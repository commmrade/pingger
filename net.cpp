#include "net.hpp"

netsocket::netsocket(int domain, int type, int proto) {
    sock_ = socket(domain, type, proto);
    if (sock_ < 0) {
        throw std::runtime_error(std::format("socket: {}", std::strerror(errno)));
    }
}
netsocket::~netsocket() {
    close();
}

void netsocket::setsockoption(netsocket_options opt, int val) {
    int ret = setsockopt(sock_, IPPROTO_IP, static_cast<int>(opt), &val, sizeof(val));
    if (ret < 0) {
        throw std::runtime_error(std::format("setsockopt: {}", std::strerror(errno)));
    }
}

ssize_t netsocket::send(const sockaddr_in& addr, const std::array<char, 32>& packet_buf) {
    auto sent_b = ::sendto(sock_, packet_buf.data(), packet_buf.size(), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    return sent_b;
}
ssize_t netsocket::recv(std::array<char, 32>& packet_buf) {
    auto recv_bytes = ::recv(sock_, packet_buf.data(), packet_buf.size(), 0);
    return recv_bytes;
}

void netsocket::close() {
    ::close(sock_);
}
