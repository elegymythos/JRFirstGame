#include "GameLogic.hpp"

sf::Vector2f normalize(const sf::Vector2f& v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y);
    if (len == 0) return sf::Vector2f(0,0);
    return v / len;
}

// ---------- RPGEntity ----------
RPGEntity::RPGEntity(sf::Vector2f pos) : position(pos) {}

void RPGEntity::update(float dt) {
    position += velocity * dt;
}

void RPGEntity::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(radius);
    shape.setPosition(position - sf::Vector2f(radius, radius));
    shape.setFillColor(color);
    window.draw(shape);
}

sf::FloatRect RPGEntity::getBounds() const {
    return {position - sf::Vector2f(radius,radius), {radius*2, radius*2}};
}

bool RPGEntity::isAlive() const {
    return health > 0;
}

// ---------- Player ----------
Player::Player(sf::Vector2f pos) : RPGEntity(pos) {
    color = sf::Color::Cyan;
    radius = 18.f;
}

void Player::update(float dt, sf::Vector2u windowSize) {
    if (attackCooldown > 0) attackCooldown -= dt;
    RPGEntity::update(dt);
    float left = radius, right = windowSize.x - radius;
    float top = radius, bottom = windowSize.y - radius;
    position.x = std::clamp(position.x, left, right);
    position.y = std::clamp(position.y, top, bottom);
}

bool Player::canAttack() const {
    return attackCooldown <= 0;
}

void Player::resetAttackCooldown() {
    attackCooldown = attackCooldownMax;
}

void Player::draw(sf::RenderWindow& window) const {
    RPGEntity::draw(window);
    sf::RectangleShape bg({50, 8});
    bg.setFillColor(sf::Color::Red);
    bg.setPosition(sf::Vector2f(position.x - 25, position.y - radius - 10));
    window.draw(bg);
    sf::RectangleShape bar({50.f * health / maxHealth, 8});
    bar.setFillColor(sf::Color::Green);
    bar.setPosition(sf::Vector2f(position.x - 25, position.y - radius - 10));
    window.draw(bar);
}

// ---------- Enemy ----------
Enemy::Enemy(sf::Vector2f pos) : RPGEntity(pos) {
    color = sf::Color::Red;
    radius = 16.f;
    health = 40;
    maxHealth = 40;
    speed = 80.f;
    targetPos = pos;
}

void Enemy::setTarget(const sf::Vector2f& target) {
    targetPos = target;
}

void Enemy::update(float dt) {
    sf::Vector2f dir = targetPos - position;
    if (dir != sf::Vector2f(0,0)) dir = normalize(dir);
    velocity = dir * speed;
    RPGEntity::update(dt);
}

void Enemy::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(radius);
    shape.setPosition(position - sf::Vector2f(radius, radius));
    shape.setFillColor(color);
    window.draw(shape);
    float barWidth = radius * 2;
    float barHeight = 5.f;
    float barX = position.x - radius;
    float barY = position.y - radius - 10.f;
    sf::RectangleShape bg(sf::Vector2f(barWidth, barHeight));
    bg.setFillColor(sf::Color::Red);
    bg.setPosition(sf::Vector2f(barX, barY));
    window.draw(bg);
    float healthPercent = static_cast<float>(health) / maxHealth;
    sf::RectangleShape bar(sf::Vector2f(barWidth * healthPercent, barHeight));
    bar.setFillColor(sf::Color::Green);
    bar.setPosition(sf::Vector2f(barX, barY));
    window.draw(bar);
}

// ---------- GameWorld ----------
void GameWorld::addPlayer(int id, const sf::Vector2f& startPos) {
    players[id] = {startPos, {}, 100, 100, 0.f, 20, 40.f};
}

void GameWorld::removePlayer(int id) {
    players.erase(id);
}

void GameWorld::setPlayerInput(int id, const PlayerInput& input) {
    if (players.count(id)) players[id].input = input;
}

void GameWorld::attack(int attackerId, int targetId) {
    if (!players.count(attackerId) || !players.count(targetId)) return;
    auto& attacker = players[attackerId];
    auto& target = players[targetId];
    if (attacker.attackCooldown > 0) return;
    float dist = std::hypot(attacker.position.x - target.position.x,
                            attacker.position.y - target.position.y);
    if (dist <= attacker.attackRange) {
        target.health -= attacker.attackDamage;
        attacker.attackCooldown = NetPlayer::attackCooldownMax;
    }
}

void GameWorld::update(float dt) {
    const float speed = 200.f;
    for (auto& [id, p] : players) {
        if (p.attackCooldown > 0) p.attackCooldown -= dt;
    }
    for (auto& [id, p] : players) {
        sf::Vector2f move(0,0);
        if (p.input.up)    move.y -= 1;
        if (p.input.down)  move.y += 1;
        if (p.input.left)  move.x -= 1;
        if (p.input.right) move.x += 1;
        if (move.x != 0 || move.y != 0) {
            float len = std::sqrt(move.x*move.x + move.y*move.y);
            move = move / len;
            p.position += move * speed * dt;
        }
        p.position.x = std::clamp(p.position.x, 0.f, 800.f);
        p.position.y = std::clamp(p.position.y, 0.f, 600.f);
    }
}

GameState GameWorld::getState() const {
    GameState state;
    for (const auto& [id, p] : players) {
        GameState::PlayerState ps;
        ps.position = p.position;
        ps.health = p.health;
        ps.maxHealth = p.maxHealth;
        state.players.push_back(ps);
    }
    return state;
}