#include "GameLogic.hpp"
#include "I18n.hpp"
#include <limits>

/* ======================== 辅助函数 ======================== */

sf::Vector2f normalize(const sf::Vector2f& v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y);
    if (len < 1e-6f) return sf::Vector2f(0,0);
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
                    WEAPON_SWORD, "Warrior"};
        case CharacterClass::Mage:
            return {CharacterClass::Mage,
                    Attributes{0, 0, 5, 0, 0, 0},  // 魔法+5
                    WEAPON_ARTIFACT, "Mage"};
        case CharacterClass::Assassin:
            return {CharacterClass::Assassin,
                    Attributes{0, 5, 0, 0, 0, 0},  // 敏捷+5
                    WEAPON_DAGGER, "Assassin"};
    }
    return {CharacterClass::Warrior, {}, WEAPON_SWORD, "Warrior"};
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
    if (experience >= expToNextLevel && expToNextLevel > 0 && level < 999) {
        experience -= expToNextLevel;
        level++;
        int nextExp = 100 * level;
        expToNextLevel = (nextExp > 0) ? nextExp : std::numeric_limits<int>::max();
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
            return {EnemyType::Slime, "Slime", 40, 8, 50.f, 30.f, 0.8f, false, 15};
        case EnemyType::SkeletonWarrior:
            return {EnemyType::SkeletonWarrior, "Sk.Warrior", 80, 15, 80.f, 50.f, 1.0f, false, 30};
        case EnemyType::SkeletonArcher:
            return {EnemyType::SkeletonArcher, "Sk.Archer", 50, 12, 70.f, 250.f, 1.5f, true, 25};
        case EnemyType::Giant:
            return {EnemyType::Giant, "Giant", 200, 30, 25.f, 60.f, 2.5f, false, 60};
    }
    return {EnemyType::Slime, "Slime", 40, 8, 50.f, 30.f, 0.8f, false, 15};
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
    shape.setFillColor(color);
    window.draw(shape);
}

/* ======================== 掉落物系统 ======================== */

void DropItem::update(float dt) {
    lifetime -= dt;
    bobTimer += dt;
}

void DropItem::draw(sf::RenderWindow& window) const {
    float bobOffset = std::sin(bobTimer * 3.f) * 3.f;  // 上下浮动
    sf::Vector2f drawPos = position + sf::Vector2f(0, bobOffset);

    sf::Color col = getColor();
    // 外圈光晕
    sf::CircleShape glow(12.f);
    glow.setPosition(drawPos - sf::Vector2f(12.f, 12.f));
    glow.setFillColor(sf::Color(col.r, col.g, col.b, 60));
    window.draw(glow);
    // 内圈
    sf::CircleShape inner(8.f);
    inner.setPosition(drawPos - sf::Vector2f(8.f, 8.f));
    inner.setFillColor(col);
    window.draw(inner);
    // 中心高亮
    sf::CircleShape center(3.f);
    center.setPosition(drawPos - sf::Vector2f(3.f, 3.f));
    center.setFillColor(sf::Color(255, 255, 255, 200));
    window.draw(center);
}

sf::Color DropItem::getColor() const {
    switch (type) {
        case DropType::HealthPotion: return sf::Color(255, 50, 50);    // 红色
        case DropType::StrBoost:     return sf::Color(255, 165, 0);    // 橙色
        case DropType::AgiBoost:     return sf::Color(0, 200, 255);    // 青色
        case DropType::MagBoost:     return sf::Color(180, 0, 255);    // 紫色
        case DropType::VitBoost:     return sf::Color(0, 255, 100);    // 绿色
        case DropType::RangeBoost:   return sf::Color(255, 255, 0);    // 黄色
    }
    return sf::Color::White;
}

