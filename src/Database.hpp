#pragma once
#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <sqlite3.h>
#include "GameLogic.hpp"  // Attributes, CharacterClass

/**
 * @struct RankRecord
 * @brief 排行榜单条记录
 *
 * 包含用户名、等级和分数，按等级降序+分数降序排列，
 * 最多返回前10名。
 */
struct RankRecord {
    std::string username;  ///< 用户名
    int level;             ///< 角色等级
    int score;             ///< 最高分数
};

/**
 * @struct CharacterData
 * @brief 角色完整数据
 *
 * 存储一个角色的所有持久化信息，对应数据库 characters 表的一行。
 * 每个用户最多3个角色，通过 (username, slot) 联合主键标识。
 */
struct CharacterData {
    std::string username;              ///< 所属用户名
    int slot = 0;                      ///< 角色槽位编号 (0-2)
    CharacterClass classType;          ///< 职业类型（战士/法师/刺客）
    int level;                         ///< 当前等级
    int experience;                    ///< 当前经验值
    int score = 0;                     ///< 累计分数
    float gameTime = 0.f;              ///< 游戏时长（秒，不含暂停时间）
    int victoryStage = 0;              ///< 已达到的最高胜利阶段 (0=未达到, 1-5)
    Attributes attributes;             ///< 六大基础属性
};

/**
 * @class Database
 * @brief SQLite 数据库操作封装
 *
 * 管理四张表：
 * - users：用户账户（用户名、盐值、密码哈希）
 * - characters：角色数据（多角色，每用户最多3个槽位）
 * - rankings：排行榜（按等级和分数排名）
 * - user_settings：用户设置（选中的角色槽位）
 *
 * 所有查询使用参数化语句防止 SQL 注入。
 * 数据库文件为同目录下的 game.db，程序启动时自动创建。
 */
class Database {
public:
    Database();
    ~Database();

    // 禁止拷贝（sqlite3* 不应共享）
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /* ---- 用户操作 ---- */

    /// 检查用户名是否已存在
    bool userExists(const std::string& username);
    /// 添加新用户（存储盐值和哈希），成功返回 true
    bool addUser(const std::string& username, const std::string& salt, const std::string& hash);
    /// 获取用户的盐值和哈希，用户不存在则返回 nullopt
    std::optional<std::pair<std::string, std::string>> getCredentials(const std::string& username);

    /* ---- 角色操作（多角色支持） ---- */

    /// 检查用户是否至少有一个角色
    bool hasCharacter(const std::string& username);
    /// 获取用户的角色数量
    int getCharacterCount(const std::string& username);
    /// 保存角色数据（INSERT OR REPLACE，按 username+slot 匹配）
    void saveCharacter(const CharacterData& data);
    /// 加载指定槽位的角色，不存在则返回 nullopt
    std::optional<CharacterData> loadCharacter(const std::string& username, int slot);
    /// 加载用户的所有角色，按槽位排序
    std::vector<CharacterData> loadAllCharacters(const std::string& username);
    /// 删除指定槽位的角色
    void deleteCharacter(const std::string& username, int slot);

    /* ---- 选中角色槽位 ---- */

    /// 保存用户当前选中的角色槽位
    void saveSelectedSlot(const std::string& username, int slot);
    /// 获取用户当前选中的角色槽位，默认0
    int getSelectedSlot(const std::string& username);

    /* ---- 排行榜操作 ---- */

    /// 保存排行榜记录（取等级和分数的最大值更新）
    void saveRanking(const std::string& username, int level, int score);
    /// 加载排行榜前10名（按等级降序、分数降序）
    std::vector<RankRecord> loadRankings();

    /// 旧接口兼容：仅保存分数（等级默认1）
    void saveScore(const std::string& username, int score);

private:
    sqlite3* db = nullptr;  ///< SQLite 数据库句柄
    /// 初始化数据库表结构（含旧表迁移）
    void initTables();
};
