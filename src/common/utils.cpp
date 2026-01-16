#include <random>
#include <algorithm>
#include <functional>
// #include <openssl/rand.h>
#include <cstring>
#include <stdexcept>
#include "utils.h"

std::vector<uint64_t> GenerateRandom64Cuckoo(std::size_t count) {
  std::vector<uint64_t> result(count);
  std::random_device random;
  // To generate random keys to lookup, this uses ::std::random_device which is slower but
  // stronger than some other pseudo-random alternatives. The reason is that some of these
  // alternatives (like libstdc++'s ::std::default_random, which is a linear congruential
  // generator) behave non-randomly under some hash families like Dietzfelbinger's
  // multiply-shift.
  auto genrand = [&random]() {
    return random() + (static_cast<uint64_t>(random()) << 32);
  };
  std::generate(result.begin(), result.end(), std::ref(genrand));
  return result;
}

// Deepseek + Kimi
std::vector<uint64_t> GenerateRandom64(std::size_t count) {
    std::mt19937_64 generator(std::random_device{}());
    
    std::vector<uint64_t> result(count);
    std::generate(result.begin(), result.end(), std::ref(generator));
    return result;
}

// std::vector<uint64_t> GenerateRandomOpenSSL(std::size_t count) {
//     std::vector<uint64_t> vals(count);
//     if (RAND_bytes((unsigned char *)vals.data(), sizeof(uint64_t) * count) != 1) {
//         throw std::runtime_error("OpenSSL RAND_bytes failed");
//     }
//     return vals;
// }

int reliable_send(int sockfd, const void *data, uint32_t length) {
    if (0 == length) return 0;
    uint32_t total_sent = 0;
    const char *buffer = static_cast<const char *>(data);

    while (total_sent < length) {
        uint32_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue; // 被信号中断，重试
            std::cerr << "Failed to send data over socket. errno: " << errno << std::endl;
            return -1;
        }
        else if (sent == 0) {
            std::cerr << "Connection closed by peer during send" << std::endl;
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

int reliable_recv(int sockfd, void *data, uint32_t length) {
    if (0 == length) return 0;
    uint32_t total_received = 0;
    char *buffer = static_cast<char *>(data);

    while (total_received < length) {
        uint32_t received = recv(sockfd, buffer + total_received, length - total_received, 0);
        if (received < 0) {
            if (errno == EINTR) continue; // 被信号中断，重试
            std::cerr << "Failed to receive data over socket. errno: " << errno << std::endl;
            return -1;
        }
        else if (received == 0) {
            std::cerr << "Connection closed by peer during recv" << std::endl;
            return -1;
        }
        total_received += received;
    }
    return total_received;
}

void assert_else(bool condition, const std::string& message, bool exit_on_failure) {
    if (!condition) std::cerr << "[Assert Failure] " << message << std::endl;
    if (!condition && exit_on_failure) exit(-1);
}

void alloc_aligned_64(void** ptr, std::size_t size) {
    int ret = posix_memalign(ptr, 64, size);
    assert_else(ret == 0, "posix_memalign failed");
    memset(*ptr, 0, size);
    return;
}

void sync_client(int const &sockfd) {
    char cmd[6];
    reliable_send(sockfd, "READY", 6);
    reliable_recv(sockfd, cmd, 3);
    std::cout << "[Client] " << get_current_time_string() << " Sync point reached." << std::endl;
    return;
}

void sync_server(std::vector<int> const &list_sockfd) {
    char cmd[6];
    for (auto const &sockfd : list_sockfd) {
        reliable_recv(sockfd, cmd, 6);
    }
    for (auto const &sockfd : list_sockfd) {
        reliable_send(sockfd, "GO", 3);
    }
    std::cout << "[Server] " << get_current_time_string() << " Sync point reached." << std::endl;
    return;
}

std::string get_current_time_string() {
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", std::localtime(&now));
    return std::string(buf);
}