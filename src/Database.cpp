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
            score INTEGER NOT NULL
        );
    )";
    sqlite3_exec(db, createUsers, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createRankings, nullptr, nullptr, nullptr);
}

bool Database::userExists(const std::string& username) {
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
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT salt, hash FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        sqlite3_finalize(stmt);
        return std::make_pair(salt, hash);
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

void Database::saveScore(const std::string& username, int score) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"(
        INSERT INTO rankings (username, score) VALUES (?, ?)
        ON CONFLICT(username) DO UPDATE SET score = MAX(score, excluded.score);
    )";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, score);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<ScoreRecord> Database::loadRankings() {
    std::vector<ScoreRecord> records;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT username, score FROM rankings ORDER BY score DESC LIMIT 10;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return records;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ScoreRecord r;
        r.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.score = sqlite3_column_int(stmt, 1);
        records.push_back(r);
    }
    sqlite3_finalize(stmt);
    return records;
}