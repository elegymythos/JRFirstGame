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
#include "WeaponListBridge.hpp"   // 武器系统（C链表+C++桥接层）

/**
 * @brief 二维向量归一化
 * @param v 输入向量
 * @return 单位向量，零向量返回 (0,0)
 */
sf::Vector2f normalize(const sf::Vector2f& v);

/**
 * @brief 计算两点间欧几里得距离
 */
float distance(const sf::Vector2f& a, const sf::Vector2f& b);

/* ======================== 属性系统 ======================== */

/**
 * @struct Attributes
 * @brief 角色六大属性
 *
 * 力量影响剑伤害，敏捷影响刺刀伤害和翻滚，
 * 魔法影响法器伤害，智力预留，生命影响血量上限，幸运预留。
 * 随机生成时使用正态分布（均值10，标准差4），钳制到[1,20]，总和上限80。
 */
struct Attributes {
    int strength = 1;      ///< 力量：剑伤害加成
    int agility = 1;       ///< 敏捷：刺刀伤害加成、翻滚相关
    int magic = 1;         ///< 魔法：法器伤害加成
    int intelligence = 1;  ///< 智力：预留扩展
    int vitality = 1;      ///< 生命：决定 maxHealth = 120 + vitality*8
    int luck = 1;          ///< 幸运：预留扩展

    /// 六项属性之和
    int total() const {
        return strength + agility + magic + intelligence + vitality + luck;
    }
};

/// 随机生成六大属性（正态分布，范围[1,20]，总和上限80）
Attributes generateRandomAttributes();

/// 属性逐项相加
Attributes operator+(const Attributes& a, const Attributes& b);

/* ======================== 职业系统 ======================== */

/**
 * @enum CharacterClass
 * @brief 角色职业枚举
 *
 * 战士：力量+5，装备铁剑，近战扇形攻击
 * 法师：魔法+5，装备火焰法器，远程投射物攻击
 * 刺客：敏捷+5，装备暗影刺刀，近战快速攻击+暴击
 */
enum class CharacterClass { Warrior, Mage, Assassin };

/**
 * @struct ClassConfig
 * @brief 职业配置
 *
 * 定义每个职业的属性加成、允许装备的武器类型和显示名称。
 */
struct ClassConfig {
    CharacterClass type;          ///< 职业类型
    Attributes bonus;             ///< 职业属性加成（如战士力量+5）
    WeaponType allowedWeapon;     ///< 允许装备的武器类型（C枚举）
    const char* name;             ///< 职业英文名称
};

/// 获取指定职业的配置信息
ClassConfig getClassConfig(CharacterClass cls);

/**
 * @brief 计算最终属性 = 基础属性 + 职业加成 + 等级加成
 */
Attributes getTotalAttributes(const Attributes& base, const ClassConfig& cls, const Attributes& levelBonus);

/* ======================== 等级系统 ======================== */

/**
 * @struct LevelData
 * @brief 等级与经验数据
 *
 * 升级经验需求 = 100 * 当前等级。
 * 每升一级，所有属性 +LEVEL_BONUS_PER_ATTR（默认2）。
 */
struct LevelData {
    int level = 1;                          ///< 当前等级
    int experience = 0;                     ///< 当前经验值
    int expToNextLevel = 100;               ///< 升级所需经验
    static constexpr int LEVEL_BONUS_PER_ATTR = 2;  ///< 每级每项属性加成

    /// 增加经验值，自动处理连续升级
    void gainExp(int amount);
    /// 检查是否满足升级条件，满足则升级并返回 true
    bool checkLevelUp();
    /// 获取等级带来的属性加成（每项 = LEVEL_BONUS_PER_ATTR * (level-1)）
    Attributes getLevelBonus() const;
};

/* ======================== 敌人系统 ======================== */

/**
 * @enum EnemyType
 * @brief 敌人类型枚举
 *
 * Slime：绿色，慢速追踪，低血量低伤害
 * SkeletonWarrior：灰白色，中速追踪，中等血量近战
 * SkeletonArcher：暗黄色，保持距离远程射击
 * Giant：棕色，慢速追踪，高血量高伤害
 */
enum class EnemyType { Slime, SkeletonWarrior, SkeletonArcher, Giant };

/**
 * @struct EnemyConfig
 * @brief 敌人配置参数
 *
 * 定义每种敌人的基础属性，由 getEnemyConfig() 返回。
 */