const char* DropItem::getLabel() const {
    switch (type) {
        case DropType::HealthPotion: return "HP+";
        case DropType::StrBoost:     return "STR+";
        case DropType::AgiBoost:     return "AGI+";
        case DropType::MagBoost:     return "MAG+";
        case DropType::VitBoost:     return "VIT+";
        case DropType::RangeBoost:   return "RNG+";
    }
    return "?";
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
}

void Player::update(float dt) {
    if (attackCooldown > 0) attackCooldown -= dt;
    if (rollCooldown > 0) rollCooldown -= dt;
    updateRoll(dt);
    updateSlash(dt);
    RPGEntity::update(dt);
}

bool Player::canAttack() const {
    return attackCooldown <= 0 && !isRolling;
}

void Player::resetAttackCooldown() {
    attackCooldown = attackCooldownMax;
}

bool Player::canRoll() const {
    return rollCooldown <= 0 && !isRolling && !slashActive;
}

void Player::startRoll(const sf::Vector2f& dir) {
    if (!canRoll()) return;
    rollDirection = (dir != sf::Vector2f(0,0)) ? normalize(dir) : sf::Vector2f(1, 0);
    isRolling = true;
    isInvincible = true;
    rollTimer = rollDuration;
    rollCooldown = rollCooldownMax;
}

void Player::updateRoll(float dt) {
    if (!isRolling) {
        isInvincible = false;
        return;
    }
    rollTimer -= dt;
    velocity = rollDirection * rollSpeed;
    if (rollTimer <= 0) {
        isRolling = false;
        isInvincible = false;
        velocity = {0, 0};
    }
}

void Player::startSlash(float angle) {
    slashActive = true;
    slashTimer = slashDuration;
    slashAngle = angle;
}

void Player::updateSlash(float dt) {
    if (!slashActive) return;
    slashTimer -= dt;
    if (slashTimer <= 0) {
        slashActive = false;
    }
}

bool Player::equipWeapon(const std::string& name) {
    auto found = weaponList.findWeapon(name);
    if (!found) return false;

    // 校验职业匹配
    if (found->type != classConfig.allowedWeapon) return false;

    weaponList.equipWeapon(name);
    equippedWeapon = weaponList.getEquipped();
    recalcCombatStats();

    // 设置刀光颜色：统一橙色
    switch (charClass) {
        case CharacterClass::Warrior:
            slashColor = sf::Color(255, 140, 0);    // 战士：橙色
            break;
        case CharacterClass::Mage:
            slashColor = sf::Color(255, 100, 0);    // 法师：亮橙色
            break;
        case CharacterClass::Assassin:
            slashColor = sf::Color(255, 160, 0);    // 刺客：金橙色
            break;
    }

    return true;
}

void Player::recalcCombatStats() {
    Attributes finalAttr = getFinalAttributes();

    if (equippedWeapon) {
        attackDamage = equippedWeapon->damage;
        attackRange = equippedWeapon->range + attackRangeBonus;
        attackCooldownMax = (equippedWeapon->attackSpeed > 0.01f) ? (1.0f / equippedWeapon->attackSpeed) : 0.8f;
    } else {
        attackDamage = 5;       // 基础攻击
        attackRange = 40.f + attackRangeBonus;
        attackCooldownMax = 0.8f;
    }

    // 阶段强化：攻速加成减少冷却时间
    if (attackSpeedBonus > 0.f) {
        attackCooldownMax = std::max(0.05f, attackCooldownMax / (1.0f + attackSpeedBonus));
    }

    // 阶段强化：战士攻击范围增大
    if (charClass == CharacterClass::Warrior && victoryStageLevel > 0) {
        attackRange += victoryStageLevel * 10.f;
    }

    // maxHealth 由 vitality 决定，分数加成也增加生命
    maxHealth = 120 + finalAttr.vitality * 8;
    // 阶段强化：生命值提升
    if (victoryStageLevel > 0) {
        maxHealth += victoryStageLevel * 30;
    }
    if (health > maxHealth) health = maxHealth;

    // 分数加成：速度随分数提升
    float scoreMult = getScoreMultiplier();
    speed = 150.f * scoreMult;
    rollSpeed = 400.f * scoreMult;
}

