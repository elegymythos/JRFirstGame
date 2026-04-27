#pragma once
#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <sqlite3.h>
#include "GameLogic.hpp"  // Attributes, CharacterClass

/**
 * @brief 排行榜记录条目（含等级）
 */
struct RankRecord {
    std::string username;
    int level;
    int score;
};

/**
 * @brief 角色数据
 */
struct CharacterData {
    std::string username;
    int slot = 0;            // 角色槽位 (0-2)
    CharacterClass classType;
    int level;
    int experience;
    int score = 0;           // 当前分数
    float gameTime = 0.f;    // 游戏时间（不含暂停）
    bool victoryShown = false;  // 是否已显示过阶段性胜利
    Attributes attributes;
};

/**
 * @class Database
 * @brief 封装 SQLite 数据库操作，管理用户账户、多角色和排行榜数据
 */
class Database {
public:
    Database();
    ~Database();

    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // 用户操作
    bool userExists(const std::string& username);
    bool addUser(const std::string& username, const std::string& salt, const std::string& hash);
    std::optional<std::pair<std::string, std::string>> getCredentials(const std::string& username);

    // 角色操作（多角色支持）
    bool hasCharacter(const std::string& username);
    int getCharacterCount(const std::string& username);
    void saveCharacter(const CharacterData& data);
    std::optional<CharacterData> loadCharacter(const std::string& username, int slot);
    std::vector<CharacterData> loadAllCharacters(const std::string& username);
    void deleteCharacter(const std::string& username, int slot);
    
    // 选中的角色槽位
    void saveSelectedSlot(const std::string& username, int slot);
    int getSelectedSlot(const std::string& username);

    // 排行榜操作
    void saveRanking(const std::string& username, int level, int score);
    std::vector<RankRecord> loadRankings();

    // 旧接口兼容
    void saveScore(const std::string& username, int score);

private:
    sqlite3* db = nullptr;
    void initTables();
};
