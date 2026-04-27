#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include "Network.hpp"           // 需要 PlayerInput 和 GameState
#include "WeaponList.hpp"         // 武器系统（C++链表）

// 向量归一化辅助函数
sf::Vector2f normalize(const sf::Vector2f& v);

// 两点间距离
float distance(const sf::Vector2f& a, const sf::Vector2f& b);

/* ======================== 属性系统 ======================== */

/**
 * @brief 六大属性结构体
 */
struct Attributes {
    int strength = 1;      // 力量
    int agility = 1;       // 敏捷
    int magic = 1;         // 魔法
    int intelligence = 1;  // 智力
    int vitality = 1;      // 生命
    int luck = 1;          // 幸运

    int total() const {
        return strength + agility + magic + intelligence + vitality + luck;
    }
};

/**
 * @brief 随机生成六大属性（正态分布，范围[1,20]，总和上限80）
 */
Attributes generateRandomAttributes();

/**
 * @brief 属性相加
 */
Attributes operator+(const Attributes& a, const Attributes& b);

/* ======================== 职业系统 ======================== */

/**
 * @brief 角色职业枚举
 */
enum class CharacterClass { Warrior, Mage, Assassin };

/**
 * @brief 职业配置结构体
 */
struct ClassConfig {
    CharacterClass type;
    Attributes bonus;           // 职业属性加成
    WeaponType allowedWeapon;   // 允许的武器类型
    const char* name;           // 职业名称
};

/**
 * @brief 获取职业配置
 */
ClassConfig getClassConfig(CharacterClass cls);

/**
 * @brief 计算最终属性 = 基础 + 职业加成 + 等级加成
 */
Attributes getTotalAttributes(const Attributes& base, const ClassConfig& cls, const Attributes& levelBonus);

/* ======================== 等级系统 ======================== */

/**
 * @brief 等级数据结构体
 */
struct LevelData {
    int level = 1;
    int experience = 0;
    int expToNextLevel = 100;
    static constexpr int LEVEL_BONUS_PER_ATTR = 2;

    void gainExp(int amount);
    bool checkLevelUp();
    Attributes getLevelBonus() const;
};

/* ======================== 敌人系统 ======================== */

/**
 * @brief 敌人类型枚举
 */
enum class EnemyType { Slime, SkeletonWarrior, SkeletonArcher, Giant };

/**
 * @brief 敌人配置结构体
 */
struct EnemyConfig {
    EnemyType type;
    const char* name;
    int health;
    int attackDamage;
    float speed;
    float attackRange;
    float attackCooldownMax;
    bool isRanged;
    int score;  // 击杀分数
};

/**
 * @brief 获取敌人配置
 */
EnemyConfig getEnemyConfig(EnemyType type);

/* ======================== 投射物系统 ======================== */

/**
 * @brief 投射物结构体
 */
struct Projectile {
    sf::Vector2f position;
    sf::Vector2f direction;  // 归一化方向
    float speed = 300.f;
    int damage = 10;
    float maxDistance = 400.f;
    float traveled = 0.f;
    bool fromPlayer = true;  // 玩家发射 or 敌人发射

    void update(float dt);
    bool isExpired() const;
    void draw(sf::RenderWindow& window) const;
};

/* ======================== 游戏实体 ======================== */

/**
 * @class RPGEntity
 * @brief 游戏实体基类
 */
class RPGEntity {
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius = 20.f;
    int health = 100;
    int maxHealth = 100;
    float speed = 150.f;
    sf::Color color = sf::Color::Green;

    RPGEntity(sf::Vector2f pos = {400,300});
    virtual ~RPGEntity() = default;
    virtual void update(float dt);
    virtual void draw(sf::RenderWindow& window) const;
    sf::FloatRect getBounds() const;
    bool isAlive() const;
};

/**
 * @class Player
 * @brief 玩家角色类
 */
class Player : public RPGEntity {
public:
    // 属性与职业
    Attributes baseAttributes;
    CharacterClass charClass = CharacterClass::Warrior;
    ClassConfig classConfig;
    LevelData levelData;
    std::string username;

    // 武器系统
    WeaponManager weaponManager;
    const WeaponData* equippedWeapon = nullptr;

    // 战斗属性
    int killCount = 0;
    int totalScore = 0;
    float attackCooldown = 0.f;
    int attackDamage = 20;
    float attackRange = 80.f;
    float attackCooldownMax = 0.5f;

    Player(sf::Vector2f pos = {400,300});
    void update(float dt);
    bool canAttack() const;
    void resetAttackCooldown();

    /**
     * @brief 装备武器（校验职业匹配）
     * @return true=成功
     */
    bool equipWeapon(const std::string& name);

    /**
     * @brief 根据武器+属性重新计算战斗属性
     */
    void recalcCombatStats();

    /**
     * @brief 计算当前伤害（含武器+属性加成）
     */
    int calculateDamage() const;

    /**
     * @brief 获取最终属性（基础+职业+等级）
     */
    Attributes getFinalAttributes() const;

    void draw(sf::RenderWindow& window) const override;
    void drawWithName(sf::RenderWindow& window, const sf::Font* font = nullptr) const;
};

/**
 * @class Enemy
 * @brief 敌人角色类，支持四种类型与差异化AI
 */
class Enemy : public RPGEntity {
public:
    EnemyType enemyType = EnemyType::Slime;
    std::string name;
    int score = 10;
    bool isRanged = false;
    float attackCooldown = 0.f;
    float attackCooldownMax = 1.0f;
    int attackDamage = 5;
    float attackRange = 30.f;

    Enemy(EnemyType type, sf::Vector2f pos);
    void setTarget(const sf::Vector2f& target);
    void update(float dt) override;
    void draw(sf::RenderWindow& window) const override;
    void drawWithName(sf::RenderWindow& window, const sf::Font* font = nullptr) const;

    bool canAttack() const;
    void resetAttackCooldown();

    /**
     * @brief 根据难度缩放敌人属性
     * @param scale 缩放因子 (1.0=原始, 2.0=双倍)
     */
    void scaleStats(float scale);

private:
    sf::Vector2f targetPos;
};

/**
 * @class GameWorld
 * @brief 联机游戏逻辑管理（双人闯关模式）
 */
class GameWorld {
public:
    struct NetPlayer {
        sf::Vector2f position;
        PlayerInput input;
        int health = 100;
        int maxHealth = 100;
        float attackCooldown = 0.f;
        static constexpr float attackCooldownMax = 0.5f;
        int attackDamage = 20;
        float attackRange = 40.f;
    };

    void addPlayer(int id, const sf::Vector2f& startPos);
    void removePlayer(int id);
    void setPlayerInput(int id, const PlayerInput& input);
    void attack(int attackerId, int targetId);
    void update(float dt);
    GameState getState() const;

private:
    std::unordered_map<int, NetPlayer> players;
};