void Player::applyStageBoost(int stageLevel) {
    if (stageLevel <= victoryStageLevel) return;  // 不重复应用低等级
    victoryStageLevel = stageLevel;

    switch (charClass) {
        case CharacterClass::Warrior:
            // 战士：伤害大幅提升，攻击范围增大（在recalcCombatStats中处理）
            damageBonus = stageLevel * 5.f;       // 每阶段+5伤害
            attackSpeedBonus = stageLevel * 0.15f; // 每阶段+15%攻速
            break;
        case CharacterClass::Mage:
            // 法师：投射物爆炸范围杀伤，伤害提升
            damageBonus = stageLevel * 4.f;
            projectileExplosionRadius = stageLevel * 30.f;  // 每阶段+30爆炸半径
            attackSpeedBonus = stageLevel * 0.1f;            // 每阶段+10%攻速
            break;
        case CharacterClass::Assassin:
            // 刺客：暴击概率和伤害提升，攻速大幅提升
            damageBonus = stageLevel * 3.f;
            critChance = std::min(0.8f, stageLevel * 0.12f);  // 每阶段+12%暴击率，上限80%
            critMultiplier = 1.5f + stageLevel * 0.2f;         // 暴击倍率递增
            attackSpeedBonus = stageLevel * 0.25f;              // 每阶段+25%攻速
            break;
    }

    recalcCombatStats();
    // 阶段强化后回满血
    health = maxHealth;
}

float Player::getScoreMultiplier() const {
    // 每50分增加10%加成，上限200%（使用浮点除法避免精度丢失）
    return 1.0f + std::min(static_cast<float>(totalScore) / 50.f * 0.1f, 1.0f);
}

int Player::calculateDamage() const {
    int baseDmg = attackDamage;
    Attributes finalAttr = getFinalAttributes();

    // 属性加成：剑→力量×0.8, 法器→魔法×0.8, 刺刀→敏捷×0.8
    float attrBonus = 0.f;
    if (equippedWeapon) {
        switch (equippedWeapon->type) {
            case WEAPON_SWORD:    attrBonus = finalAttr.strength * 0.8f; break;
            case WEAPON_ARTIFACT: attrBonus = finalAttr.magic * 0.8f; break;
            case WEAPON_DAGGER:   attrBonus = finalAttr.agility * 0.8f; break;
        }
    }

    // 分数加成
    float scoreMult = getScoreMultiplier();

    // 阶段强化伤害加成
    float totalDmg = (baseDmg + attrBonus + damageBonus) * scoreMult;

    // 暴击判定（所有职业都有基础暴击率，刺客更高）
    lastAttackWasCrit = false;
    float effectiveCritChance = critChance;
    if (effectiveCritChance <= 0.f) effectiveCritChance = 0.05f;  // 基础5%暴击率
    {
        static thread_local std::mt19937 critGen(std::random_device{}());
        std::uniform_real_distribution<float> critDist(0.f, 1.f);
        if (critDist(critGen) < effectiveCritChance) {
            totalDmg *= critMultiplier;
            lastAttackWasCrit = true;
        }
    }

    return static_cast<int>(totalDmg);
}

Attributes Player::getFinalAttributes() const {
    Attributes base = getTotalAttributes(baseAttributes, classConfig, levelData.getLevelBonus());
    // 分数加成：每50分给所有属性+1
    int scoreBonus = std::min(totalScore / 50, 20);
    Attributes scoreAttr{scoreBonus, scoreBonus, scoreBonus, scoreBonus, scoreBonus, scoreBonus};
    return base + scoreAttr;
}

