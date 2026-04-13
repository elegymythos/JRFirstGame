#pragma once
#include <string>

/**
 * @brief 生成随机盐值（16进制字符串，长度16）
 * @return 随机盐值字符串
 */
std::string generateRandomSalt();

/**
 * @brief 对密码进行哈希（djb2 算法 + 盐）
 * @param password 明文密码
 * @param secret_salt 盐值
 * @return 哈希后的16进制字符串
 */
std::string hashPassword(const std::string& password, const std::string& secret_salt);