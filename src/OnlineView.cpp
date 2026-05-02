/**
 * @file OnlineView.cpp
 * @brief 联机大厅与联机游戏视图实现（双人合作闯关）
 */

#include "OnlineView.hpp"
#include "Network.hpp"
#include "GameLogic.hpp"
#include "MapSystem.hpp"
#include "I18n.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <sstream>
#include <iomanip>

// ======================== OnlineLobbyView 实现 ========================

OnlineLobbyView::OnlineLobbyView(const sf::Font& font, std::function<void(std::string)> cv, NetworkManager& net)
    : View(font), ipInput(font, {540,150}, {200,40}, false, 15, false),
      portInput(font, {540,220}, {200,40}, false, 5, false),
      createBtn(font, I18n::instance().t("host_coop"), {540,290}, {200,50}, [this,cv](){
            std::string portStr = portInput.getString();
            if (portStr.empty()) {
                std::string err = I18n::instance().t("fill_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) {
                std::string err = I18n::instance().t("invalid_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            network.setPendingPort(port);
            cv("ONLINE_GAME_HOST");
      }),
      joinBtn(font, I18n::instance().t("join_coop"), {540,360}, {200,50}, [this,cv](){
            std::string ip = ipInput.getString();
            std::string portStr = portInput.getString();
            if (ip.empty() || portStr.empty()) {
                std::string err = I18n::instance().t("fill_ip_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) {
                std::string err = I18n::instance().t("invalid_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            auto resolved = sf::IpAddress::resolve(ip);
            if (!resolved) {
                std::string err = I18n::instance().t("invalid_ip");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            network.setPendingPort(port);
            std::string conn = I18n::instance().t("connecting");
            statusText.setString(sf::String::fromUtf8(conn.begin(), conn.end()));
            if (network.connectToServer(*resolved, port)) {
                std::string succ = I18n::instance().t("connected");
                statusText.setString(sf::String::fromUtf8(succ.begin(), succ.end()));
                cv("ONLINE_GAME_CLIENT");
            } else {
                std::string fail = I18n::instance().t("conn_failed");
                statusText.setString(sf::String::fromUtf8(fail.begin(), fail.end()));
            }
      }),
      backBtn(font, I18n::instance().t("back"), {540,430}, {200,50}, [cv](){ cv("SELECT_LEVEL"); }),
      statusText(font, "", 18), changeView(cv), network(net) {
    ipInput.setString("127.0.0.1"); portInput.setString("54000");
    statusText.setPosition({540,500}); statusText.setFillColor(sf::Color::Red);
}

void OnlineLobbyView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    ipInput.handleEvent(event, mPos);
    portInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            createBtn.handleEvent(mPos, true);
            joinBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
        }
    }
}

void OnlineLobbyView::update(sf::Vector2f mousePos, sf::Vector2u) {
    ipInput.updateCursor(); portInput.updateCursor();
    createBtn.handleEvent(mousePos, false);
    joinBtn.handleEvent(mousePos, false);
    backBtn.handleEvent(mousePos, false);
}

void OnlineLobbyView::draw(sf::RenderWindow& window) {
    std::string ipLabelStr = I18n::instance().t("server_ip");
    sf::Text ipLabel(globalFont, sf::String::fromUtf8(ipLabelStr.begin(), ipLabelStr.end()), 18);
    ipLabel.setPosition({540,125}); ipLabel.setFillColor(sf::Color::Black); window.draw(ipLabel);

    std::string portLabelStr = I18n::instance().t("port");
    sf::Text portLabel(globalFont, sf::String::fromUtf8(portLabelStr.begin(), portLabelStr.end()), 18);
    portLabel.setPosition({540,195}); portLabel.setFillColor(sf::Color::Black); window.draw(portLabel);

    ipInput.draw(window); portInput.draw(window);
    createBtn.draw(window); joinBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
}

// ======================== OnlineGameView 实现 ========================

OnlineGameView::OnlineGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               NetworkManager& net, bool host, const std::string& username,
                               Database& database)
    : View(font), myUsername(username), changeView(cv), network(net), db(database), isHost(host),
      attrPanel(font),
      statusText(font, "", 18),
      infoText(font, "", 20),
      gameResultText(font, "", 30),
      victoryText(font, "", 40) {
    backBtn = std::make_unique<Button>(font, I18n::instance().t("back_menu"), sf::Vector2f{1150,20}, sf::Vector2f{120,40},
                                       [cv](){ cv("SELECT_LEVEL"); });
    startBtn = std::make_unique<Button>(font, I18n::instance().t("start_game"), sf::Vector2f{540,350}, sf::Vector2f{200,50},
                                        [this](){
                                            gameStarted = true;
                                            sf::Packet p;
                                            p << static_cast<uint8_t>(NetMsgType::GameStart);
                                            network.broadcast(p);
                                        });
    victoryContinueBtn = std::make_unique<Button>(font, I18n::instance().t("continue"), sf::Vector2f{540,400}, sf::Vector2f{100,50},
                                                  [this](){
                                                      float currentTime = gameClock.getElapsedTime().asSeconds();
                                                      pausedTime += (currentTime - lastPauseTime);
                                                      stageVictory = false;
                                                  });
    victoryBackBtn = std::make_unique<Button>(font, I18n::instance().t("back_menu"), sf::Vector2f{640,400}, sf::Vector2f{100,50},
                                              [this, cv](){
                                                  if (isHost && coopPlayers.count(myId)) {
                                                      db.saveRanking(myUsername, coopPlayers[myId].levelData.level, coopPlayers[myId].totalScore);
                                                  }
                                                  cv("SELECT_LEVEL");
                                              });
    statusText.setPosition({10,550}); statusText.setFillColor(sf::Color::Red);
    infoText.setPosition({10,10}); infoText.setFillColor(sf::Color::Black);
    gameResultText.setPosition({490, 300}); gameResultText.setFillColor(sf::Color::Blue);
    gameResultText.setCharacterSize(40);
    victoryText.setPosition({440,280}); victoryText.setFillColor(sf::Color::Green);
    victoryText.setCharacterSize(30);

    gameView.setSize(sf::Vector2f(800, 600));
    gameView.setCenter({400, 300});

    if (isHost) {
        unsigned short port = network.getPendingPort();
        if (!network.startServer(port)) {
            std::string err = I18n::instance().t("server_start_failed") + std::to_string(port);
            statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
        } else {
            std::string succ = I18n::instance().t("server_started") + std::to_string(port);
            statusText.setString(sf::String::fromUtf8(succ.begin(), succ.end()));
            connected = true;
        }
        myId = 0;
        playerNames[0] = myUsername;

        // 从角色存档加载主机玩家
        Player hostPlayer({400, 300});
        hostPlayer.username = myUsername;

        int slot = db.getSelectedSlot(myUsername);
        auto charData = db.loadCharacter(myUsername, slot);
        if (charData) {
            hostPlayer.baseAttributes = charData->attributes;
            hostPlayer.charClass = charData->classType;
            hostPlayer.classConfig = getClassConfig(charData->classType);
            hostPlayer.levelData.level = charData->level;
            hostPlayer.levelData.experience = charData->experience;
            hostPlayer.levelData.expToNextLevel = 100 * charData->level;
            hostPlayer.totalScore = charData->score;
            currentVictoryStage = charData->victoryStage;
        } else {
            hostPlayer.baseAttributes = generateRandomAttributes();
            hostPlayer.charClass = CharacterClass::Warrior;
            hostPlayer.classConfig = getClassConfig(CharacterClass::Warrior);
        }

        hostPlayer.weaponList.initDefaultWeapons();
        auto w = hostPlayer.weaponList.findWeaponByType(hostPlayer.classConfig.allowedWeapon);
        if (w) hostPlayer.equipWeapon(w->name);
        hostPlayer.recalcCombatStats();
        hostPlayer.health = hostPlayer.maxHealth;
        coopPlayers[0] = std::move(hostPlayer);
    } else {
        myId = -1;
        std::string conn = I18n::instance().t("connecting_server");
        statusText.setString(sf::String::fromUtf8(conn.begin(), conn.end()));
    }
}

// ---- 网络通信 ----

void OnlineGameView::sendInputToServer() {
    if (inputSendClock.getElapsedTime().asSeconds() < 0.033f) return;  // ~30Hz
    inputSendClock.restart();

    if (isHost) {
        if (coopPlayers.count(myId)) {
            auto& me = coopPlayers[myId];
            sf::Vector2f move(0,0);
            if (currentInput.up) move.y -= 1;
            if (currentInput.down) move.y += 1;
            if (currentInput.left) move.x -= 1;
            if (currentInput.right) move.x += 1;
            if (move != sf::Vector2f(0,0)) move = normalize(move);
            if (!me.isRolling) {
                me.velocity = move * me.speed;
            }
        }
    } else {
        if (!connected || !network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerInput) << myId
          << currentInput.up << currentInput.down << currentInput.left << currentInput.right;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::sendAttackSignal() {
    if (attackSendClock.getElapsedTime().asSeconds() < 0.1f) return;
    attackSendClock.restart();

    if (isHost) {
        performAttack(myId);
    } else {
        if (!connected || !network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerAttack) << myId;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::broadcastFullState() {
    sf::Packet p;
    p << static_cast<uint8_t>(NetMsgType::GameState);

    // 玩家数据（含名字）
    p << static_cast<uint32_t>(coopPlayers.size());
    for (auto& [id, pl] : coopPlayers) {
        std::string name = playerNames.count(id) ? playerNames[id] : pl.username;
        p << id << pl.position.x << pl.position.y << pl.health << pl.maxHealth
          << pl.totalScore << pl.levelData.level << name;
    }

    // 敌人数据
    p << static_cast<uint32_t>(enemies.size());
    for (auto& e : enemies) {
        p << static_cast<int>(e.enemyType) << e.position.x << e.position.y
          << e.health << e.maxHealth << e.radius << e.score;
    }

    // 投射物数据（含颜色）
    p << static_cast<uint32_t>(projectiles.size());
    for (auto& proj : projectiles) {
        p << proj.position.x << proj.position.y << proj.direction.x << proj.direction.y
          << proj.speed << proj.damage << proj.maxDistance << proj.traveled
          << proj.fromPlayer
          << static_cast<uint8_t>(proj.color.r) << static_cast<uint8_t>(proj.color.g)
          << static_cast<uint8_t>(proj.color.b);
    }

    // 掉落物数据
    p << static_cast<uint32_t>(drops.size());
    for (auto& d : drops) {
        p << static_cast<int>(d.type) << d.position.x << d.position.y
          << d.lifetime << d.bobTimer;
    }

    network.broadcast(p);
}

void OnlineGameView::processNetwork() {
    for (int i = 0; i < 10; ++i) {
        sf::Packet packet;
        auto result = network.receive(packet);
        if (!result) break;
        auto [type, sender] = *result;
        auto [ip, port] = sender;

        if (isHost) {
            if (type == NetMsgType::Connect) {
                if (!network.hasClient(ip, port)) {
                    int newId = nextPlayerId++;
                    network.addClient(ip, port);

                    // 从Connect包中读取客户端角色数据
                    int classType; std::string clientName;
                    packet >> classType >> clientName;

                    playerNames[newId] = clientName;

                    Player newPlayer({400, 300});
                    newPlayer.username = clientName;
                    newPlayer.baseAttributes = generateRandomAttributes();
                    newPlayer.charClass = static_cast<CharacterClass>(classType);
                    newPlayer.classConfig = getClassConfig(static_cast<CharacterClass>(classType));
                    newPlayer.weaponList.initDefaultWeapons();
                    auto w = newPlayer.weaponList.findWeaponByType(newPlayer.classConfig.allowedWeapon);
                    if (w) newPlayer.equipWeapon(w->name);
                    newPlayer.recalcCombatStats();
                    newPlayer.health = newPlayer.maxHealth;
                    coopPlayers[newId] = std::move(newPlayer);

                    sf::Packet accept;
                    accept << static_cast<uint8_t>(NetMsgType::ConnectAccept) << newId;
                    network.send(accept, ip, port);

                    std::string msg = clientName + " connected!";
                    statusText.setString(sf::String::fromUtf8(msg.begin(), msg.end()));
                }
            }
            else if (type == NetMsgType::Disconnect) {
                int id; packet >> id;
                if (!packet) break;
                network.removeClient(ip, port);
                coopPlayers.erase(id);
                playerNames.erase(id);
            }
            else if (type == NetMsgType::PlayerInput) {
                int id; bool u,d,l,r;
                packet >> id >> u >> d >> l >> r;
                if (!packet) break;
                if (coopPlayers.count(id)) {
                    auto& pl = coopPlayers[id];
                    sf::Vector2f move(0,0);
                    if (u) move.y -= 1;
                    if (d) move.y += 1;
                    if (l) move.x -= 1;
                    if (r) move.x += 1;
                    if (move != sf::Vector2f(0,0)) move = normalize(move);
                    if (!pl.isRolling) {
                        pl.velocity = move * pl.speed;
                    }
                }
            }
            else if (type == NetMsgType::PlayerAttack) {
                int attackerId; packet >> attackerId;
                if (!packet) break;
                performAttack(attackerId);
            }
            else if (type == NetMsgType::PlayerRoll) {
                int rollerId; float dirX, dirY;
                packet >> rollerId >> dirX >> dirY;
                if (!packet) break;
                if (coopPlayers.count(rollerId)) {
                    coopPlayers[rollerId].startRoll(sf::Vector2f(dirX, dirY));
                }
            }
            else if (type == NetMsgType::PauseSync) {
                bool p; packet >> p;
                if (!packet) break;
                paused = p;
                if (p) lastPauseTime = gameClock.getElapsedTime().asSeconds();
                else pausedTime += (gameClock.getElapsedTime().asSeconds() - lastPauseTime);
                sf::Packet bp;
                bp << static_cast<uint8_t>(NetMsgType::PauseSync) << paused;
                network.broadcast(bp);
            }
        } else {
            // 客户端
            if (type == NetMsgType::ConnectAccept) {
                packet >> myId;
                if (!packet) break;
                connected = true;
                std::string msg = I18n::instance().t("connected") + " ID=" + std::to_string(myId);
                statusText.setString(sf::String::fromUtf8(msg.begin(), msg.end()));

                // 发送自己的角色数据给主机
                if (network.getServerAddr().has_value()) {
                    int slot = db.getSelectedSlot(myUsername);
                    auto charData = db.loadCharacter(myUsername, slot);
                    int myClass = charData ? static_cast<int>(charData->classType) : 0;

                    sf::Packet charPkt;
                    charPkt << static_cast<uint8_t>(NetMsgType::Connect)
                            << myClass << myUsername;
                    network.send(charPkt, *network.getServerAddr(), network.getServerPort());
                }
            }
            else if (type == NetMsgType::GameStart) {
                gameStarted = true;
            }
            else if (type == NetMsgType::PauseSync) {
                bool p; packet >> p;
                if (!packet) break;
                paused = p;
                if (p) lastPauseTime = gameClock.getElapsedTime().asSeconds();
                else pausedTime += (gameClock.getElapsedTime().asSeconds() - lastPauseTime);
            }
            else if (type == NetMsgType::GameState) {
                // 解析玩家数据（含名字）
                syncPlayers.clear();
                uint32_t playerCount; packet >> playerCount;
                if (!packet || playerCount > 16) break;
                for (uint32_t i = 0; i < playerCount; ++i) {
                    SyncPlayer sp;
                    int id;
                    packet >> id >> sp.posX >> sp.posY >> sp.health >> sp.maxHealth
                           >> sp.totalScore >> sp.level >> sp.name;
                    if (!packet) { syncPlayers.clear(); break; }
                    syncPlayers[id] = sp;
                    playerNames[id] = sp.name;
                }
                if (!packet) break;

                // 解析敌人数据
                syncEnemies.clear();
                uint32_t enemyCount; packet >> enemyCount;
                if (!packet || enemyCount > 1024) break;
                for (uint32_t i = 0; i < enemyCount; ++i) {
                    SyncEnemy se;
                    packet >> se.type >> se.posX >> se.posY >> se.health >> se.maxHealth
                           >> se.radius >> se.score;
                    if (!packet) { syncEnemies.clear(); break; }
                    syncEnemies.push_back(se);
                }
                if (!packet) break;

                // 解析投射物数据（含颜色）
                syncProjectiles.clear();
                uint32_t projCount; packet >> projCount;
                if (!packet || projCount > 1024) break;
                for (uint32_t i = 0; i < projCount; ++i) {
                    SyncProjectile sp;
                    packet >> sp.posX >> sp.posY >> sp.dirX >> sp.dirY
                           >> sp.speed >> sp.damage >> sp.maxDistance >> sp.traveled
                           >> sp.fromPlayer >> sp.colorR >> sp.colorG >> sp.colorB;
                    if (!packet) { syncProjectiles.clear(); break; }
                    syncProjectiles.push_back(sp);
                }
                if (!packet) break;

                // 解析掉落物数据
                syncDrops.clear();
                uint32_t dropCount; packet >> dropCount;
                if (!packet || dropCount > 256) break;
                for (uint32_t i = 0; i < dropCount; ++i) {
                    SyncDrop sd;
                    packet >> sd.type >> sd.posX >> sd.posY >> sd.lifetime >> sd.bobTimer;
                    if (!packet) { syncDrops.clear(); break; }
                    syncDrops.push_back(sd);
                }

                // 检查自己是否死亡
                if (syncPlayers.count(myId)) {
                    if (syncPlayers[myId].health <= 0 && !gameOver) {
                        gameOver = true;
                        std::string lost = I18n::instance().t("you_lost");
                        gameResultText.setString(sf::String::fromUtf8(lost.begin(), lost.end()));
                    }
                }
            }
            else if (type == NetMsgType::Disconnect) {
                int id; packet >> id;
                syncPlayers.erase(id);
                playerNames.erase(id);
            }
        }
    }
}

// ---- 主机端游戏逻辑 ----

void OnlineGameView::hostUpdateGame(float dt) {
    for (auto& [id, pl] : coopPlayers) {
        pl.update(dt);
    }

    if (gameStarted) {
        for (auto& [id, pl] : coopPlayers) {
            auto newEnemies = mapSystem.checkAndSpawn(pl.position);
            float diffScale = mapSystem.getDifficultyScale();
            float timeScale = 1.0f + gameClock.getElapsedTime().asSeconds() / 600.f;
            float scoreScale = 1.0f + pl.totalScore / 1000.f;
            float enemyScale = diffScale * timeScale * scoreScale;
            for (auto& spawn : newEnemies) {
                enemies.emplace_back(spawn.type, spawn.position);
                enemies.back().scaleStats(enemyScale);
            }
        }
    }

    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        sf::Vector2f closestPos = e.position;
        float minDist = std::numeric_limits<float>::max();
        for (auto& [id, pl] : coopPlayers) {
            float d = distance(e.position, pl.position);
            if (d < minDist) { minDist = d; closestPos = pl.position; }
        }
        e.setTarget(closestPos);
        e.update(dt);
    }

    hostHandleEnemyAttacks(dt);
    hostUpdateProjectiles(dt);
    hostProcessKill();
    updateDrops(dt);
    checkPickups();
}

void OnlineGameView::hostHandleEnemyAttacks(float dt) {
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        for (auto& [id, pl] : coopPlayers) {
            if (e.isRanged) {
                if (e.canAttack() && distance(e.position, pl.position) <= e.attackRange) {
                    Projectile proj;
                    proj.position = e.position;
                    proj.direction = normalize(pl.position - e.position);
                    proj.speed = 250.f;
                    proj.damage = e.attackDamage;
                    proj.maxDistance = e.attackRange;
                    proj.traveled = 0.f;
                    proj.fromPlayer = false;
                    proj.color = sf::Color::Magenta;
                    projectiles.push_back(proj);
                    e.resetAttackCooldown();
                }
            } else {
                if (distance(pl.position, e.position) < pl.radius + e.radius && e.canAttack()) {
                    if (!pl.isInvincible) {
                        pl.health = std::max(0, pl.health - e.attackDamage);
                        e.resetAttackCooldown();
                        sf::Vector2f dir = pl.position - e.position;
                        if (dir != sf::Vector2f(0,0)) dir = normalize(dir);
                        pl.position += dir * 20.f;
                        if (pl.health <= 0 && id == myId) {
                            gameOver = true;
                            std::string lost = I18n::instance().t("you_lost");
                            gameResultText.setString(sf::String::fromUtf8(lost.begin(), lost.end()));
                        }
                    }
                }
            }
        }
    }
}

void OnlineGameView::hostUpdateProjectiles(float dt) {
    for (auto& proj : projectiles) proj.update(dt);

    for (auto& proj : projectiles) {
        if (proj.fromPlayer) {
            for (auto& e : enemies) {
                if (!e.isAlive()) continue;
                if (distance(proj.position, e.position) < e.radius + 4.f) {
                    e.health = std::max(0, e.health - proj.damage);
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
            for (auto& [id, pl] : coopPlayers) {
                if (distance(proj.position, pl.position) < pl.radius + 4.f) {
                    if (!pl.isInvincible) {
                        pl.health = std::max(0, pl.health - proj.damage);
                        proj.traveled = proj.maxDistance;
                        if (pl.health <= 0 && id == myId) {
                            gameOver = true;
                            std::string lost = I18n::instance().t("you_lost");
                            gameResultText.setString(sf::String::fromUtf8(lost.begin(), lost.end()));
                        }
                    } else {
                        proj.traveled = proj.maxDistance;
                    }
                    break;
                }
            }
        }
    }

    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p){ return p.isExpired(); }), projectiles.end());
}

void OnlineGameView::hostProcessKill() {
    for (auto& e : enemies) {
        if (!e.isAlive() && !e.scoreProcessed) {
            e.scoreProcessed = true;
            spawnDrops(e);
            // 合作模式：所有玩家共享击杀分数和经验
            for (auto& [id, pl] : coopPlayers) {
                pl.totalScore += e.score;
                pl.levelData.gainExp(e.score);
                pl.recalcCombatStats();
            }
            killCount++;
        }
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e){ return !e.isAlive(); }), enemies.end());
}

// ---- 掉落物系统 ----

void OnlineGameView::spawnDrops(const Enemy& enemy) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    float dropChance = 0.40f;
    if (enemy.enemyType == EnemyType::Giant) dropChance = 0.80f;
    else if (enemy.enemyType == EnemyType::SkeletonWarrior) dropChance = 0.55f;
    else if (enemy.enemyType == EnemyType::SkeletonArcher) dropChance = 0.45f;

    if (dist(gen) < dropChance) {
        DropItem item;
        item.position = enemy.position;
        float r = dist(gen);
        if (r < 0.30f)      item.type = DropType::HealthPotion;
        else if (r < 0.45f) item.type = DropType::StrBoost;
        else if (r < 0.60f) item.type = DropType::AgiBoost;
        else if (r < 0.75f) item.type = DropType::MagBoost;
        else if (r < 0.88f) item.type = DropType::VitBoost;
        else                item.type = DropType::RangeBoost;
        drops.push_back(item);
    }
}

void OnlineGameView::updateDrops(float dt) {
    for (auto& d : drops) d.update(dt);
    drops.erase(std::remove_if(drops.begin(), drops.end(),
        [](const DropItem& d){ return d.lifetime <= 0; }), drops.end());
}

void OnlineGameView::checkPickups() {
    float pickupRange = 30.f;
    for (auto& [pid, pl] : coopPlayers) {
        for (auto it = drops.begin(); it != drops.end(); ) {
            float dist = distance(pl.position, it->position);
            if (dist < pickupRange + pl.radius) {
                PickupEffect effect;
                effect.position = it->position;
                effect.color = it->getColor();
                effect.timer = 3.0f;

                switch (it->type) {
                    case DropType::HealthPotion: {
                        int heal = pl.maxHealth / 3;
                        pl.health = std::min(pl.maxHealth, pl.health + heal);
                        effect.text = "HP+" + std::to_string(heal);
                        break;
                    }
                    case DropType::StrBoost:
                        pl.baseAttributes.strength = std::min(30, pl.baseAttributes.strength + 2);
                        effect.text = "STR+2";
                        break;
                    case DropType::AgiBoost:
                        pl.baseAttributes.agility = std::min(30, pl.baseAttributes.agility + 2);
                        effect.text = "AGI+2";
                        break;
                    case DropType::MagBoost:
                        pl.baseAttributes.magic = std::min(30, pl.baseAttributes.magic + 2);
                        effect.text = "MAG+2";
                        break;
                    case DropType::VitBoost:
                        pl.baseAttributes.vitality = std::min(30, pl.baseAttributes.vitality + 2);
                        effect.text = "VIT+2";
                        break;
                    case DropType::RangeBoost:
                        pl.attackRangeBonus += 15.f;
                        effect.text = "RNG+15";
                        break;
                }
                pl.recalcCombatStats();
                pickupEffects.push_back(effect);
                it = drops.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// ---- 攻击逻辑 ----

void OnlineGameView::performAttack(int playerId) {
    if (!coopPlayers.count(playerId)) return;
    auto& pl = coopPlayers[playerId];
    if (!pl.canAttack()) return;

    if (pl.equippedWeapon && pl.equippedWeapon->isRanged) {
        sf::Vector2f dir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(pl.position, e.position);
            if (d < minDist) { minDist = d; dir = normalize(e.position - pl.position); }
        }
        Projectile proj;
        proj.position = pl.position;
        proj.direction = dir;
        proj.speed = 400.f;
        proj.damage = pl.calculateDamage();
        proj.maxDistance = pl.attackRange;
        proj.traveled = 0.f;
        proj.fromPlayer = true;
        proj.color = sf::Color::Red;  // 法师火球红色
        proj.explosionRadius = pl.projectileExplosionRadius;  // 阶段强化爆炸半径
        proj.isCrit = pl.lastAttackWasCrit;  // 暴击标记
        projectiles.push_back(proj);
    } else {
        sf::Vector2f attackDir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(pl.position, e.position);
            if (d < minDist) { minDist = d; attackDir = normalize(e.position - pl.position); }
        }
        float attackAngle = std::atan2(attackDir.y, attackDir.x);
        pl.startSlash(attackAngle);

        float halfArc = 1.57f;
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float dist = distance(pl.position, e.position);
            if (dist <= pl.attackRange) {
                sf::Vector2f toEnemy = normalize(e.position - pl.position);
                float enemyAngle = std::atan2(toEnemy.y, toEnemy.x);
                float angleDiff = std::abs(std::atan2(std::sin(enemyAngle - attackAngle), std::cos(enemyAngle - attackAngle)));
                if (angleDiff <= halfArc) {
                    int dmg = pl.calculateDamage();
                    e.health = std::max(0, e.health - dmg);
                    // 生成伤害数字
                    DamageNumber dn;
                    dn.position = e.position;
                    dn.damage = dmg;
                    dn.isCrit = pl.lastAttackWasCrit;
                    dn.angle = enemyAngle;
                    dn.radius = e.radius + 10.f;
                    damageNumbers.push_back(dn);
                }
            }
        }
    }
    pl.resetAttackCooldown();
}

// ---- 渲染 ----

void OnlineGameView::drawGameWorld(sf::RenderWindow& window) {
    mapSystem.drawBackground(window);

    if (isHost) {
        for (auto& e : enemies) e.drawWithName(window, &globalFont);
        for (auto& proj : projectiles) proj.draw(window);
        for (auto& d : drops) d.draw(window);
    } else {
        // 客户端：绘制同步的敌人
        for (auto& se : syncEnemies) {
            if (se.health <= 0) continue;
            float size = se.radius;
            sf::RectangleShape diamond(sf::Vector2f(size * 1.414f, size * 1.414f));
            diamond.setOrigin(sf::Vector2f(size * 1.414f / 2, size * 1.414f / 2));
            diamond.setPosition(sf::Vector2f(se.posX, se.posY));
            diamond.setRotation(sf::degrees(45.f));
            auto eType = static_cast<EnemyType>(se.type);
            switch (eType) {
                case EnemyType::Slime: diamond.setFillColor(sf::Color(0, 200, 0)); break;
                case EnemyType::SkeletonWarrior: diamond.setFillColor(sf::Color(200, 200, 200)); break;
                case EnemyType::SkeletonArcher: diamond.setFillColor(sf::Color(180, 180, 140)); break;
                case EnemyType::Giant: diamond.setFillColor(sf::Color(139, 90, 43)); break;
            }
            window.draw(diamond);

            float barW = se.radius * 2, barH = 5.f;
            sf::RectangleShape bg(sf::Vector2f(barW, barH));
            bg.setFillColor(sf::Color::Red);
            bg.setPosition(sf::Vector2f(se.posX - barW/2, se.posY - se.radius - 12.f));
            window.draw(bg);
            float pct = (se.maxHealth > 0) ? static_cast<float>(se.health) / se.maxHealth : 0.f;
            sf::RectangleShape bar(sf::Vector2f(barW * pct, barH));
            bar.setFillColor(sf::Color::Green);
            bar.setPosition(sf::Vector2f(se.posX - barW/2, se.posY - se.radius - 12.f));
            window.draw(bar);
        }

        // 绘制同步的投射物（使用正确颜色）
        for (auto& sp : syncProjectiles) {
            sf::CircleShape shape(4.f);
            shape.setPosition(sf::Vector2f(sp.posX - 4.f, sp.posY - 4.f));
            shape.setFillColor(sf::Color(sp.colorR, sp.colorG, sp.colorB));
            window.draw(shape);
        }

        // 绘制同步的掉落物
        for (auto& sd : syncDrops) {
            if (sd.lifetime <= 0) continue;
            float bobOffset = std::sin(sd.bobTimer * 3.f) * 3.f;
            sf::Vector2f drawPos(sd.posX, sd.posY + bobOffset);
            auto dtype = static_cast<DropType>(sd.type);
            sf::Color col = sf::Color::White;  // 默认白色，防止未初始化
            switch (dtype) {
                case DropType::HealthPotion: col = sf::Color(255, 50, 50); break;
                case DropType::StrBoost:     col = sf::Color(255, 165, 0); break;
                case DropType::AgiBoost:     col = sf::Color(0, 200, 255); break;
                case DropType::MagBoost:     col = sf::Color(180, 0, 255); break;
                case DropType::VitBoost:     col = sf::Color(0, 255, 100); break;
                case DropType::RangeBoost:   col = sf::Color(255, 255, 0); break;
                default: break;
            }
            sf::CircleShape glow(12.f);
            glow.setPosition(drawPos - sf::Vector2f(12.f, 12.f));
            glow.setFillColor(sf::Color(col.r, col.g, col.b, 60));
            window.draw(glow);
            sf::CircleShape inner(8.f);
            inner.setPosition(drawPos - sf::Vector2f(8.f, 8.f));
            inner.setFillColor(col);
            window.draw(inner);
            sf::CircleShape center(3.f);
            center.setPosition(drawPos - sf::Vector2f(3.f, 3.f));
            center.setFillColor(sf::Color(255, 255, 255, 200));
            window.draw(center);
        }
    }
}

void OnlineGameView::drawPlayers(sf::RenderWindow& window) {
    std::vector<sf::Color> colors = {sf::Color::Cyan, sf::Color::Blue, sf::Color::Magenta,
                                     sf::Color::Green, sf::Color::Yellow, sf::Color(255, 165, 0)};

    if (isHost) {
        int colorIdx = 0;
        for (auto& [id, pl] : coopPlayers) {
            sf::Color drawColor = (id == myId) ? sf::Color::Cyan : colors[colorIdx % colors.size()];
            if (id != myId) ++colorIdx;

            sf::CircleShape shape(pl.radius);
            shape.setPosition(pl.position - sf::Vector2f(pl.radius, pl.radius));
            shape.setFillColor(pl.isRolling ? sf::Color(drawColor.r, drawColor.g, drawColor.b, 128) : drawColor);
            window.draw(shape);

            if (playerNames.count(id)) {
                sf::Text nameText(globalFont, sf::String::fromUtf8(playerNames[id].begin(), playerNames[id].end()), 12);
                nameText.setFillColor(sf::Color::White);
                nameText.setPosition(sf::Vector2f(pl.position.x - nameText.getLocalBounds().size.x / 2, pl.position.y + pl.radius + 2));
                window.draw(nameText);
            }

            float barW = 40.f, barH = 6.f;
            sf::RectangleShape bg(sf::Vector2f(barW, barH));
            bg.setFillColor(sf::Color::Red);
            bg.setPosition(sf::Vector2f(pl.position.x - barW/2, pl.position.y - 24));
            window.draw(bg);
            float pct = (pl.maxHealth > 0) ? static_cast<float>(pl.health) / pl.maxHealth : 0.f;
            sf::RectangleShape bar(sf::Vector2f(barW * pct, barH));
            bar.setFillColor(sf::Color::Green);
            bar.setPosition(sf::Vector2f(pl.position.x - barW/2, pl.position.y - 24));
            window.draw(bar);

            pl.drawSlash(window);
        }
    } else {
        int colorIdx = 0;
        for (auto& [id, sp] : syncPlayers) {
            sf::Color drawColor = (id == myId) ? sf::Color::Cyan : colors[colorIdx % colors.size()];
            if (id != myId) ++colorIdx;

            sf::CircleShape shape(18.f);
            shape.setPosition(sf::Vector2f(sp.posX - 18, sp.posY - 18));
            shape.setFillColor(drawColor);
            window.draw(shape);

            std::string name = sp.name.empty() ? ("Player " + std::to_string(id)) : sp.name;
            sf::Text nameText(globalFont, sf::String::fromUtf8(name.begin(), name.end()), 12);
            nameText.setFillColor(sf::Color::White);
            nameText.setPosition(sf::Vector2f(sp.posX - nameText.getLocalBounds().size.x / 2, sp.posY + 20));
            window.draw(nameText);

            float barW = 40.f, barH = 6.f;
            sf::RectangleShape bg(sf::Vector2f(barW, barH));
            bg.setFillColor(sf::Color::Red);
            bg.setPosition(sf::Vector2f(sp.posX - barW/2, sp.posY - 24));
            window.draw(bg);
            float pct = (sp.maxHealth > 0) ? static_cast<float>(sp.health) / sp.maxHealth : 0.f;
            sf::RectangleShape bar(sf::Vector2f(barW * pct, barH));
            bar.setFillColor(sf::Color::Green);
            bar.setPosition(sf::Vector2f(sp.posX - barW/2, sp.posY - 24));
            window.draw(bar);
        }
    }

    // 拾取反馈特效
    for (auto& pe : pickupEffects) {
        float progress = 1.0f - pe.timer / 3.0f;
        float yOffset = progress * 40.f;
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
        float alpha = std::min(1.0f, dn.timer / 0.6f);
        float x = dn.position.x + std::cos(dn.angle) * dn.radius;
        float y = dn.position.y + std::sin(dn.angle) * dn.radius;
        std::string dmgStr = std::to_string(dn.damage);
        int fontSize = dn.isCrit ? 24 : 18;
        sf::Text dmgText(globalFont, sf::String::fromUtf8(dmgStr.begin(), dmgStr.end()), fontSize);
        sf::Color baseColor = dn.isCrit ? sf::Color::Red : sf::Color::Yellow;
        dmgText.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b,
                                        static_cast<std::uint8_t>(alpha * 255)));
        if (dn.isCrit) {
            dmgText.setOutlineColor(sf::Color(255, 100, 100, static_cast<std::uint8_t>(alpha * 180)));
            dmgText.setOutlineThickness(2.f);
        }
        dmgText.setPosition(sf::Vector2f(x - dmgText.getLocalBounds().size.x / 2,
                                          y - dmgText.getLocalBounds().size.y / 2));
        window.draw(dmgText);
    }
}

