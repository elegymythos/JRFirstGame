/**
 * @file GameView.cpp
 * @brief 单机闯关游戏视图实现
 */

#include "GameView.hpp"
#include "Database.hpp"
#include "MapSystem.hpp"
#include "I18n.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

// ======================== ActualGameView 实现 ========================

ActualGameView::ActualGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               const std::string& name, Database& database)
    : View(font), db(database), username(name), player({400,300}),
      attrPanel(font),
      infoText(font, "", 20),
      gameOverText(font, "", 40),
      pauseText(font, "", 40),
      victoryText(font, "", 40),
      backBtn(font, I18n::instance().t("back_menu"), {1150,20}, {120,40}, [cv](){ cv("SELECT_LEVEL"); }),
      restartBtn(font, I18n::instance().t("restart"), {590,400}, {100,50}, [this](){ restartGame(); }),
      resumeBtn(font, I18n::instance().t("resume"), {590,350}, {100,50}, [this](){
          if (paused) {
              float currentTime = gameClock.getElapsedTime().asSeconds();
              pausedTime += (currentTime - lastPauseTime);
              paused = false;
          }
      }),
      pauseBackBtn(font, I18n::instance().t("back_menu"), {590,420}, {100,50}, [this, cv](){
          db.saveCharacter(buildCharacterData());
          db.saveRanking(username, player.levelData.level, player.totalScore);
          cv("SELECT_LEVEL");
      }),
      victoryContinueBtn(font, I18n::instance().t("continue"), {540,400}, {100,50}, [this](){ continueFromVictory(); }),
      victoryBackBtn(font, I18n::instance().t("back_menu"), {640,400}, {100,50}, [this, cv](){
          db.saveCharacter(buildCharacterData());
          db.saveRanking(username, player.levelData.level, player.totalScore);
          cv("SELECT_LEVEL");
      }),
      changeView(cv) {
    player.username = username;

    int slot = db.getSelectedSlot(username);
    auto charData = db.loadCharacter(username, slot);
    if (charData) {
        player.baseAttributes = charData->attributes;
        player.charClass = charData->classType;
        player.classConfig = getClassConfig(charData->classType);
        player.levelData.level = charData->level;
        player.levelData.experience = charData->experience;
        player.levelData.expToNextLevel = 100 * charData->level;
        player.totalScore = charData->score;
        currentVictoryStage = charData->victoryStage;
        pausedTime = -charData->gameTime;
    } else {
        player.baseAttributes = generateRandomAttributes();
        player.charClass = CharacterClass::Warrior;
        player.classConfig = getClassConfig(CharacterClass::Warrior);
    }

    player.weaponList.initDefaultWeapons();
    auto weapon = player.weaponList.findWeaponByType(player.classConfig.allowedWeapon);
    if (weapon) player.equipWeapon(weapon->name);
    player.recalcCombatStats();
    player.health = player.maxHealth;

    gameView.setSize(sf::Vector2f(800, 600));
    gameView.setCenter(player.position);

    infoText.setPosition({10,10}); infoText.setFillColor(sf::Color::Black);
    std::string goStr = I18n::instance().t("game_over");
    gameOverText.setString(sf::String::fromUtf8(goStr.begin(), goStr.end()));
    gameOverText.setPosition({490,300}); gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setCharacterSize(30);

    std::string pauseStr = I18n::instance().t("paused");
    pauseText.setString(sf::String::fromUtf8(pauseStr.begin(), pauseStr.end()));
    pauseText.setPosition({590,280}); pauseText.setFillColor(sf::Color::Blue);
    pauseText.setCharacterSize(40);

    std::string victoryStr = I18n::instance().t("stage_victory");
    victoryText.setString(sf::String::fromUtf8(victoryStr.begin(), victoryStr.end()));
    victoryText.setPosition({540,280}); victoryText.setFillColor(sf::Color::Green);
    victoryText.setCharacterSize(40);
}

