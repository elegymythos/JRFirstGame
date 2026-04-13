#include "Utils.hpp"
#include <random>
#include <sstream>
#include <iomanip>

std::string generateRandomSalt() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

std::string hashPassword(const std::string& password, const std::string& secret_salt) {
    std::string combined = password + secret_salt;
    unsigned long hash = 5381;
    for (char c : combined) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}