void OnlineGameView::drawUI(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());

    if (backBtn) backBtn->draw(window);
    if (startBtn && isHost && !gameStarted) startBtn->draw(window);
    window.draw(statusText);

    // 信息文本（与闯关模式一致）
    if (isHost && coopPlayers.count(myId)) {
        auto& me = coopPlayers[myId];
        float rawTime = gameClock.getElapsedTime().asSeconds();
        float adjustedTime = rawTime - pausedTime;
        int totalSeconds = static_cast<int>(adjustedTime);
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;

        std::stringstream ss;
        ss << myUsername << "  " << I18n::instance().t("hp") << me.health << "/" << me.maxHealth
           << "  " << I18n::instance().t("lv") << me.levelData.level
           << "  " << I18n::instance().t("score") << me.totalScore
           << "  " << I18n::instance().t("time") << " " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;

        if (me.equippedWeapon) {
            std::string weaponName = me.equippedWeapon->name;
            if (weaponName == "Iron Sword") weaponName = I18n::instance().t("iron_sword");
            else if (weaponName == "Flame Artifact") weaponName = I18n::instance().t("flame_artifact");
            else if (weaponName == "Shadow Dagger") weaponName = I18n::instance().t("shadow_dagger");
            ss << "  " << I18n::instance().t("weapon") << weaponName
               << " (" << I18n::instance().t("range") << static_cast<int>(me.attackRange) << ")";
        }

        float scoreMult = me.getScoreMultiplier();
        if (scoreMult > 1.0f) {
            ss << " x" << std::fixed << std::setprecision(1) << scoreMult;
        }

        std::string info = ss.str();
        infoText.setString(sf::String::fromUtf8(info.begin(), info.end()));
    } else if (syncPlayers.count(myId)) {
        auto& sp = syncPlayers[myId];
        std::stringstream ss;
        ss << myUsername << "  " << I18n::instance().t("hp") << sp.health << "/" << sp.maxHealth
           << "  " << I18n::instance().t("lv") << sp.level
           << "  " << I18n::instance().t("score") << sp.totalScore;
        std::string info = ss.str();
        infoText.setString(sf::String::fromUtf8(info.begin(), info.end()));
    }
    window.draw(infoText);

    attrPanel.draw(window);
    if (gameOver) window.draw(gameResultText);

    // 阶段胜利界面
    if (stageVictory) {
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);

        window.draw(victoryText);
        if (victoryContinueBtn) victoryContinueBtn->draw(window);
        if (victoryBackBtn) victoryBackBtn->draw(window);
    }
}