void ActualGameView::attack() {
    if (!player.canAttack()) return;

    if (player.equippedWeapon && player.equippedWeapon->isRanged) {
        sf::Vector2f dir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(player.position, e.position);
            if (d < minDist) { minDist = d; dir = normalize(e.position - player.position); }
        }
        Projectile proj;
        proj.position = player.position;
        proj.direction = dir;
        proj.speed = 400.f;
        proj.damage = player.calculateDamage();
        proj.maxDistance = player.attackRange;
        proj.traveled = 0.f;
        proj.fromPlayer = true;
        proj.color = sf::Color::Red;  // 法师火球红色
        proj.explosionRadius = player.projectileExplosionRadius;  // 阶段强化爆炸半径
        proj.isCrit = player.lastAttackWasCrit;  // 暴击标记
        projectiles.push_back(proj);
    } else {
        sf::Vector2f attackDir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(player.position, e.position);
            if (d < minDist) { minDist = d; attackDir = normalize(e.position - player.position); }
        }
        float attackAngle = std::atan2(attackDir.y, attackDir.x);

        player.startSlash(attackAngle);

        float halfArc = 1.57f;  // 半弧（与刀光动画一致）
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float dist = distance(player.position, e.position);
            if (dist <= player.attackRange) {
                sf::Vector2f toEnemy = normalize(e.position - player.position);
                float enemyAngle = std::atan2(toEnemy.y, toEnemy.x);
                float angleDiff = std::abs(std::atan2(std::sin(enemyAngle - attackAngle), std::cos(enemyAngle - attackAngle)));
                if (angleDiff <= halfArc) {
                    int dmg = player.calculateDamage();
                    e.health = std::max(0, e.health - dmg);
                    // 生成伤害数字
                    DamageNumber dn;
                    dn.position = e.position;
                    dn.damage = dmg;
                    dn.isCrit = player.lastAttackWasCrit;
                    dn.angle = enemyAngle;
                    dn.radius = e.radius + 10.f;
                    damageNumbers.push_back(dn);
                }
            }
        }
    }
    player.resetAttackCooldown();
}

void ActualGameView::handleEnemyAttacks(float dt) {
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;

        if (e.isRanged) {
            if (e.canAttack()) {
                float dist = distance(e.position, player.position);
                if (dist <= e.attackRange) {
                    Projectile proj;
                    proj.position = e.position;
                    proj.direction = normalize(player.position - e.position);
                    proj.speed = 250.f;
                    proj.damage = e.attackDamage;
                    proj.maxDistance = e.attackRange;
                    proj.traveled = 0.f;
                    proj.fromPlayer = false;
                    proj.color = sf::Color::Magenta;  // 敌人投射物品红色
                    projectiles.push_back(proj);
                    e.resetAttackCooldown();
                }
            }
        } else {
            float dist = distance(player.position, e.position);
            if (dist < player.radius + e.radius) {
                if (e.canAttack() && !player.isInvincible) {
                    player.health = std::max(0, player.health - e.attackDamage);
                    e.resetAttackCooldown();
                    sf::Vector2f dir = player.position - e.position;
                    if (dir != sf::Vector2f(0,0)) dir = normalize(dir);
                    player.position += dir * 20.f;
                    if (player.health <= 0) {
                        gameOver = true;
                        db.saveCharacter(buildCharacterData());
                        db.saveRanking(username, player.levelData.level, player.totalScore);
                    }
                }
            }
        }
    }
}