void Player::draw(sf::RenderWindow& window) const {
    // 翻滚时半透明
    sf::Color drawColor = color;
    if (isRolling) drawColor.a = 128;

    // 圆形主体
    sf::CircleShape shape(radius);
    shape.setPosition(position - sf::Vector2f(radius, radius));
    shape.setFillColor(drawColor);
    window.draw(shape);

    // 翻滚时显示方向指示
    if (isRolling) {
        sf::RectangleShape arrow(sf::Vector2f(12.f, 4.f));
        arrow.setFillColor(sf::Color::White);
        arrow.setPosition(position);
        float angle = std::atan2(rollDirection.y, rollDirection.x);
        arrow.setRotation(sf::radians(angle));
        arrow.setOrigin({0, 2});
        window.draw(arrow);
    }

    // 血条（上方）
    float barWidth = 50.f;
    float barHeight = 8.f;
    float barX = position.x - barWidth / 2;
    float barY = position.y - radius - 12.f;
    sf::RectangleShape bg(sf::Vector2f(barWidth, barHeight));
    bg.setFillColor(sf::Color::Red);
    bg.setPosition(sf::Vector2f(barX, barY));
    window.draw(bg);
    float healthPercent = (maxHealth > 0) ? static_cast<float>(health) / maxHealth : 0.f;
    sf::RectangleShape bar(sf::Vector2f(barWidth * healthPercent, barHeight));
    bar.setFillColor(sf::Color::Green);
    bar.setPosition(sf::Vector2f(barX, barY));
    window.draw(bar);
}

void Player::drawSlash(sf::RenderWindow& window) const {
    if (!slashActive) return;

    float progress = 1.0f - (slashTimer / slashDuration);  // 0→1 挥砍进度
    float halfArc = 1.57f;  // 半弧（π/2 ≈ 90度，总弧180度）
    float arcStart = slashAngle - halfArc;   // 起始角度
    float arcEnd = slashAngle + halfArc;     // 结束角度

    // 剑气拖尾特效：弧形扫过，拖尾渐隐
    int segments = 24;
    float arcLength = arcEnd - arcStart;
    float segAngle = arcLength / segments;

    // 当前剑尖角度（随进度扫过弧线）
    float currentSwordAngle = arcStart + arcLength * progress;

    for (int i = 0; i < segments; ++i) {
        float segCenterAngle = arcStart + (i + 0.5f) * segAngle;

        // 只绘制已经扫过的部分（拖尾）
        if (segCenterAngle > currentSwordAngle) continue;

        // 拖尾衰减：越远离剑尖越淡
        float distFromTip = (currentSwordAngle - segCenterAngle) / arcLength;
        float trailAlpha = std::max(0.f, 1.0f - distFromTip * 2.5f);  // 拖尾快速衰减
        if (trailAlpha <= 0.f) continue;

        // 剑尖处最亮，拖尾渐暗
        float brightness = trailAlpha;

        // 内外半径：剑气从角色边缘延伸到攻击范围
        float innerR = radius + 3.f;
        float outerR = radius + attackRange * (0.4f + 0.6f * brightness);

        // 绘制梯形四边形（剑气宽度）
        float halfWidth = segAngle * 0.5f;
        sf::Vector2f p1 = position + sf::Vector2f(std::cos(segCenterAngle - halfWidth) * innerR,
                                                    std::sin(segCenterAngle - halfWidth) * innerR);
        sf::Vector2f p2 = position + sf::Vector2f(std::cos(segCenterAngle + halfWidth) * innerR,
                                                    std::sin(segCenterAngle + halfWidth) * innerR);
        sf::Vector2f p3 = position + sf::Vector2f(std::cos(segCenterAngle + halfWidth) * outerR,
                                                    std::sin(segCenterAngle + halfWidth) * outerR);
        sf::Vector2f p4 = position + sf::Vector2f(std::cos(segCenterAngle - halfWidth) * outerR,
                                                    std::sin(segCenterAngle - halfWidth) * outerR);

        std::uint8_t alphaOuter = static_cast<std::uint8_t>(brightness * 200);
        std::uint8_t alphaInner = static_cast<std::uint8_t>(brightness * 255);
        sf::Color colOuter(slashColor.r, slashColor.g, slashColor.b, alphaOuter);
        sf::Color colInner(slashColor.r, slashColor.g, slashColor.b, alphaInner);

        sf::Vertex quad[4] = {
            {p1, colInner},
            {p2, colInner},
            {p3, colOuter},
            {p4, colOuter}
        };
        window.draw(quad, 4, sf::PrimitiveType::TriangleStrip);
    }

    // 剑尖高亮光点
    float tipR = radius + attackRange * 0.9f;
    sf::Vector2f tipPos = position + sf::Vector2f(std::cos(currentSwordAngle) * tipR,
                                                    std::sin(currentSwordAngle) * tipR);
    float tipAlpha = std::min(1.0f, progress * 5.f) * std::min(1.0f, (1.0f - progress) * 5.f);
    if (tipAlpha > 0.f) {
        sf::CircleShape tip(4.f);
        tip.setPosition(tipPos - sf::Vector2f(4.f, 4.f));
        tip.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(tipAlpha * 220)));
        window.draw(tip);
    }
}

