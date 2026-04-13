#pragma once
#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <sqlite3.h>

/**
 * @brief 排行榜记录条目
 */
struct ScoreRecord {
    std::string username;
    int score;
};

/**
 * @class Database
 * @brief 封装 SQLite 数据库操作，管理用户账户和排行榜数据
 */
class Database {
public:
    Database();
    ~Database();

    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool userExists(const std::string& username);
    bool addUser(const std::string& username, const std::string& salt, const std::string& hash);
    std::optional<std::pair<std::string, std::string>> getCredentials(const std::string& username);
    void saveScore(const std::string& username, int score);
    std::vector<ScoreRecord> loadRankings();

private:
    sqlite3* db = nullptr;
    void initTables();
};