/**
 * @file Utils.hpp
 * @brief 工具函数：随机盐值生成和密码哈希
 *
 * 提供两个安全相关的工具函数：
 * - generateRandomSalt()：生成16字节随机数，输出为32字符的16进制字符串
 * - hashPassword()：使用纯C++实现的SHA-256，加盐后做10000轮迭代哈希
 *
 * 注意：此实现用于学习项目，生产环境应使用系统级加密库。
 */

#pragma once
#include <string>

/**
 * @brief 生成随机盐值
 * @return 32字符的16进制字符串（16字节随机数）
 *
 * 使用 std::random_device 获取真随机数种子。
 */
std::string generateRandomSalt();

/**
 * @brief 对密码进行哈希
 * @param password     明文密码
 * @param secret_salt  盐值
 * @return 64字符的16进制字符串（SHA-256哈希结果）
 *
 * 哈希过程（类PBKDF2）：
 * 1. 组合字符串 = salt + password + salt
 * 2. 第1轮：SHA-256(组合字符串)
 * 3. 第2-10000轮：SHA-256(上一轮哈希 + 组合字符串)
 * 4. 输出最终哈希的16进制表示
 */
std::string hashPassword(const std::string& password, const std::string& secret_salt);
