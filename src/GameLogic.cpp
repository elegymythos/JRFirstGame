#include "GameLogic.hpp"
#include "I18n.hpp"

/* ======================== 辅助函数 ======================== */

sf::Vector2f normalize(const sf::Vector2f& v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y);
    if (len == 0) return sf::Vector2f(0,0);
    return v / len;
}

float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

/* ======================== 属性系统 ======================== */

Attributes operator+(const Attributes& a, const Attributes& b) {
    return {
        a.strength + b.strength,
        a.agility + b.agility,
        a.magic + b.magic,
        a.intelligence + b.intelligence,
        a.vitality + b.vitality,
        a.luck + b.luck
    };
}

Attributes generateRandomAttributes() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<float> dist(10.0f, 4.0f);  // 均值10, 标准差4

    Attributes attr;
    attr.strength     = static_cast<int>(std::round(dist(gen)));
    attr.agility      = static_cast<int>(std::round(dist(gen)));
    attr.magic        = static_cast<int>(std::round(dist(gen)));
    attr.intelligence = static_cast<int>(std::round(dist(gen)));
    attr.vitality     = static_cast<int>(std::round(dist(gen)));
    attr.luck         = static_cast<int>(std::round(dist(gen)));

    // 钳制到 [1, 20]
    auto clampAttr = [](int v) -> int { return std::clamp(v, 1, 20); };
    attr.strength     = clampAttr(attr.strength);
    attr.agility      = clampAttr(attr.agility);
    attr.magic        = clampAttr(attr.magic);
    attr.intelligence = clampAttr(attr.intelligence);
    attr.vitality     = clampAttr(attr.vitality);
    attr.luck         = clampAttr(attr.luck);

    // 总和上限80，超出时等比缩放
    const int TOTAL_CAP = 80;
    int total = attr.total();
    if (total > TOTAL_CAP) {
        float scale = static_cast<float>(TOTAL_CAP) / total;
        attr.strength     = std::max(1, static_cast<int>(attr.strength * scale));
        attr.agility      = std::max(1, static_cast<int>(attr.agility * scale));
        attr.magic        = std::max(1, static_cast<int>(attr.magic * scale));
        attr.intelligence = std::max(1, static_cast<int>(attr.intelligence * scale));
        attr.vitality     = std::max(1, static_cast<int>(attr.vitality * scale));
        attr.luck         = std::max(1, static_cast<int>(attr.luck * scale));
    }

    return attr;
}

/* ======================== 职业系统 ======================== */

ClassConfig getClassConfig(CharacterClass cls) {
    switch (cls) {
        case CharacterClass::Warrior:
            return {CharacterClass::Warrior,
                    Attributes{5, 0, 0, 0, 0, 0},  // 力量+5
                    WeaponType::Sword, "Warrior"};
        case CharacterClass::Mage:
            return {CharacterClass::Mage,
                    Attributes{0, 0, 5, 0, 0, 0},  // 魔法+5
                    WeaponType::Artifact, "Mage"};
        case CharacterClass::Assassin:
            return {CharacterClass::Assassin,
                    Attributes{0, 5, 0, 0, 0, 0},  // 敏捷+5
                    WeaponType::Dagger, "Assassin"};
    }
    return {CharacterClass::Warrior, {}, WeaponType::Sword, "Warrior"};
}

Attributes getTotalAttributes(const Attributes& base, const ClassConfig& cls, const Attributes& levelBonus) {
    return base + cls.bonus + levelBonus;
}

/* ======================== 等级系统 ======================== */

void LevelData::gainExp(int amount) {
    experience += amount;
    while (checkLevelUp()) {}
}

bool LevelData::checkLevelUp() {
    if (experience >= expToNextLevel) {
        experience -= expToNextLevel;
        level++;
        expToNextLevel = 100 * level;  // 线性增长
        return true;
    }
    return false;
}

Attributes LevelData::getLevelBonus() const {
    int bonus = LEVEL_BONUS_PER_ATTR * (level - 1);
    return {bonus, bonus, bonus, bonus, bonus, bonus};
}

/* ======================== 敌人配置 ======================== */

EnemyConfig getEnemyConfig(EnemyType type) {
    switch (type) {
        case EnemyType::Slime:
            return {EnemyType::Slime, "Slime", 30, 5, 40.f, 30.f, 1.0f, false, 10};
        case EnemyType::SkeletonWarrior:
            return {EnemyType::SkeletonWarrior, "Sk.Warrior", 80, 15, 80.f, 50.f, 0.8f, false, 30};
        case EnemyType::SkeletonArcher:
            return {EnemyType::SkeletonArcher, "Sk.Archer", 50, 12, 70.f, 250.f, 1.2f, true, 25};
        case EnemyType::Giant:
            return {EnemyType::Giant, "Giant", 300, 40, 25.f, 60.f, 2.0f, false, 80};
    }
    return {EnemyType::Slime, "Slime", 30, 5, 40.f, 30.f, 1.0f, false, 10};
}

/* ======================== 投射物系统 ======================== */