struct EnemyConfig {
    EnemyType type;                ///< 敌人类型
    const char* name;              ///< 显示名称
    int health;                    ///< 基础血量
    int attackDamage;              ///< 攻击伤害
    float speed;                   ///< 移动速度
    float attackRange;             ///< 攻击范围（近战=接触距离，远程=射击距离）
    float attackCooldownMax;       ///< 攻击冷却时间（秒）
    bool isRanged;                 ///< 是否为远程敌人
    int score;                     ///< 击杀获得的分数
};

/// 获取指定敌人类型的配置
EnemyConfig getEnemyConfig(EnemyType type);

/* ======================== 投射物系统 ======================== */

/**
 * @struct Projectile
 * @brief 投射物（法师火球、弓箭手箭矢）
 *
 * 沿直线飞行，超过最大距离后消失。
 * 法师火球命中后可产生爆炸范围杀伤（explosionRadius > 0 时）。
 */
struct Projectile {
    sf::Vector2f position;         ///< 当前位置
    sf::Vector2f direction;        ///< 归一化飞行方向
    float speed = 300.f;           ///< 飞行速度
    int damage = 10;               ///< 伤害值
    float maxDistance = 400.f;     ///< 最大飞行距离
    float traveled = 0.f;          ///< 已飞行距离
    bool fromPlayer = true;        ///< true=玩家发射, false=敌人发射
    sf::Color color = sf::Color::Yellow;  ///< 投射物颜色
    float explosionRadius = 0.f;   ///< 爆炸半径（法师阶段强化后 > 0）
    bool isCrit = false;           ///< 是否暴击（用于伤害数字显示）

    /// 每帧更新位置
    void update(float dt);
    /// 是否超过最大飞行距离
    bool isExpired() const;
    /// 绘制投射物（小圆点）
    void draw(sf::RenderWindow& window) const;
};

/* ======================== 掉落物系统 ======================== */

/**
 * @enum DropType
 * @brief 掉落物类型
 *
 * HealthPotion：回复33%血量
 * StrBoost/AgiBoost/MagBoost/VitBoost：对应属性+2
 * RangeBoost：攻击范围+15
 */
enum class DropType { HealthPotion, StrBoost, AgiBoost, MagBoost, VitBoost, RangeBoost };

/**
 * @struct DropItem
 * @brief 掉落物
 *
 * 敌人被击杀后有概率掉落，10秒后自动消失。
 * 绘制时带有上下浮动动画和光晕效果。
 */
struct DropItem {
    DropType type;                 ///< 掉落物类型
    sf::Vector2f position;         ///< 世界坐标位置
    float lifetime = 10.f;         ///< 剩余存活时间（秒）
    float bobTimer = 0.f;          ///< 上下浮动动画计时器

    /// 每帧更新（减少存活时间，推进浮动动画）
    void update(float dt);
    /// 绘制掉落物（光晕+内圈+中心高亮）
    void draw(sf::RenderWindow& window) const;
    /// 获取掉落物颜色
    sf::Color getColor() const;
    /// 获取掉落物标签文字（如 "HP+"、"STR+"）
    const char* getLabel() const;
};

/* ======================== 游戏实体 ======================== */

/**
 * @class RPGEntity
 * @brief 游戏实体基类
 *
 * 所有游戏对象（玩家、敌人）的公共基类，
 * 包含位置、速度、半径、血量、速度、颜色等基础属性。
 */
class RPGEntity {
public:
    sf::Vector2f position;         ///< 世界坐标位置
    sf::Vector2f velocity;         ///< 当前速度向量
    float radius = 20.f;          ///< 碰撞半径
    int health = 100;             ///< 当前血量
    int maxHealth = 100;          ///< 血量上限
    float speed = 150.f;          ///< 移动速度标量
    sf::Color color = sf::Color::Green;  ///< 渲染颜色

    RPGEntity(sf::Vector2f pos = {400,300});
    virtual ~RPGEntity() = default;
    /// 每帧更新位置：position += velocity * dt
    virtual void update(float dt);
    /// 绘制为圆形
    virtual void draw(sf::RenderWindow& window) const;
    /// 获取碰撞包围盒
    sf::FloatRect getBounds() const;
    /// 是否存活（血量 > 0）
    bool isAlive() const;
};

/**
 * @class Player
 * @brief 玩家角色
 *
 * 继承 RPGEntity，增加以下系统：
 * - 属性与职业：基础属性、职业配置、等级数据
 * - 武器系统：WeaponListBridge 管理武器链表，可装备匹配职业的武器
 * - 战斗属性：攻击伤害、范围、冷却，由武器和属性共同决定
 * - 翻滚系统：Shift 触发，持续0.3秒，期间无敌，冷却0.8秒
 * - 刀光动画：攻击时显示扇形剑气拖尾
 * - 阶段胜利强化：根据职业不同，提升伤害/攻速/暴击/爆炸范围
 * - 分数加成：每50分增加10%全属性和速度，上限200%
 */