// ---- 事件处理 ----

void OnlineGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left && backBtn)
            backBtn->handleEvent(mPos, true);
        if (mbp->button == sf::Mouse::Button::Left && startBtn && isHost && !gameStarted)
            startBtn->handleEvent(mPos, true);
        if (mbp->button == sf::Mouse::Button::Left && !gameOver && !paused && gameStarted)
            attackRequested = true;
    }

    // 阶段胜利界面按钮
    if (stageVictory) {
        sf::Vector2f uiMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mbp->button == sf::Mouse::Button::Left) {
                if (victoryContinueBtn) victoryContinueBtn->handleEvent(uiMousePos, true);
                if (victoryBackBtn) victoryBackBtn->handleEvent(uiMousePos, true);
            }
        }
        return;
    }

    if (!gameOver && !paused) {
        currentInput.up    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
        currentInput.down  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
        currentInput.left  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
        currentInput.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Space) attackRequested = true;
            if (kp->code == sf::Keyboard::Key::Tab) attrPanel.toggle();
            if (kp->code == sf::Keyboard::Key::LShift || kp->code == sf::Keyboard::Key::RShift) {
                sf::Vector2f rollDir(0, 0);
                if (currentInput.up) rollDir.y -= 1;
                if (currentInput.down) rollDir.y += 1;
                if (currentInput.left) rollDir.x -= 1;
                if (currentInput.right) rollDir.x += 1;
                if (isHost && coopPlayers.count(myId)) {
                    coopPlayers[myId].startRoll(rollDir);
                } else if (!isHost && connected && network.getServerAddr().has_value()) {
                    sf::Packet p;
                    p << static_cast<uint8_t>(NetMsgType::PlayerRoll)
                      << myId << rollDir.x << rollDir.y;
                    network.send(p, *network.getServerAddr(), network.getServerPort());
                }
            }
            if (kp->code == sf::Keyboard::Key::Escape) {
                paused = !paused;
                if (paused) lastPauseTime = gameClock.getElapsedTime().asSeconds();
                else pausedTime += (gameClock.getElapsedTime().asSeconds() - lastPauseTime);
                if (isHost) {
                    sf::Packet p;
                    p << static_cast<uint8_t>(NetMsgType::PauseSync) << paused;
                    network.broadcast(p);
                } else {
                    if (connected && network.getServerAddr().has_value()) {
                        sf::Packet p;
                        p << static_cast<uint8_t>(NetMsgType::PauseSync) << paused;
                        network.send(p, *network.getServerAddr(), network.getServerPort());
                    }
                }
            }
        }
    }
}

void OnlineGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    if (backBtn) backBtn->handleEvent(mousePos, false);
    if (startBtn && isHost && !gameStarted) startBtn->handleEvent(mousePos, false);
    if (stageVictory) {
        if (victoryContinueBtn) victoryContinueBtn->handleEvent(mousePos, false);
        if (victoryBackBtn) victoryBackBtn->handleEvent(mousePos, false);
        return;
    }

    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    processNetwork();

    if (paused) return;
    if (gameOver) return;

    sendInputToServer();

    if (attackRequested) {
        sendAttackSignal();
        attackRequested = false;
    }

    if (isHost) {
        hostUpdateGame(dt);

        // 检查阶段性胜利
        if (coopPlayers.count(myId)) {
            for (int i = STAGE_COUNT - 1; i >= 0; --i) {
                if (coopPlayers[myId].totalScore >= STAGE_THRESHOLDS[i] && currentVictoryStage < i + 1) {
                    currentVictoryStage = i + 1;
                    stageVictory = true;
                    lastPauseTime = gameClock.getElapsedTime().asSeconds();
                    // 所有玩家应用阶段性胜利强化
                    for (auto& [pid, pl] : coopPlayers) {
                        pl.applyStageBoost(currentVictoryStage);
                    }
                    bool isFinal = (currentVictoryStage == STAGE_COUNT);
                    std::string victoryStr = (isFinal ? I18n::instance().t("stage_victory_final") : I18n::instance().t("stage_victory")) +
                        " " + std::to_string(currentVictoryStage) + "/" + std::to_string(STAGE_COUNT) +
                        " (" + std::to_string(STAGE_THRESHOLDS[i]) + ")";
                    victoryText.setString(sf::String::fromUtf8(victoryStr.begin(), victoryStr.end()));
                    if (isFinal) victoryText.setFillColor(sf::Color::Yellow);
                    break;
                }
            }
        }

        broadcastFullState();

        if (coopPlayers.count(myId)) attrPanel.update(coopPlayers[myId]);
    } else {
        if (syncPlayers.count(myId)) {
            Player tempPlayer({syncPlayers[myId].posX, syncPlayers[myId].posY});
            tempPlayer.health = syncPlayers[myId].health;
            tempPlayer.maxHealth = syncPlayers[myId].maxHealth;
            tempPlayer.totalScore = syncPlayers[myId].totalScore;
            tempPlayer.levelData.level = syncPlayers[myId].level;
            attrPanel.update(tempPlayer);
        }
    }

    // 更新拾取反馈特效
    for (auto& pe : pickupEffects) pe.timer -= dt;
    pickupEffects.erase(std::remove_if(pickupEffects.begin(), pickupEffects.end(),
        [](const PickupEffect& pe){ return pe.timer <= 0; }), pickupEffects.end());

    // 更新伤害数字
    for (auto& dn : damageNumbers) {
        dn.timer -= dt;
        dn.angle += dt * 2.0f;
    }
    damageNumbers.erase(std::remove_if(damageNumbers.begin(), damageNumbers.end(),
        [](const DamageNumber& dn){ return dn.timer <= 0; }), damageNumbers.end());

    // 摄像机跟随
    if (isHost && coopPlayers.count(myId)) {
        mapSystem.updateCamera(gameView, coopPlayers[myId].position);
    } else if (syncPlayers.count(myId)) {
        mapSystem.updateCamera(gameView, sf::Vector2f(syncPlayers[myId].posX, syncPlayers[myId].posY));
    }
}

void OnlineGameView::draw(sf::RenderWindow& window) {
    window.setView(gameView);
    drawGameWorld(window);
    drawPlayers(window);
    drawUI(window);

    // 暂停覆盖
    if (paused) {
        window.setView(window.getDefaultView());
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);
        sf::Text pauseText(globalFont, "", 40);
        std::string pauseStr = I18n::instance().t("paused");
        pauseText.setString(sf::String::fromUtf8(pauseStr.begin(), pauseStr.end()));
        pauseText.setPosition({590,280}); pauseText.setFillColor(sf::Color::Blue);
        window.draw(pauseText);
    }
}