void Projectile::update(float dt) {
    sf::Vector2f movement = direction * speed * dt;
    position += movement;
    traveled += std::sqrt(movement.x*movement.x + movement.y*movement.y);
}

bool Projectile::isExpired() const {
    return traveled >= maxDistance;
}

void Projectile::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(4.f);
    shape.setPosition(position - sf::Vector2f(4.f, 4.f));
    shape.setFillColor(fromPlayer ? sf::Color::Yellow : sf::Color::Magenta);
    window.draw(shape);
}

/* ======================== RPGEntity ======================== */

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

/* ======================== Player ======================== */

Player::Player(sf::Vector2f pos) : RPGEntity(pos) {
    color = sf::Color::Cyan;
    radius = 18.f;
    classConfig = getClassConfig(CharacterClass::Warrior);
    weaponManager.getList().initDefaults();
}

void Player::update(float dt) {
    if (attackCooldown > 0) attackCooldown -= dt;
    RPGEntity::update(dt);
}

bool Player::canAttack() const {
    return attackCooldown <= 0;
}

void Player::resetAttackCooldown() {
    attackCooldown = attackCooldownMax;
}

bool Player::equipWeapon(const std::string& name) {
    auto found = weaponManager.getList().find(name);
    if (!found) return false;

    // 校验职业匹配
    if (found->type != classConfig.allowedWeapon) return false;

    weaponManager.equip(name);
    equippedWeapon = weaponManager.getEquipped();
    recalcCombatStats();
    return true;
}

void Player::recalcCombatStats() {
    Attributes finalAttr = getFinalAttributes();

    if (equippedWeapon) {
        attackDamage = equippedWeapon->damage;
        attackRange = equippedWeapon->range;
        attackCooldownMax = 1.0f / equippedWeapon->attackSpeed;
    } else {
        attackDamage = 5;       // 基础攻击
        attackRange = 40.f;
        attackCooldownMax = 0.8f;
    }

    // maxHealth 由 vitality 决定
    maxHealth = 100 + finalAttr.vitality * 5;
    if (health > maxHealth) health = maxHealth;
}

int Player::calculateDamage() const {
    int baseDmg = attackDamage;
    Attributes finalAttr = getFinalAttributes();

    // 属性加成：剑→力量×0.5, 法器→魔法×0.5, 刺刀→敏捷×0.5
    float attrBonus = 0.f;
    if (equippedWeapon) {
        switch (equippedWeapon->type) {
            case WeaponType::Sword:    attrBonus = finalAttr.strength * 0.5f; break;
            case WeaponType::Artifact: attrBonus = finalAttr.magic * 0.5f; break;
            case WeaponType::Dagger:   attrBonus = finalAttr.agility * 0.5f; break;
        }
    }

    return baseDmg + static_cast<int>(attrBonus);
}

Attributes Player::getFinalAttributes() const {
    return getTotalAttributes(baseAttributes, classConfig, levelData.getLevelBonus());
}

