#include "Database.hpp"
#include <iostream>

Database::Database() {
    if (sqlite3_open("game.db", &db) != SQLITE_OK) {
        db = nullptr;
        std::cerr << "无法打开/创建数据库文件 game.db" << std::endl;
    } else {
        initTables();
    }
}

Database::~Database() {
    if (db) sqlite3_close(db);
}

void Database::initTables() {
    const char* createUsers = R"(
        CREATE TABLE IF NOT EXISTS users (
            username TEXT PRIMARY KEY,
            salt TEXT NOT NULL,
            hash TEXT NOT NULL
        );
    )";
    const char* createRankings = R"(
        CREATE TABLE IF NOT EXISTS rankings (
            username TEXT PRIMARY KEY,
            level INTEGER NOT NULL DEFAULT 1,
            score INTEGER NOT NULL DEFAULT 0
        );
    )";
    const char* createCharacters = R"(
        CREATE TABLE IF NOT EXISTS characters (
            username TEXT NOT NULL,
            slot INTEGER NOT NULL DEFAULT 0,
            class_type INTEGER NOT NULL,
            level INTEGER NOT NULL DEFAULT 1,
            experience INTEGER NOT NULL DEFAULT 0,
            score INTEGER NOT NULL DEFAULT 0,
            game_time REAL NOT NULL DEFAULT 0.0,
            victory_shown INTEGER NOT NULL DEFAULT 0,
            strength INTEGER NOT NULL,
            agility INTEGER NOT NULL,
            magic INTEGER NOT NULL,
            intelligence INTEGER NOT NULL,
            vitality INTEGER NOT NULL,
            luck INTEGER NOT NULL,
            PRIMARY KEY (username, slot)
        );
    )";
    const char* createUserSettings = R"(
        CREATE TABLE IF NOT EXISTS user_settings (
            username TEXT PRIMARY KEY,
            selected_slot INTEGER NOT NULL DEFAULT 0
        );
    )";

    sqlite3_exec(db, createUsers, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createRankings, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createCharacters, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createUserSettings, nullptr, nullptr, nullptr);

    // 迁移：如果旧rankings表没有level列，添加它
    sqlite3_exec(db, "ALTER TABLE rankings ADD COLUMN level INTEGER NOT NULL DEFAULT 1;",
                 nullptr, nullptr, nullptr);
    
    // 迁移：如果旧characters表没有score列，添加它
    sqlite3_exec(db, "ALTER TABLE characters ADD COLUMN score INTEGER NOT NULL DEFAULT 0;",
                 nullptr, nullptr, nullptr);
    
    // 迁移：添加game_time和victory_shown列
    sqlite3_exec(db, "ALTER TABLE characters ADD COLUMN game_time REAL NOT NULL DEFAULT 0.0;",
                 nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE characters ADD COLUMN victory_shown INTEGER NOT NULL DEFAULT 0;",
                 nullptr, nullptr, nullptr);
}

/* ======================== 用户操作 ======================== */