class Player : public RPGEntity {
public:
    /* ---- 属性与职业 ---- */
    Attributes baseAttributes;                  ///< 基础属性（随机生成+掉落物增加）
    CharacterClass charClass = CharacterClass::Warrior; ///< 当前职业
    ClassConfig classConfig;                    ///< 职业配置
    LevelData levelData;                        ///< 等级与经验
    std::string username;                       ///< 玩家用户名

    /* ---- 武器系统 ---- */
    WeaponListBridge weaponList;                ///< 武器链表（C链表+C++桥接）
    const WeaponData* equippedWeapon = nullptr; ///< 当前装备的武器指针

    /* ---- 战斗属性 ---- */
    int killCount = 0;                          ///< 击杀数
    int totalScore = 0;                         ///< 累计分数
    float attackCooldown = 0.f;                 ///< 攻击冷却剩余时间
    int attackDamage = 20;                      ///< 攻击伤害（由武器和属性计算）
    float attackRange = 80.f;                   ///< 攻击范围
    float attackCooldownMax = 0.5f;             ///< 攻击冷却时间
    float attackRangeBonus = 0.f;               ///< 掉落物增加的攻击范围

    /* ---- 阶段胜利强化 ---- */
    int victoryStageLevel = 0;                  ///< 当前强化等级（0=未强化，1-5）
    float damageBonus = 0.f;                    ///< 阶段强化伤害加成
    float attackSpeedBonus = 0.f;               ///< 阶段强化攻速加成（减少冷却）
    float projectileExplosionRadius = 0.f;      ///< 法师投射物爆炸半径
    float critChance = 0.f;                     ///< 刺客暴击概率（0-1）
    float critMultiplier = 1.5f;                ///< 暴击伤害倍率

    /* ---- 翻滚系统 ---- */
    bool isRolling = false;                     ///< 是否正在翻滚
    float rollTimer = 0.f;                      ///< 翻滚剩余时间
    float rollDuration = 0.3f;                  ///< 翻滚持续时间
    float rollCooldown = 0.f;                   ///< 翻滚冷却剩余时间
    float rollCooldownMax = 0.8f;               ///< 翻滚冷却时间
    float rollSpeed = 400.f;                    ///< 翻滚速度
    sf::Vector2f rollDirection = {0, 0};        ///< 翻滚方向
    bool isInvincible = false;                  ///< 翻滚期间无敌标记

    /* ---- 刀光动画 ---- */
    float slashTimer = 0.f;                     ///< 刀光剩余时间
    float slashDuration = 0.25f;                ///< 刀光持续时间
    float slashAngle = 0.f;                     ///< 刀光朝向角度（弧度）
    bool slashActive = false;                   ///< 刀光是否激活
    sf::Color slashColor = sf::Color(255, 140, 0);  ///< 刀光颜色（默认橙色）
    mutable bool lastAttackWasCrit = false;     ///< 上次攻击是否暴击（calculateDamage 设置）

    Player(sf::Vector2f pos = {400,300});
    void update(float dt) override;

    /// 攻击冷却是否已结束且不在翻滚中
    bool canAttack() const;
    /// 重置攻击冷却
    void resetAttackCooldown();

    /* ---- 翻滚 ---- */
    /// 翻滚冷却是否已结束且不在翻滚/攻击中
    bool canRoll() const;
    /// 开始翻滚（设置方向、无敌标记、冷却）
    void startRoll(const sf::Vector2f& dir);
    /// 每帧更新翻滚状态
    void updateRoll(float dt);

    /* ---- 刀光 ---- */
    /// 激活刀光动画
    void startSlash(float angle);
    /// 每帧更新刀光状态
    void updateSlash(float dt);

    /**
     * @brief 装备武器
     * @param name 武器名称
     * @return true=装备成功（职业匹配），false=失败
     *
     * 装备后自动调用 recalcCombatStats() 重新计算战斗属性。
     */
    bool equipWeapon(const std::string& name);

    /**
     * @brief 根据武器+属性+阶段强化重新计算战斗属性
     *
     * 计算 attackDamage、attackRange、attackCooldownMax、maxHealth、speed 等。
     */
    void recalcCombatStats();