void Player::drawWithName(sf::RenderWindow& window, const sf::Font* font) const {
    draw(window);

    // 武器指示器（在角色右侧显示小图标）
    if (equippedWeapon) {
        sf::Color weaponColor;
        float weaponSize = 6.f;
        switch (equippedWeapon->type) {
            case WEAPON_SWORD:    weaponColor = sf::Color(200, 200, 200); break;  // 银白色-剑
            case WEAPON_ARTIFACT: weaponColor = sf::Color(255, 100, 0); break;     // 橙色-法器
            case WEAPON_DAGGER:   weaponColor = sf::Color(150, 0, 200); break;     // 紫色-刺刀
        }
        // 剑=长条, 法器=圆, 刺刀=短条
        if (equippedWeapon->type == WEAPON_ARTIFACT) {
            sf::CircleShape wIcon(weaponSize);
            wIcon.setFillColor(weaponColor);
            wIcon.setPosition(sf::Vector2f(position.x + radius + 2, position.y - weaponSize));
            window.draw(wIcon);
        } else {
            float w = (equippedWeapon->type == WEAPON_SWORD) ? 4.f : 3.f;
            float h = (equippedWeapon->type == WEAPON_SWORD) ? 14.f : 10.f;
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
    if (scale < 0.01f) scale = 0.01f;  // 防止零值/负值导致属性异常
    health = std::max(1, static_cast<int>(health * scale));
    maxHealth = std::max(1, static_cast<int>(maxHealth * scale));
    attackDamage = std::max(1, static_cast<int>(attackDamage * scale));
    score = std::max(1, static_cast<int>(score * scale));
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
            else velocity = {0, 0};
            break;

        case EnemyType::SkeletonWarrior:
            // 追踪玩家
            if (dist > 0) velocity = normalize(dir) * speed;
            else velocity = {0, 0};
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
            else velocity = {0, 0};
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
    float healthPercent = (maxHealth > 0) ? static_cast<float>(health) / maxHealth : 0.f;
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

        sf::Text nameText(*font, sf::String::fromUtf8(translatedName.begin(), translatedName.end()), 13);
        float textX = position.x - nameText.getLocalBounds().size.x / 2;
        float textY = position.y + radius + 2;

        // 黑色描边（4方向偏移）让名字更显眼
        nameText.setFillColor(sf::Color::Black);
        for (auto [dx, dy] : {std::pair{-1,0},{1,0},{0,-1},{0,1}}) {
            nameText.setPosition(sf::Vector2f(textX + dx, textY + dy));
            window.draw(nameText);
        }
        // 白色主体
        nameText.setFillColor(sf::Color::White);
        nameText.setPosition(sf::Vector2f(textX, textY));
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
        target.health = std::max(0, target.health - attacker.attackDamage);
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