bool Database::userExists(const std::string& username) {
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

bool Database::addUser(const std::string& username, const std::string& salt, const std::string& hash) {
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO users (username, salt, hash) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash.c_str(), -1, SQLITE_STATIC);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<std::pair<std::string, std::string>> Database::getCredentials(const std::string& username) {
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT salt, hash FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* saltText = sqlite3_column_text(stmt, 0);
        const unsigned char* hashText = sqlite3_column_text(stmt, 1);
        if (!saltText || !hashText) { sqlite3_finalize(stmt); return std::nullopt; }
        std::string salt = reinterpret_cast<const char*>(saltText);
        std::string hash = reinterpret_cast<const char*>(hashText);
        sqlite3_finalize(stmt);
        return std::make_pair(salt, hash);
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

/* ======================== 角色操作（多角色） ======================== */

bool Database::hasCharacter(const std::string& username) {
    return getCharacterCount(username) > 0;
}

int Database::getCharacterCount(const std::string& username) {
    if (!db) return 0;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM characters WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

void Database::saveCharacter(const CharacterData& data) {
    if (!db) return;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        INSERT INTO characters (username, slot, class_type, level, experience, score, game_time, victory_shown, strength, agility, magic, intelligence, vitality, luck)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(username, slot) DO UPDATE SET
            class_type = excluded.class_type,
            level = excluded.level,
            experience = excluded.experience,
            score = excluded.score,
            game_time = excluded.game_time,
            victory_shown = excluded.victory_shown,
            strength = excluded.strength,
            agility = excluded.agility,
            magic = excluded.magic,
            intelligence = excluded.intelligence,
            vitality = excluded.vitality,
            luck = excluded.luck;
    )";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, data.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, data.slot);
    sqlite3_bind_int(stmt, 3, static_cast<int>(data.classType));
    sqlite3_bind_int(stmt, 4, data.level);
    sqlite3_bind_int(stmt, 5, data.experience);
    sqlite3_bind_int(stmt, 6, data.score);
    sqlite3_bind_double(stmt, 7, data.gameTime);
    sqlite3_bind_int(stmt, 8, data.victoryStage);
    sqlite3_bind_int(stmt, 9, data.attributes.strength);
    sqlite3_bind_int(stmt, 10, data.attributes.agility);
    sqlite3_bind_int(stmt, 11, data.attributes.magic);
    sqlite3_bind_int(stmt, 12, data.attributes.intelligence);
    sqlite3_bind_int(stmt, 13, data.attributes.vitality);
    sqlite3_bind_int(stmt, 14, data.attributes.luck);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::optional<CharacterData> Database::loadCharacter(const std::string& username, int slot) {
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        SELECT class_type, level, experience, score, game_time, victory_shown, strength, agility, magic, intelligence, vitality, luck
        FROM characters WHERE username = ? AND slot = ?;
    )";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, slot);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CharacterData data;
        data.username = username;
        data.slot = slot;
        data.classType = static_cast<CharacterClass>(sqlite3_column_int(stmt, 0));
        data.level = sqlite3_column_int(stmt, 1);
        data.experience = sqlite3_column_int(stmt, 2);
        data.score = sqlite3_column_int(stmt, 3);
        data.gameTime = static_cast<float>(sqlite3_column_double(stmt, 4));
        data.victoryStage = sqlite3_column_int(stmt, 5);
        data.attributes.strength = sqlite3_column_int(stmt, 6);
        data.attributes.agility = sqlite3_column_int(stmt, 7);
        data.attributes.magic = sqlite3_column_int(stmt, 8);
        data.attributes.intelligence = sqlite3_column_int(stmt, 9);
        data.attributes.vitality = sqlite3_column_int(stmt, 10);
        data.attributes.luck = sqlite3_column_int(stmt, 11);
        sqlite3_finalize(stmt);
        return data;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<CharacterData> Database::loadAllCharacters(const std::string& username) {
    std::vector<CharacterData> result;
    if (!db) return result;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        SELECT slot, class_type, level, experience, score, game_time, victory_shown, strength, agility, magic, intelligence, vitality, luck
        FROM characters WHERE username = ? ORDER BY slot;
    )";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CharacterData data;
        data.username = username;
        data.slot = sqlite3_column_int(stmt, 0);
        data.classType = static_cast<CharacterClass>(sqlite3_column_int(stmt, 1));
        data.level = sqlite3_column_int(stmt, 2);
        data.experience = sqlite3_column_int(stmt, 3);
        data.score = sqlite3_column_int(stmt, 4);
        data.gameTime = static_cast<float>(sqlite3_column_double(stmt, 5));
        data.victoryStage = sqlite3_column_int(stmt, 6);
        data.attributes.strength = sqlite3_column_int(stmt, 7);
        data.attributes.agility = sqlite3_column_int(stmt, 8);
        data.attributes.magic = sqlite3_column_int(stmt, 9);
        data.attributes.intelligence = sqlite3_column_int(stmt, 10);
        data.attributes.vitality = sqlite3_column_int(stmt, 11);
        data.attributes.luck = sqlite3_column_int(stmt, 12);
        result.push_back(data);
    }
    sqlite3_finalize(stmt);
    return result;
}

void Database::deleteCharacter(const std::string& username, int slot) {
    if (!db) return;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM characters WHERE username = ? AND slot = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, slot);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

/* ======================== 排行榜操作 ======================== */

void Database::saveRanking(const std::string& username, int level, int score) {
    if (!db) return;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        INSERT INTO rankings (username, level, score) VALUES (?, ?, ?)
        ON CONFLICT(username) DO UPDATE SET
            level = MAX(level, excluded.level),
            score = MAX(score, excluded.score);
    )";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, level);
    sqlite3_bind_int(stmt, 3, score);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<RankRecord> Database::loadRankings() {
    std::vector<RankRecord> records;
    if (!db) return records;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT username, level, score FROM rankings ORDER BY level DESC, score DESC LIMIT 10;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return records;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RankRecord r;
        const unsigned char* nameText = sqlite3_column_text(stmt, 0);
        if (!nameText) continue;
        r.username = reinterpret_cast<const char*>(nameText);
        r.level = sqlite3_column_int(stmt, 1);
        r.score = sqlite3_column_int(stmt, 2);
        records.push_back(r);
    }
    sqlite3_finalize(stmt);
    return records;
}

void Database::saveScore(const std::string& username, int score) {
    saveRanking(username, 1, score);
}

void Database::saveSelectedSlot(const std::string& username, int slot) {
    if (!db) return;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO user_settings (username, selected_slot) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, slot);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int Database::getSelectedSlot(const std::string& username) {
    if (!db) return 0;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT selected_slot FROM user_settings WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    int slot = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        slot = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return slot;
}