void ActualGameView::updateProjectiles(float dt) {
    for (auto& proj : projectiles) proj.update(dt);

    for (auto& proj : projectiles) {
        if (proj.fromPlayer) {
            for (auto& e : enemies) {
                if (!e.isAlive()) continue;
                float dist = distance(proj.position, e.position);
                if (dist < e.radius + 4.f) {
                    e.health -= proj.damage;
                    e.health = std::max(0, e.health);
                    // 生成伤害数字
                    DamageNumber dn;
                    dn.position = e.position;
                    dn.damage = proj.damage;
                    dn.isCrit = proj.isCrit;
                    dn.angle = std::atan2(proj.direction.y, proj.direction.x);
                    dn.radius = e.radius + 10.f;
                    damageNumbers.push_back(dn);
                    // 法师爆炸范围杀伤
                    if (proj.explosionRadius > 0.f) {
                        for (auto& e2 : enemies) {
                            if (&e2 == &e || !e2.isAlive()) continue;
                            float dist2 = distance(proj.position, e2.position);
                            if (dist2 < proj.explosionRadius) {
                                // 距离越远伤害越低
                                float falloff = 1.0f - (dist2 / proj.explosionRadius) * 0.5f;
                                int aoeDmg = static_cast<int>(proj.damage * falloff);
                                e2.health -= aoeDmg;
                                e2.health = std::max(0, e2.health);
                                // 爆炸范围伤害数字
                                DamageNumber dn2;
                                dn2.position = e2.position;
                                dn2.damage = aoeDmg;
                                dn2.isCrit = false;
                                dn2.angle = std::atan2(e2.position.y - proj.position.y, e2.position.x - proj.position.x);
                                dn2.radius = e2.radius + 10.f;
                                damageNumbers.push_back(dn2);
                            }
                        }
                    }
                    proj.traveled = proj.maxDistance;
                    break;
                }
            }
        } else {
            float dist = distance(proj.position, player.position);
            if (dist < player.radius + 4.f) {
                if (!player.isInvincible) {
                    player.health = std::max(0, player.health - proj.damage);
                    proj.traveled = proj.maxDistance;
                    if (player.health <= 0) {
                        gameOver = true;
                        db.saveCharacter(buildCharacterData());
                        db.saveRanking(username, player.levelData.level, player.totalScore);
                    }
                } else {
                    proj.traveled = proj.maxDistance;
                }
            }
        }
    }

    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p){ return p.isExpired(); }), projectiles.end());
}

void ActualGameView::updateInfoText() {
    float rawTime = gameClock.getElapsedTime().asSeconds();
    float adjustedTime = rawTime - pausedTime;
    int totalSeconds = static_cast<int>(adjustedTime);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    std::stringstream ss;
    ss << username << "  " << I18n::instance().t("hp") << player.health << "/" << player.maxHealth
       << "  " << I18n::instance().t("lv") << player.levelData.level
       << "  " << I18n::instance().t("score") << player.totalScore
       << "  " << I18n::instance().t("time") << " " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;

    if (player.equippedWeapon) {
        std::string weaponName = player.equippedWeapon->name;
        if (weaponName == "Iron Sword") weaponName = I18n::instance().t("iron_sword");
        else if (weaponName == "Flame Artifact") weaponName = I18n::instance().t("flame_artifact");
        else if (weaponName == "Shadow Dagger") weaponName = I18n::instance().t("shadow_dagger");

        ss << "  " << I18n::instance().t("weapon") << weaponName
           << " (" << I18n::instance().t("range") << static_cast<int>(player.attackRange) << ")";
    }

    ss << "  " << I18n::instance().t("controls");
    if (player.isRolling) {
        ss << " [ROLL]";
    } else if (player.rollCooldown > 0) {
        ss << " [Roll:" << static_cast<int>(player.rollCooldown * 10) << "]";
    }
    float scoreMult = player.getScoreMultiplier();
    if (scoreMult > 1.0f) {
        ss << " x" << std::fixed << std::setprecision(1) << scoreMult;
    }
    std::string info = ss.str();
    infoText.setString(sf::String::fromUtf8(info.begin(), info.end()));
}

void ActualGameView::restartGame() {
    player.position = {400, 300};
    player.velocity = {0, 0};
    player.health = player.maxHealth;
    player.killCount = 0;
    player.totalScore = 0;
    player.levelData = LevelData();
    player.attackCooldown = 0.f;
    player.isRolling = false;
    player.isInvincible = false;
    player.rollTimer = 0.f;
    player.rollCooldown = 0.f;
    player.slashActive = false;
    player.slashTimer = 0.f;
    enemies.clear();
    projectiles.clear();
    drops.clear();
    pickupEffects.clear();
    damageNumbers.clear();
    gameOver = false;
    paused = false;
    stageVictory = false;
    currentVictoryStage = 0;
    killCount = 0;
    pausedTime = 0.f;
    lastPauseTime = 0.f;
    gameClock.restart();
    mapSystem = MapSystem();
}

