#include "Utils.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <array>
#include <cstdint>

/* ======================== SHA-256 纯C++实现 ======================== */

static constexpr std::array<uint32_t, 64> SHA256_K = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9aca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static std::array<uint8_t, 32> sha256(const std::string& input) {
    // 预处理：填充消息
    size_t len = input.size();
    size_t bitLen = len * 8;
    size_t paddedLen = ((len + 8) / 64 + 1) * 64;
    std::vector<uint8_t> msg(paddedLen, 0);
    for (size_t i = 0; i < len; ++i) msg[i] = static_cast<uint8_t>(input[i]);
    msg[len] = 0x80;
    for (size_t i = 0; i < 8; ++i) msg[paddedLen - 1 - i] = static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF);

    // 初始哈希值
    uint32_t h0=0x6a09e667, h1=0xbb67ae85, h2=0x3c6ef372, h3=0xa54ff53a;
    uint32_t h4=0x510e527f, h5=0x9b05688c, h6=0x1f83d9ab, h7=0x5be0cd19;

    // 处理每个512位块
    for (size_t offset = 0; offset < paddedLen; offset += 64) {
        std::array<uint32_t, 64> w;
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[offset+i*4]) << 24) |
                   (static_cast<uint32_t>(msg[offset+i*4+1]) << 16) |
                   (static_cast<uint32_t>(msg[offset+i*4+2]) << 8) |
                    static_cast<uint32_t>(msg[offset+i*4+3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
            uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }

        uint32_t a=h0, b=h1, c=h2, d=h3, e=h4, f=h5, g=h6, h=h7;
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + SHA256_K[i] + w[i];
            uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e; h5+=f; h6+=g; h7+=h;
    }

    std::array<uint8_t, 32> result;
    std::array<uint32_t, 8> hh = {h0,h1,h2,h3,h4,h5,h6,h7};
    for (int i = 0; i < 8; ++i) {
        result[i*4]   = static_cast<uint8_t>((hh[i] >> 24) & 0xFF);
        result[i*4+1] = static_cast<uint8_t>((hh[i] >> 16) & 0xFF);
        result[i*4+2] = static_cast<uint8_t>((hh[i] >> 8) & 0xFF);
        result[i*4+3] = static_cast<uint8_t>(hh[i] & 0xFF);
    }
    return result;
}

/* ======================== 公共接口 ======================== */

std::string generateRandomSalt() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {  // 16字节 = 32个16进制字符
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

std::string hashPassword(const std::string& password, const std::string& secret_salt) {
    // SHA-256 + 盐，10000轮迭代（类PBKDF2）
    std::string combined = secret_salt + password + secret_salt;
    auto hash = sha256(combined);
    // 多轮迭代增强安全性
    for (int i = 0; i < 9999; ++i) {
        std::string round_input;
        round_input.reserve(32 + combined.size());
        for (auto b : hash) round_input += static_cast<char>(b);
        round_input += combined;
        hash = sha256(round_input);
    }
    std::stringstream ss;
    for (auto b : hash) ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return ss.str();
}
