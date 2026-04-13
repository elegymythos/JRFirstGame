#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include "Network.hpp"   // 需要 PlayerInput 和 GameState

// 向量归一化辅助函数
sf::Vector2f normalize(const sf::Vector2f& v);

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
 * @brief 玩家角色类（单机模式）
 */
class Player : public RPGEntity {
public:
    int killCount = 0;
    float attackCooldown = 0.f;
    static constexpr float attackCooldownMax = 0.5f;
    int attackDamage = 20;
    float attackRange = 80.f;

    Player(sf::Vector2f pos = {400,300});
    void update(float dt, sf::Vector2u windowSize);
    bool canAttack() const;
    void resetAttackCooldown();
    void draw(sf::RenderWindow& window) const override;
};

/**
 * @class Enemy
 * @brief 敌人角色类，会追逐玩家
 */
class Enemy : public RPGEntity {
private:
    sf::Vector2f targetPos;
public:
    Enemy(sf::Vector2f pos);
    void setTarget(const sf::Vector2f& target);
    void update(float dt) override;
    void draw(sf::RenderWindow& window) const override;
};

/**
 * @class GameWorld
 * @brief 联机游戏逻辑管理，负责玩家移动、攻击、状态更新
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