void ActualGameView::spawnDrops(const Enemy& enemy) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    float dropChance = 0.40f;  // 40%基础掉率（提高）
    // 强敌掉率更高
    if (enemy.enemyType == EnemyType::Giant) dropChance = 0.80f;
    else if (enemy.enemyType == EnemyType::SkeletonWarrior) dropChance = 0.55f;
    else if (enemy.enemyType == EnemyType::SkeletonArcher) dropChance = 0.45f;

    if (dist(gen) < dropChance) {
        DropItem item;
        item.position = enemy.position;
        // 随机掉落类型（提高属性掉率，降低血瓶掉率）
        float r = dist(gen);
        if (r < 0.30f)      item.type = DropType::HealthPotion;   // 30%
        else if (r < 0.45f) item.type = DropType::StrBoost;      // 15%
        else if (r < 0.60f) item.type = DropType::AgiBoost;      // 15%
        else if (r < 0.75f) item.type = DropType::MagBoost;      // 15%
        else if (r < 0.88f) item.type = DropType::VitBoost;      // 13%
        else                item.type = DropType::RangeBoost;     // 12%（提高）
        drops.push_back(item);
    }
}

void ActualGameView::updateDrops(float dt) {
    for (auto& d : drops) d.update(dt);
    drops.erase(std::remove_if(drops.begin(), drops.end(),
        [](const DropItem& d){ return d.lifetime <= 0; }), drops.end());
}

void ActualGameView::checkPickups() {
    float pickupRange = 30.f;
    for (auto it = drops.begin(); it != drops.end(); ) {
        float dist = distance(player.position, it->position);
        if (dist < pickupRange + player.radius) {
            // 拾取！
            PickupEffect effect;
            effect.position = it->position;
            effect.color = it->getColor();
            effect.timer = 3.0f;

            switch (it->type) {
                case DropType::HealthPotion: {
                    int heal = player.maxHealth / 3;  // 回复33%血量（提高）
                    player.health = std::min(player.maxHealth, player.health + heal);
                    effect.text = "HP+" + std::to_string(heal);
                    break;
                }
                case DropType::StrBoost:
                    player.baseAttributes.strength = std::min(30, player.baseAttributes.strength + 2);
                    effect.text = "STR+2";
                    break;
                case DropType::AgiBoost:
                    player.baseAttributes.agility = std::min(30, player.baseAttributes.agility + 2);
                    effect.text = "AGI+2";
                    break;
                case DropType::MagBoost:
                    player.baseAttributes.magic = std::min(30, player.baseAttributes.magic + 2);
                    effect.text = "MAG+2";
                    break;
                case DropType::VitBoost:
                    player.baseAttributes.vitality = std::min(30, player.baseAttributes.vitality + 2);
                    effect.text = "VIT+2";
                    break;
                case DropType::RangeBoost:
                    player.attackRangeBonus += 15.f;
                    effect.text = "RNG+15";
                    break;
            }
            player.recalcCombatStats();
            pickupEffects.push_back(effect);
            it = drops.erase(it);
        } else {
            ++it;
        }
    }
}

void ActualGameView::continueFromVictory() {
    float currentTime = gameClock.getElapsedTime().asSeconds();
    pausedTime += (currentTime - lastPauseTime);
    stageVictory = false;
}

CharacterData ActualGameView::buildCharacterData() const {
    CharacterData data;
    data.username = username;
    data.slot = db.getSelectedSlot(username);
    data.classType = player.charClass;
    data.level = player.levelData.level;
    data.experience = player.levelData.experience;
    data.score = player.totalScore;
    data.gameTime = gameClock.getElapsedTime().asSeconds() - pausedTime;
    data.victoryStage = currentVictoryStage;
    data.attributes = player.baseAttributes;
    return data;
}

void ActualGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !gameOver && !stageVictory) {
            if (!paused) {
                lastPauseTime = gameClock.getElapsedTime().asSeconds();
            } else {
                float currentTime = gameClock.getElapsedTime().asSeconds();
                pausedTime += (currentTime - lastPauseTime);
            }
            paused = !paused;
            return;
        }
    }

    if (stageVictory) {
        sf::Vector2f uiMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mbp->button == sf::Mouse::Button::Left) {
                victoryContinueBtn.handleEvent(uiMousePos, true);
                victoryBackBtn.handleEvent(uiMousePos, true);
            }
        }
        return;
    }

    if (paused) {
        sf::Vector2f uiMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mbp->button == sf::Mouse::Button::Left) {
                resumeBtn.handleEvent(uiMousePos, true);
                pauseBackBtn.handleEvent(uiMousePos, true);
            }
        }
        return;
    }

    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            backBtn.handleEvent(mPos, true);
            if (gameOver) restartBtn.handleEvent(mPos, true);
            else attack();
        }
    }
    if (!gameOver) {
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Space) attack();
            if (kp->code == sf::Keyboard::Key::Tab) attrPanel.toggle();
            if (kp->code == sf::Keyboard::Key::LShift || kp->code == sf::Keyboard::Key::RShift) {
                sf::Vector2f rollDir(0, 0);
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) rollDir.y -= 1;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) rollDir.y += 1;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) rollDir.x -= 1;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) rollDir.x += 1;
                player.startRoll(rollDir);
            }
            if (kp->code == sf::Keyboard::Key::P) {
                db.saveCharacter(buildCharacterData());
                db.saveRanking(username, player.levelData.level, player.totalScore);
            }
        }
    }
}

void ActualGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    backBtn.handleEvent(mousePos, false);

    if (stageVictory) {
        victoryContinueBtn.handleEvent(mousePos, false);
        victoryBackBtn.handleEvent(mousePos, false);
        return;
    }

    if (paused) {
        resumeBtn.handleEvent(mousePos, false);
        pauseBackBtn.handleEvent(mousePos, false);
        return;
    }

    if (gameOver) { restartBtn.handleEvent(mousePos, false); return; }

    // 检查阶段性胜利
    for (int i = STAGE_COUNT - 1; i >= 0; --i) {
        if (player.totalScore >= STAGE_THRESHOLDS[i] && currentVictoryStage < i + 1) {
            currentVictoryStage = i + 1;
            stageVictory = true;
            lastPauseTime = gameClock.getElapsedTime().asSeconds();
            // 应用阶段性胜利强化
            player.applyStageBoost(currentVictoryStage);
            // 更新胜利文本显示当前阶段
            bool isFinal = (currentVictoryStage == STAGE_COUNT);
            std::string victoryStr = (isFinal ? I18n::instance().t("stage_victory_final") : I18n::instance().t("stage_victory")) +
                " " + std::to_string(currentVictoryStage) + "/" + std::to_string(STAGE_COUNT) +
                " (" + std::to_string(STAGE_THRESHOLDS[i]) + ")";
            victoryText.setString(sf::String::fromUtf8(victoryStr.begin(), victoryStr.end()));
            if (isFinal) victoryText.setFillColor(sf::Color::Yellow);
            db.saveCharacter(buildCharacterData());
            db.saveRanking(username, player.levelData.level, player.totalScore);
            return;
        }
    }

    sf::Vector2f move(0,0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) move.y -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) move.y += 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) move.x -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) move.x += 1;
    if (move != sf::Vector2f(0,0)) move = normalize(move);
    if (!player.isRolling) {
        player.velocity = move * player.speed;
    }
    player.update(dt);

    auto newEnemies = mapSystem.checkAndSpawn(player.position);
    float diffScale = mapSystem.getDifficultyScale();
    float timeScale = 1.0f + gameClock.getElapsedTime().asSeconds() / 600.f;
    float scoreScale = 1.0f + player.totalScore / 1000.f;
    float enemyScale = diffScale * timeScale * scoreScale;
    for (auto& spawn : newEnemies) {
        enemies.emplace_back(spawn.type, spawn.position);
        enemies.back().scaleStats(enemyScale);
    }

    for (auto& e : enemies) {
        if (e.isAlive()) {
            e.setTarget(player.position);
            e.update(dt);
        }
    }

    handleEnemyAttacks(dt);
    updateProjectiles(dt);
    updateDrops(dt);
    checkPickups();

    for (auto& e : enemies) {
        if (!e.isAlive() && !e.scoreProcessed) {
            e.scoreProcessed = true;
            spawnDrops(e);
            player.totalScore += e.score;
            player.levelData.gainExp(e.score);
            player.recalcCombatStats();
            killCount++;
        }
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e){ return !e.isAlive(); }), enemies.end());

    // 更新拾取反馈特效
    for (auto& pe : pickupEffects) pe.timer -= dt;
    pickupEffects.erase(std::remove_if(pickupEffects.begin(), pickupEffects.end(),
        [](const PickupEffect& pe){ return pe.timer <= 0; }), pickupEffects.end());

    // 更新伤害数字
    for (auto& dn : damageNumbers) {
        dn.timer -= dt;
        dn.angle += dt * 2.0f;  // 缓慢环绕旋转
    }
    damageNumbers.erase(std::remove_if(damageNumbers.begin(), damageNumbers.end(),
        [](const DamageNumber& dn){ return dn.timer <= 0; }), damageNumbers.end());

    mapSystem.updateCamera(gameView, player.position);
    attrPanel.update(player);
    updateInfoText();
}

