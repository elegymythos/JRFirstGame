/**
 * @file AIController.hpp
 * @brief AI 观测编码与动作解码
 *
 * 定义 AI 与游戏交互的数据结构和转换逻辑：
 * - AIGameState：AI 感知的游戏状态（玩家/敌人/投射物/掉落物/难度等）
 * - AIObservation：将游戏状态编码为 276 维浮点观测向量
 * - AIAction：将动作索引解码为移动/攻击/翻滚/拾取指令
 *
 * 动作空间（12 个离散动作）：
 *   0-8: 9 个移动方向（静止/上/下/左/右/左上/右上/左下/右下）
 *   9:   攻击最近敌人（自动边打边走）
 *   10:  翻滚（朝最近敌人方向）
 *   11:  拾取（朝最近掉落物移动）
 *
 * 观测空间（330 维）：
 *   玩家状态 20 维 + 20 敌人×10 维 + 10 投射物×6 维
 *   + 10 掉落物×4 维 + 8 方向危险度 + 游戏时间 + 难度
 */

#pragma once

#include <vector>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>

/**
 * @struct AIPlayerState
 * @brief AI 感知的玩家状态
 */
struct AIPlayerState {
    float x = 0, y = 0;             ///< 世界坐标位置
    float vx = 0, vy = 0;           ///< 速度分量
    int health = 100, maxHealth = 100; ///< 当前/最大血量
    float speed = 150.f;            ///< 移动速度
    float radius = 20.f;            ///< 碰撞半径
    bool isRolling = false;          ///< 是否翻滚中
    bool isInvincible = false;       ///< 是否无敌
    float attackCooldown = 0.f;      ///< 攻击冷却剩余
    float attackCooldownMax = 0.5f;  ///< 攻击冷却时间
    float rollCooldown = 0.f;       ///< 翻滚冷却剩余
    float rollCooldownMax = 0.8f;    ///< 翻滚冷却时间
    int totalScore = 0;             ///< 累计分数
    int killCount = 0;              ///< 击杀数
    int level = 1;                  ///< 当前等级
    int victoryStage = 0;           ///< 已达阶段
    float attackRange = 80.f;       ///< 攻击范围
    int attackDamage = 20;          ///< 攻击伤害
    float damageBonus = 0.f;        ///< 阶段强化伤害加成
    float attackSpeedBonus = 0.f;   ///< 阶段强化攻速加成
    float critChance = 0.f;         ///< 暴击概率
    int strength = 15;              ///< 力量属性（影响攻击力）
};

/**
 * @struct AIEnemyState
 * @brief AI 感知的敌人状态
 */
struct AIEnemyState {
    float x = 0, y = 0;             ///< 世界坐标位置
    float vx = 0, vy = 0;           ///< 速度分量
    int health = 0, maxHealth = 1;   ///< 当前/最大血量
    float radius = 16.f;            ///< 碰撞半径
    int entityType = 0;             ///< 敌人类型 (0=Slime,1=Warrior,2=Archer,3=Giant)
    int attackDamage = 10;          ///< 攻击伤害
    float attackRange = 30.f;       ///< 攻击范围
    float attackCooldown = 0.f;     ///< 攻击冷却剩余
    float attackCooldownMax = 1.0f;  ///< 攻击冷却时间
    float speed = 80.f;            ///< 移动速度
    bool isRanged = false;          ///< 是否远程
    int score = 15;                 ///< 击杀分数
    bool alive = true;              ///< 是否存活
};

/**
 * @struct AIProjectileState
 * @brief AI 感知的投射物状态
 */
struct AIProjectileState {
    float x = 0, y = 0;             ///< 世界坐标位置
    float dx = 0, dy = 0;           ///< 归一化飞行方向
    float speed = 300.f;            ///< 飞行速度
    int damage = 10;                ///< 伤害值
    bool fromPlayer = true;         ///< true=玩家发射, false=敌人发射
    bool alive = true;              ///< 是否存活
};

/**
 * @struct AIDropState
 * @brief AI 感知的掉落物状态
 */
struct AIDropState {
    float x = 0, y = 0;             ///< 世界坐标位置
    int dropType = 0;               ///< 掉落物类型
    bool alive = true;              ///< 是否存活
};

/**
 * @struct AIGameState
 * @brief AI 完整游戏状态（所有感知信息的聚合）
 */