void Player::draw(sf::RenderWindow& window) const {
    // 圆形主体
    sf::CircleShape shape(radius);
    shape.setPosition(position - sf::Vector2f(radius, radius));
    shape.setFillColor(color);
    window.draw(shape);

    // 血条（上方）
    float barWidth = 50.f;
    float barHeight = 8.f;
    float barX = position.x - barWidth / 2;
    float barY = position.y - radius - 12.f;
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

void Player::drawWithName(sf::RenderWindow& window, const sf::Font* font) const {
    draw(window);

    // 武器指示器（在角色右侧显示小图标）
    if (equippedWeapon) {
        sf::Color weaponColor;
        float weaponSize = 6.f;
        switch (equippedWeapon->type) {
            case WeaponType::Sword:    weaponColor = sf::Color(200, 200, 200); break;  // 银白色-剑
            case WeaponType::Artifact: weaponColor = sf::Color(255, 100, 0); break;     // 橙色-法器
            case WeaponType::Dagger:   weaponColor = sf::Color(150, 0, 200); break;     // 紫色-刺刀
        }
        // 剑=长条, 法器=圆, 刺刀=短条
        if (equippedWeapon->type == WeaponType::Artifact) {
            sf::CircleShape wIcon(weaponSize);
            wIcon.setFillColor(weaponColor);
            wIcon.setPosition(sf::Vector2f(position.x + radius + 2, position.y - weaponSize));
            window.draw(wIcon);
        } else {
            float w = (equippedWeapon->type == WeaponType::Sword) ? 4.f : 3.f;
            float h = (equippedWeapon->type == WeaponType::Sword) ? 14.f : 10.f;
            sf::RectangleShape wIcon(sf::Vector2f(w, h));
            wIcon.setFillColor(weaponColor);
            wIcon.setPosition(sf::Vector2f(position.x + radius + 2, position.y - h / 2));
            window.draw(wIcon);
        }
    }

    if (font && !username.empty()) {
        sf::Text nameText(*font, sf::String::fromUtf8(username.begin(), username.end()), 12);
        nameText.setFillColor(sf::Color::White);
        nameText.setPosition(sf::Vector2f(position.x - nameText.getLocalBounds().size.x / 2,
                                          position.y + radius + 2));
        window.draw(nameText);
    }
}

/* ======================== Enemy ======================== */

Enemy::Enemy(EnemyType type, sf::Vector2f pos) : RPGEntity(pos), enemyType(type) {
    EnemyConfig cfg = getEnemyConfig(type);
    name = cfg.name;
    health = cfg.health;
    maxHealth = cfg.health;
    speed = cfg.speed;
    attackDamage = cfg.attackDamage;
    attackRange = cfg.attackRange;
    attackCooldownMax = cfg.attackCooldownMax;
    isRanged = cfg.isRanged;
    score = cfg.score;
    targetPos = pos;

    // 根据类型设置颜色和大小
    switch (type) {
        case EnemyType::Slime:
            color = sf::Color(0, 200, 0);      // 绿色
            radius = 12.f;
            break;
        case EnemyType::SkeletonWarrior:
            color = sf::Color(200, 200, 200);   // 灰白色
            radius = 16.f;
            break;
        case EnemyType::SkeletonArcher:
            color = sf::Color(180, 180, 140);   // 暗黄
            radius = 14.f;
            break;
        case EnemyType::Giant:
            color = sf::Color(139, 90, 43);     // 棕色
            radius = 28.f;
            break;
    }
}

void Enemy::scaleStats(float scale) {
    health = static_cast<int>(health * scale);
    maxHealth = static_cast<int>(maxHealth * scale);
    attackDamage = static_cast<int>(attackDamage * scale);
    score = static_cast<int>(score * scale);
    // Speed scales slightly (sqrt)
    speed *= std::sqrt(scale);
}

void Enemy::setTarget(const sf::Vector2f& target) {
    targetPos = target;
}

void Enemy::update(float dt) {
    if (attackCooldown > 0) attackCooldown -= dt;

    sf::Vector2f dir = targetPos - position;
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);

    switch (enemyType) {
        case EnemyType::Slime:
            // 缓慢追踪
            if (dist > 0) velocity = normalize(dir) * speed;
            break;

        case EnemyType::SkeletonWarrior:
            // 追踪玩家
            if (dist > 0) velocity = normalize(dir) * speed;
            break;

        case EnemyType::SkeletonArcher:
            // 保持距离（150-250），太近后退，太远靠近
            if (dist < 150.f && dist > 0) {
                velocity = normalize(dir) * (-speed);  // 后退
            } else if (dist > 250.f) {
                velocity = normalize(dir) * speed;     // 靠近
            } else {
                velocity = {0, 0};  // 保持位置
            }
            break;

        case EnemyType::Giant:
            // 缓慢追踪
            if (dist > 0) velocity = normalize(dir) * speed;
            break;
    }

    RPGEntity::update(dt);
}

bool Enemy::canAttack() const {
    return attackCooldown <= 0;
}

void Enemy::resetAttackCooldown() {
    attackCooldown = attackCooldownMax;
}

void Enemy::draw(sf::RenderWindow& window) const {
    // 菱形渲染
    float size = radius;
    sf::RectangleShape diamond(sf::Vector2f(size * 1.414f, size * 1.414f));
    diamond.setOrigin(sf::Vector2f(size * 1.414f / 2, size * 1.414f / 2));
    diamond.setPosition(position);
    diamond.setRotation(sf::degrees(45.f));
    diamond.setFillColor(color);
    window.draw(diamond);

    // 血条（上方）
    float barWidth = radius * 2;
    float barHeight = 5.f;
    float barX = position.x - barWidth / 2;
    float barY = position.y - radius - 12.f;
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

void Enemy::drawWithName(sf::RenderWindow& window, const sf::Font* font) const {
    draw(window);
    if (font && !name.empty()) {
        // Translate enemy name
        std::string translatedName = name;
        if (name == "Slime") translatedName = I18n::instance().t("slime");
        else if (name == "Sk.Warrior") translatedName = I18n::instance().t("sk_warrior");
        else if (name == "Sk.Archer") translatedName = I18n::instance().t("sk_archer");
        else if (name == "Giant") translatedName = I18n::instance().t("giant");

        sf::Text nameText(*font, sf::String::fromUtf8(translatedName.begin(), translatedName.end()), 10);
        // Use enemy's distinct color for the name (brighter version)
        sf::Color nameColor = color;
        nameText.setFillColor(sf::Color(
            std::min(255, nameColor.r + 50),
            std::min(255, nameColor.g + 50),
            std::min(255, nameColor.b + 50)
        ));
        nameText.setPosition(sf::Vector2f(position.x - nameText.getLocalBounds().size.x / 2,
                                          position.y + radius + 2));
        window.draw(nameText);
    }
}

/* ======================== GameWorld ======================== */

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
    std::vector<int> ids;
    for (const auto& [id, p] : players) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    for (int id : ids) {
        const auto& p = players.at(id);
        GameState::PlayerState ps;
        ps.position = p.position;
        ps.health = p.health;
        ps.maxHealth = p.maxHealth;
        state.players.push_back(ps);
    }
    return state;
}