    /**
     * @brief 应用阶段性胜利强化
     * @param stageLevel 阶段等级 (1-5)
     *
     * 根据职业不同：
     * - 战士：伤害+5/阶段，攻速+15%/阶段，攻击范围增大
     * - 法师：伤害+4/阶段，爆炸半径+30/阶段，攻速+10%/阶段
     * - 刺客：伤害+3/阶段，暴击率+12%/阶段，暴击倍率递增，攻速+25%/阶段
     * 强化后回满血。
     */
    void applyStageBoost(int stageLevel);

    /**
     * @brief 计算当前伤害
     * @return 伤害值（含武器基础+属性加成+阶段强化+分数加成+暴击）
     *
     * 属性加成：剑→力量*0.8, 法器→魔法*0.8, 刺刀→敏捷*0.8
     * 暴击：基础5%，刺客阶段强化后最高80%
     */
    int calculateDamage() const;

    /**
     * @brief 获取最终属性 = 基础 + 职业加成 + 等级加成 + 分数加成
     */
    Attributes getFinalAttributes() const;

    /**
     * @brief 获取分数加成倍率
     * @return 1.0 + min(总分数/50 * 0.1, 1.0)，范围[1.0, 2.0]
     *
     * 每50分增加10%，上限200%（即倍率2.0）
     */
    float getScoreMultiplier() const;

    /// 绘制玩家（圆形+血条，翻滚时半透明）
    void draw(sf::RenderWindow& window) const override;
    /// 绘制玩家并显示名字和武器图标
    void drawWithName(sf::RenderWindow& window, const sf::Font* font = nullptr) const;
    /// 绘制刀光动画（扇形剑气拖尾+剑尖高亮）
    void drawSlash(sf::RenderWindow& window) const;
};

/**
 * @struct DamageNumber
 * @brief 伤害数字显示
 *
 * 普通伤害显示黄色数字，暴击显示红色大字+描边。
 * 数字环绕敌人位置缓慢旋转，2秒后消失。
 */
struct DamageNumber {
    sf::Vector2f position;      ///< 显示位置（敌人位置）
    int damage = 0;             ///< 伤害值
    bool isCrit = false;        ///< 是否暴击
    float timer = 2.0f;         ///< 剩余显示时间（秒）
    float angle = 0.f;          ///< 环绕角度
    float radius = 20.f;        ///< 环绕半径
};

/**
 * @class Enemy
 * @brief 敌人角色
 *
 * 继承 RPGEntity，支持四种类型，各有差异化 AI：
 * - Slime：缓慢直线追踪
 * - SkeletonWarrior：中速直线追踪，近战
 * - SkeletonArcher：保持150-250距离，远程射击
 * - Giant：慢速直线追踪，高血量近战
 * 绘制为45度旋转的正方形（菱形）。
 */
class Enemy : public RPGEntity {
public:
    EnemyType enemyType = EnemyType::Slime;  ///< 敌人类型
    std::string name;                        ///< 显示名称
    int score = 10;                          ///< 击杀分数
    bool isRanged = false;                   ///< 是否远程
    float attackCooldown = 0.f;              ///< 攻击冷却剩余
    float attackCooldownMax = 1.0f;          ///< 攻击冷却时间
    int attackDamage = 5;                    ///< 攻击伤害
    float attackRange = 30.f;                ///< 攻击范围

    Enemy(EnemyType type, sf::Vector2f pos);
    /// 设置追踪目标位置
    void setTarget(const sf::Vector2f& target);
    void update(float dt) override;
    void draw(sf::RenderWindow& window) const override;
    /// 绘制敌人并显示翻译后的名字
    void drawWithName(sf::RenderWindow& window, const sf::Font* font = nullptr) const;

    /// 攻击冷却是否结束
    bool canAttack() const;
    /// 重置攻击冷却
    void resetAttackCooldown();

    /**
     * @brief 根据难度缩放敌人属性
     * @param scale 缩放因子 (1.0=原始, 2.0=双倍)
     *
     * 血量、伤害、分数乘以 scale，速度乘以 sqrt(scale)。
     */
    void scaleStats(float scale);

    bool scoreProcessed = false;  ///< 防止击杀分数重复累加

private:
    sf::Vector2f targetPos;       ///< 追踪目标位置
};

/**
 * @class GameWorld
 * @brief 联机游戏逻辑管理（简化版双人模式）
 *
 * 维护多个 NetPlayer，处理输入、攻击和状态同步。
 * 当前主要用于网络同步的基础框架，完整联机逻辑在 OnlineView 中。
 */
class GameWorld {
public:
    /// 网络玩家数据（简化版，不含完整属性系统）
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
    std::unordered_map<int, NetPlayer> players;  ///< 玩家映射表（id → NetPlayer）
};