struct AIGameState {
    AIPlayerState player;                          ///< 玩家状态
    std::vector<AIEnemyState> enemies;             ///< 所有敌人状态
    std::vector<AIProjectileState> projectiles;    ///< 所有投射物状态
    std::vector<AIDropState> drops;                ///< 所有掉落物状态
    float gameTime = 0.f;                         ///< 游戏时间（秒）
    float difficultyScale = 1.f;                  ///< 难度缩放因子
    int currentVictoryStage = 0;                  ///< 当前胜利阶段
};

/**
 * @struct AIAction
 * @brief AI 决策动作
 *
 * 解码后的动作包含：移动方向、是否攻击、攻击目标策略、是否翻滚、是否拾取。
 */
struct AIAction {
    float moveX = 0, moveY = 0;     ///< 移动方向（归一化）
    bool attack = false;            ///< 是否攻击
    int attackTarget = 0;           ///< 攻击目标策略 (0=none, 1=nearest, 2=weakest, 3=ranged_priority)
    bool roll = false;              ///< 是否翻滚
    bool pickup = false;            ///< 是否拾取掉落物
};

/**
 * @class AIObservation
 * @brief AI 观测编码器
 *
 * 将 AIGameState 编码为固定维度的浮点向量供 ONNX 模型推理，
 * 并将模型输出的动作索引解码为 AIAction。
 *
 * 编码方式与 Python 训练环境 rl/game_env.py 完全一致。
 */
class AIObservation {
public:
    static constexpr int MAX_ENEMIES = 20;      ///< 最多编码 20 个敌人
    static constexpr int MAX_PROJECTILES = 10;   ///< 最多编码 10 个投射物
    static constexpr int MAX_DROPS = 10;         ///< 最多编码 10 个掉落物
    static constexpr int DANGER_GRID_DIM = 8;    ///< 8 方向危险度网格
    static constexpr int PLAYER_DIM = 20;        ///< 玩家状态维度
    static constexpr int ENEMY_DIM = 10;          ///< 单个敌人状态维度
    static constexpr int PROJ_DIM = 6;           ///< 单个投射物状态维度
    static constexpr int DROP_DIM = 4;           ///< 单个掉落物状态维度

    /// 观测总维度 = 20 + 20*10 + 10*6 + 10*4 + 8 + 2 = 330
    static constexpr int OBS_DIM =
        PLAYER_DIM
        + MAX_ENEMIES * ENEMY_DIM
        + MAX_PROJECTILES * PROJ_DIM
        + MAX_DROPS * DROP_DIM
        + DANGER_GRID_DIM
        + 2;

    /// 动作数量：9 移动 + 攻击最近 + 翻滚 + 拾取 = 12
    static constexpr int NUM_ACTIONS = 12;

    /// 动作含义：0-8=移动, 9=攻击最近, 10=翻滚, 11=拾取
    static constexpr const char* ACTION_NAMES[] = {
        "stay", "up", "down", "left", "right",
        "up_left", "up_right", "down_left", "down_right",
        "attack_nearest", "roll", "pickup"
    };

    /// 9 个移动方向（与 Python MOVE_DIRS 一致）
    static constexpr std::array<std::pair<float,float>, 9> MOVE_DIRS = {{
        {0.f, 0.f},
        {0.f, -1.f},
        {0.f, 1.f},
        {-1.f, 0.f},
        {1.f, 0.f},
        {-1.f, -1.f},
        {1.f, -1.f},
        {-1.f, 1.f},
        {1.f, 1.f},
    }};

    /**
     * @brief 将游戏状态编码为观测向量
     * @param state AI 感知的游戏状态
     * @return 330 维浮点观测向量，值域 [-1, 1]
     */
    static std::array<float, OBS_DIM> build(const AIGameState& state);

    /**
     * @brief 将动作索引解码为 AIAction
     * @param action 动作索引 [0, 11]
     * @return 解码后的动作结构
     *
     * 0-8: 移动方向, 9: 攻击最近, 10: 翻滚, 11: 拾取
     */
    static AIAction decodeAction(int action);

private:
    static float clamp(float v, float lo, float hi) {
        return std::max(lo, std::min(hi, v));
    }
    static void normalizeAngle(float dx, float dy, float& outX, float& outY) {
        float len = std::sqrt(dx*dx + dy*dy);
        if (len < 1e-8f) { outX = 0.f; outY = 0.f; return; }
        outX = dx / len; outY = dy / len;
    }
};