void ActualGameView::draw(sf::RenderWindow& window) {
    window.setView(gameView);

    mapSystem.drawBackground(window);

    for (auto& e : enemies) e.drawWithName(window, &globalFont);
    for (auto& proj : projectiles) proj.draw(window);
    for (auto& d : drops) d.draw(window);

    player.drawWithName(window, &globalFont);
    player.drawSlash(window);

    // 拾取反馈特效：向上飘浮的文字
    for (auto& pe : pickupEffects) {
        float progress = 1.0f - pe.timer / 3.0f;
        float yOffset = progress * 40.f;  // 向上飘40像素
        float alpha = pe.timer / 3.0f;
        sf::Text pickText(globalFont, sf::String::fromUtf8(pe.text.begin(), pe.text.end()), 16);
        pickText.setFillColor(sf::Color(pe.color.r, pe.color.g, pe.color.b,
                                        static_cast<std::uint8_t>(alpha * 255)));
        pickText.setPosition(sf::Vector2f(pe.position.x - pickText.getLocalBounds().size.x / 2,
                                          pe.position.y - yOffset - 20));
        window.draw(pickText);
    }

    // 伤害数字：环绕敌人显示
    for (auto& dn : damageNumbers) {
        float alpha = std::min(1.0f, dn.timer / 0.6f);  // 最后0.6秒淡出
        float x = dn.position.x + std::cos(dn.angle) * dn.radius;
        float y = dn.position.y + std::sin(dn.angle) * dn.radius;
        std::string dmgStr = std::to_string(dn.damage);
        int fontSize = dn.isCrit ? 24 : 18;  // 字体更大
        sf::Text dmgText(globalFont, sf::String::fromUtf8(dmgStr.begin(), dmgStr.end()), fontSize);
        sf::Color baseColor = dn.isCrit ? sf::Color::Red : sf::Color::Yellow;  // 黄色普通/红色暴击
        dmgText.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b,
                                        static_cast<std::uint8_t>(alpha * 255)));
        // 暴击加粗描边效果
        if (dn.isCrit) {
            dmgText.setOutlineColor(sf::Color(255, 100, 100, static_cast<std::uint8_t>(alpha * 180)));
            dmgText.setOutlineThickness(2.f);
        }
        dmgText.setPosition(sf::Vector2f(x - dmgText.getLocalBounds().size.x / 2,
                                          y - dmgText.getLocalBounds().size.y / 2));
        window.draw(dmgText);
    }

    window.setView(window.getDefaultView());

    window.draw(infoText);
    backBtn.draw(window);
    attrPanel.draw(window);
    if (gameOver) {
        window.draw(gameOverText);
        restartBtn.draw(window);
    }

    if (stageVictory) {
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);

        window.draw(victoryText);
        victoryContinueBtn.draw(window);
        victoryBackBtn.draw(window);
    }

    if (paused) {
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);

        window.draw(pauseText);
        resumeBtn.draw(window);
        pauseBackBtn.draw(window);
    }
}
