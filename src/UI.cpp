/**
 * @file UI.cpp
 * @brief 用户界面模块实现
 *
 * 包含所有视图类和控件的具体实现。依赖数据库、网络、游戏逻辑等模块。
 */

#include "UI.hpp"
#include "Database.hpp"
#include "Network.hpp"
#include "GameLogic.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <map>

// ======================== Button 实现 ========================

Button::Button(const sf::Font& font, const std::string& label, sf::Vector2f pos, sf::Vector2f size, Callback onClick)
    : text(font, label, 20), callback(onClick) {
    shape.setSize(size);
    shape.setPosition(pos);
    shape.setFillColor(sf::Color::White);
    shape.setOutlineThickness(2);
    shape.setOutlineColor(sf::Color::Black);
    text.setFillColor(sf::Color::Black);
    sf::FloatRect b = text.getLocalBounds();
    text.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    text.setPosition({pos.x + size.x / 2.f, pos.y + size.y / 2.f});
}

void Button::handleEvent(sf::Vector2f mousePos, bool isClicked) {
    bool hovered = shape.getGlobalBounds().contains(mousePos);
    shape.setFillColor(hovered ? sf::Color(200, 200, 255) : sf::Color::White);
    if (hovered && isClicked && callback) callback();
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(shape);
    window.draw(text);
}

// ======================== InputField 实现 ========================

InputField::InputField(const sf::Font& font, sf::Vector2f pos, sf::Vector2f size,
                       bool passwordMode, size_t maxLen, bool allowSpace)
    : text(font, "", 20), isPassword(passwordMode), maxLength(maxLen), allowSpace(allowSpace) {
    box.setSize(size);
    box.setPosition(pos);
    box.setFillColor(sf::Color::White);
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color(200, 200, 200));
    text.setFillColor(sf::Color::Black);
    text.setPosition({pos.x + 10.f, pos.y + 5.f});
}

void InputField::setString(const std::string& str) {
    content = str.substr(0, maxLength);
    updateDisplayText();
}

void InputField::updateCursor() {
    if (isFocused) {
        if (cursorClock.getElapsedTime().asSeconds() > 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
            updateDisplayText();
        }
    } else {
        if (showCursor) {
            showCursor = false;
            updateDisplayText();
        }
    }
}

void InputField::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        isFocused = box.getGlobalBounds().contains(mousePos);
        if (isFocused) {
            showCursor = true;
            cursorClock.restart();
        } else {
            showCursor = false;
        }
        updateDisplayText();
        box.setOutlineColor(isFocused ? sf::Color::Blue : sf::Color(200, 200, 200));
    }

    if (isFocused) {
        if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
            char32_t unicode = textEvent->unicode;
            if (unicode == U'\b') {                     // 退格键
                if (!content.empty()) content.pop_back();
            } else if (unicode < 128 && unicode >= 32 && content.length() < maxLength) {
                if (allowSpace || unicode != U' ') content += static_cast<char>(unicode);
            }
            updateDisplayText();
        }
    }
}

void InputField::updateDisplayText() {
    if (isPassword) {
        std::string stars(content.length(), '*');
        if (isFocused && showCursor) stars += "_";
        text.setString(stars);
    } else {
        text.setString(content + (isFocused && showCursor ? "_" : ""));
    }
}

void InputField::clear() {
    content.clear();
    updateDisplayText();
}

void InputField::draw(sf::RenderWindow& window) const {
    window.draw(box);
    window.draw(text);
}

std::string InputField::getString() const {
    return content;
}

// ======================== View 基类 ========================

View::View(const sf::Font& font) : globalFont(font) {}

// ======================== LoginView 实现 ========================

LoginView::LoginView(const sf::Font& font, std::function<void(std::string)> changeView,
                     std::string& username, Database& database)
    : View(font), db(database), usernameRef(username),
      usernameInput(globalFont, {300,200}, {200,40}, false, 16, false),
      passwordInput(globalFont, {300,280}, {200,40}, true, 16, false),
      confirmBtn(globalFont, "Login", {300,360}, {90,45},
            [this, changeView]() {
                std::string inputUser = usernameInput.getString();
                std::string inputPass = passwordInput.getString();
                auto cred = db.getCredentials(inputUser);
                if (cred && cred->second == hashPassword(inputPass, cred->first)) {
                    usernameRef = inputUser;
                    changeView("SELECT_LEVEL");
                } else {
                    statusText.setString("Invalid credentials!");
                    passwordInput.clear();
                }
            }),
      backBtn(font, "Back", {410,360}, {90,45}, [changeView](){ changeView("MENU"); }),
      title(font, "LOGIN", 35), statusText(font, "", 18) {
    title.setPosition({350,100}); title.setFillColor(sf::Color::Blue);
    statusText.setPosition({300,330}); statusText.setFillColor(sf::Color::Red);
}

void LoginView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    usernameInput.handleEvent(event, mPos);
    passwordInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            confirmBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
        }
    }
}

void LoginView::update(sf::Vector2f mPos, sf::Vector2u) {
    confirmBtn.handleEvent(mPos, false);
    backBtn.handleEvent(mPos, false);
}

void LoginView::draw(sf::RenderWindow& window) {
    window.draw(title);
    usernameInput.updateCursor(); passwordInput.updateCursor();
    usernameInput.draw(window); passwordInput.draw(window);
    confirmBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
}

// ======================== SignUpView 实现 ========================

SignUpView::SignUpView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database)
    : View(font), db(database),
      usernameInput(globalFont, {300,200}, {200,40}, false, 16, false),
      passwordInput(globalFont, {300,280}, {200,40}, true, 16, false),
      regBtn(globalFont, "Register", {300,360}, {90,45},
            [this, changeView]() {
                std::string user = usernameInput.getString();
                std::string pass = passwordInput.getString();
                if (user.empty() || pass.empty()) {
                    statusText.setString("Username/Password empty!");
                    return;
                }
                if (db.userExists(user)) {
                    statusText.setString("User exists!");
                    return;
                }
                std::string salt = generateRandomSalt();
                std::string hash = hashPassword(pass, salt);
                if (db.addUser(user, salt, hash)) {
                    statusText.setString("Registered! Please login.");
                    changeView("LOGIN");
                } else {
                    statusText.setString("Database error!");
                }
            }),
      backBtn(font, "Back", {410,360}, {90,45}, [changeView](){ changeView("MENU"); }),
      title(font, "SIGN UP", 35), statusText(font, "", 18) {
    statusText.setPosition({300,330}); statusText.setFillColor(sf::Color::Red);
    title.setPosition({330,100}); title.setFillColor(sf::Color::Green);
}

void SignUpView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    usernameInput.handleEvent(event, mPos);
    passwordInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            regBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
        }
    }
}

void SignUpView::update(sf::Vector2f mPos, sf::Vector2u) {
    usernameInput.updateCursor(); passwordInput.updateCursor();
    regBtn.handleEvent(mPos, false); backBtn.handleEvent(mPos, false);
}

void SignUpView::draw(sf::RenderWindow& window) {
    window.draw(title);
    usernameInput.draw(window); passwordInput.draw(window);
    regBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
}

// ======================== MainMenuView 实现 ========================

MainMenuView::MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView) : View(font) {
    buttons.emplace_back(font, "Log In", sf::Vector2f{300,200}, sf::Vector2f{200,50}, [changeView](){ changeView("LOGIN"); });
    buttons.emplace_back(font, "Sign Up", sf::Vector2f{300,280}, sf::Vector2f{200,50}, [changeView](){ changeView("SIGNUP"); });
    buttons.emplace_back(font, "Exit", sf::Vector2f{300,360}, sf::Vector2f{200,50}, [](){ exit(0); });
}

void MainMenuView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2f mPos = window.mapPixelToCoords(mbp->position);
        for (auto& b : buttons) b.handleEvent(mPos, true);
    }
}

void MainMenuView::update(sf::Vector2f mPos, sf::Vector2u) {
    for (auto& b : buttons) b.handleEvent(mPos, false);
}

void MainMenuView::draw(sf::RenderWindow& window) {
    for (auto& b : buttons) b.draw(window);
}

// ======================== LevelSelectView 实现 ========================

LevelSelectView::LevelSelectView(const sf::Font& font, std::function<void(std::string)> cv, bool showWelcome)
    : View(font), changeView(cv), welcomeMsg(font, "", 30) {
    if (showWelcome) {
        welcomeMsg.setString("Welcome, Hero!\nPreparing your adventure...");
        welcomeMsg.setFillColor(sf::Color::Blue);
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
        welcomeMsg.setPosition({400,200});
        currentState = State::Welcome;
    } else {
        currentState = State::ButtonsReady;
        welcomeMsg.setString("Choose Your Mode");
        welcomeMsg.setFillColor(sf::Color::Blue);
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
        welcomeMsg.setPosition({400,200});
    }
    buttons.push_back(std::make_unique<Button>(font, "Level Mode", sf::Vector2f{300,320}, sf::Vector2f{200,50},
        [cv](){ cv("LEVEL_GAME"); }));
    buttons.push_back(std::make_unique<Button>(font, "Online Mode", sf::Vector2f{300,400}, sf::Vector2f{200,50},
        [cv](){ cv("ONLINE_LOBBY"); }));
    buttons.push_back(std::make_unique<Button>(font, "Back", sf::Vector2f{480,480}, sf::Vector2f{200,50},
        [cv](){ cv("MENU"); }));
    buttons.push_back(std::make_unique<Button>(font, "Rankings", sf::Vector2f{120,480}, sf::Vector2f{200,50},
        [cv](){ cv("RANKINGS"); }));
}

void LevelSelectView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (currentState != State::ButtonsReady) return;
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            for (auto& b : buttons) b->handleEvent(mPos, true);
        }
    }
}

void LevelSelectView::update(sf::Vector2f mPos, sf::Vector2u) {
    if (currentState == State::Welcome && timer.getElapsedTime().asSeconds() > delayTime) {
        currentState = State::ButtonsReady;
        welcomeMsg.setString("Choose Your Mode");
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
    }
    if (currentState == State::ButtonsReady) {
        for (auto& b : buttons) b->handleEvent(mPos, false);
    }
}

void LevelSelectView::draw(sf::RenderWindow& window) {
    window.draw(welcomeMsg);
    if (currentState == State::ButtonsReady) {
        for (auto& b : buttons) b->draw(window);
    }
}

// ======================== ActualGameView 实现 ========================

ActualGameView::ActualGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               const std::string& name, Database& database)
    : View(font), db(database), username(name), player({400,300}),
      infoText(font, "", 20),                                 
      gameOverText(font, "GAME OVER\nPress Restart", 40), 
      backBtn(font, "Back to Menu", {650,20}, {120,40}, [cv](){ cv("SELECT_LEVEL"); }),
      restartBtn(font, "Restart", {350,400}, {100,50}, [this](){ restartGame(); }),
      changeView(cv) {
    spawnEnemies();
    infoText.setPosition({10,10}); infoText.setFillColor(sf::Color::Black);
    gameOverText.setPosition({200,300}); gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setCharacterSize(30);
}

void ActualGameView::spawnEnemies() {
    enemies.clear();
    enemies.emplace_back(sf::Vector2f(100,100));
    enemies.emplace_back(sf::Vector2f(700,100));
    enemies.emplace_back(sf::Vector2f(100,500));
    enemies.emplace_back(sf::Vector2f(700,500));
}

void ActualGameView::attack() {
    if (!player.canAttack()) return;
    bool hit = false;
    for (auto& e : enemies) {
        float dist = std::hypot(player.position.x - e.position.x, player.position.y - e.position.y);
        if (dist <= player.attackRange) {
            e.health -= player.attackDamage;
            hit = true;
            if (e.health <= 0) killCount++;
        }
    }
    if (hit) {
        attackEffects.push_back(player.position);
        effectTimer = 0.2f;
    }
    player.resetAttackCooldown();
}

void ActualGameView::updateInfoText(sf::Vector2u windowSize) {
    std::stringstream ss;
    ss << "Player: " << username << "   HP: " << player.health << "/" << player.maxHealth
       << "   Kills: " << killCount
       << "\n[WASD] Move   [Left Click / Space] Attack";
    infoText.setString(ss.str());
}

void ActualGameView::restartGame() {
    player.position = {400, 300};
    player.velocity = {0, 0};
    player.health = player.maxHealth;
    player.killCount = 0;
    player.attackCooldown = 0.f;
    spawnEnemies();
    killCount = 0;
    gameOver = false;
    attackEffects.clear();
    effectTimer = 0.f;
}

void ActualGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
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
        }
    }
}

void ActualGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    backBtn.handleEvent(mousePos, false);
    if (gameOver) {
        restartBtn.handleEvent(mousePos, false);
        return;
    }

    sf::Vector2f move(0,0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) move.y -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) move.y += 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) move.x -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) move.x += 1;
    if (move != sf::Vector2f(0,0)) move = normalize(move);
    player.velocity = move * player.speed;
    player.update(dt, windowSize);

    for (auto& e : enemies) {
        if (e.isAlive()) {
            e.setTarget(player.position);
            e.update(dt);
        }
    }

    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        float dist = std::hypot(player.position.x - e.position.x, player.position.y - e.position.y);
        if (dist < player.radius + e.radius) {
            player.health -= 10;
            sf::Vector2f dir = player.position - e.position;
            if (dir != sf::Vector2f(0,0)) dir = normalize(dir);
            player.position += dir * 20.f;
            player.update(0, windowSize);
            if (player.health <= 0) {
                gameOver = true;
                db.saveScore(username, killCount);
            }
            break;
        }
    }

    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e){ return !e.isAlive(); }), enemies.end());

    if (enemies.empty() && !gameOver) {
        spawnEnemies();
    }

    if (effectTimer > 0) {
        effectTimer -= dt;
        if (effectTimer <= 0) attackEffects.clear();
    }

    updateInfoText(windowSize);
}

void ActualGameView::draw(sf::RenderWindow& window) {
    for (const auto& e : enemies) e.draw(window);
    player.draw(window);
    for (const auto& pos : attackEffects) {
        sf::CircleShape effect(30.f);
        effect.setPosition(pos - sf::Vector2f(30,30));
        effect.setFillColor(sf::Color(255,255,0,128));
        window.draw(effect);
    }
    window.draw(infoText);
    backBtn.draw(window);
    if (gameOver) {
        window.draw(gameOverText);
        restartBtn.draw(window);
    }
}

// ======================== RankingsView 实现 ========================

RankingsView::RankingsView(const sf::Font& font, std::function<void(std::string)> cv, Database& database)
    : View(font), db(database), changeView(cv),
      title(font, "Rankings (Top 10)", 30),
      backBtn(font, "Back", {650,20}, {120,40}, [cv](){ cv("SELECT_LEVEL"); }) {
    title.setPosition({300,50}); title.setFillColor(sf::Color::Blue);
    refreshRankings();
}

void RankingsView::refreshRankings() {
    rankTexts.clear();
    auto records = db.loadRankings();
    if (records.empty()) {
        sf::Text empty(globalFont, "No rankings yet.", 20);
        empty.setPosition({300,150}); empty.setFillColor(sf::Color::Black);
        rankTexts.push_back(empty);
        return;
    }
    int y = 150;
    for (size_t i=0; i<records.size(); ++i) {
        std::stringstream ss;
        ss << (i+1) << ". " << records[i].username << " - " << records[i].score;
        sf::Text line(globalFont, ss.str(), 20);
        line.setPosition({300, static_cast<float>(y)});
        line.setFillColor(sf::Color::Black);
        rankTexts.push_back(line);
        y += 30;
    }
}

void RankingsView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) backBtn.handleEvent(mPos, true);
    }
}

void RankingsView::update(sf::Vector2f mousePos, sf::Vector2u) {
    backBtn.handleEvent(mousePos, false);
}

void RankingsView::draw(sf::RenderWindow& window) {
    window.draw(title);
    for (const auto& t : rankTexts) window.draw(t);
    backBtn.draw(window);
}

// ======================== OnlineLobbyView 实现 ========================

OnlineLobbyView::OnlineLobbyView(const sf::Font& font, std::function<void(std::string)> cv, NetworkManager& net)
    : View(font), ipInput(font, {300,150}, {200,40}, false, 15, false),
      portInput(font, {300,220}, {200,40}, false, 5, false),
      createBtn(font, "Host Game", {300,290}, {200,50}, [this,cv](){
            std::string portStr = portInput.getString();
            if (portStr.empty()) { statusText.setString("Fill Port"); return; }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) { statusText.setString("Invalid port"); return; }
            network.setPendingPort(port);
            cv("ONLINE_GAME_HOST");
      }),
      joinBtn(font, "Join Game", {300,360}, {200,50}, [this,cv](){
            std::string ip = ipInput.getString();
            std::string portStr = portInput.getString();
            if (ip.empty() || portStr.empty()) { statusText.setString("Fill IP and Port"); return; }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) { statusText.setString("Invalid port"); return; }
            auto resolved = sf::IpAddress::resolve(ip);
            if (!resolved) { statusText.setString("Invalid IP"); return; }
            network.setPendingPort(port);
            statusText.setString("Connecting...");
            if (network.connectToServer(*resolved, port)) {
                statusText.setString("Connected!");
                cv("ONLINE_GAME_CLIENT");
            } else statusText.setString("Connection failed");
      }),
      backBtn(font, "Back", {300,430}, {200,50}, [cv](){ cv("SELECT_LEVEL"); }),
      statusText(font, "", 18), changeView(cv), network(net) {
    ipInput.setString("127.0.0.1"); portInput.setString("54000");
    statusText.setPosition({300,500}); statusText.setFillColor(sf::Color::Red);
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
    sf::Text ipLabel(globalFont, "Server IP:", 18); ipLabel.setPosition({300,125}); ipLabel.setFillColor(sf::Color::Black); window.draw(ipLabel);
    sf::Text portLabel(globalFont, "Port:", 18); portLabel.setPosition({300,195}); portLabel.setFillColor(sf::Color::Black); window.draw(portLabel);
    ipInput.draw(window); portInput.draw(window);
    createBtn.draw(window); joinBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
}

// ======================== OnlineGameView 实现 ========================

OnlineGameView::OnlineGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               NetworkManager& net, bool host, const std::string& username)
    : View(font), myUsername(username), changeView(cv), network(net), isHost(host),
      statusText(font, "", 18),
      myNameText(font, "", 20),
      gameResultText(font, "", 30)
{
    backBtn = std::make_unique<Button>(font, "Back to Menu", sf::Vector2f{650,20}, sf::Vector2f{120,40},
                                       [cv](){ cv("SELECT_LEVEL"); });
    statusText.setPosition({10,550}); statusText.setFillColor(sf::Color::Red);
    myNameText.setPosition({50, 50}); myNameText.setFillColor(sf::Color::Green);
    gameResultText.setPosition({300, 300}); gameResultText.setFillColor(sf::Color::Blue);
    gameResultText.setCharacterSize(40);

    if (isHost) {
        unsigned short port = network.getPendingPort();
        if (!network.startServer(port)) {
            statusText.setString("Failed to start server on port " + std::to_string(port));
        }
        else {
            statusText.setString("Server started on port " + std::to_string(port));
            connected = true;
        }
        world.addPlayer(0, {400,300});
        myId = 0;
        playerNames[0] = myUsername + " (Host)";
        myNameText.setString(playerNames[0]);
    } else {
        myId = -1;
        myNameText.setString(myUsername + " (Client)");
        statusText.setString("Connecting to server...");
    }
    playerPositions[0] = {400, 300};
}

void OnlineGameView::updateLocalInput() {
    currentInput.up    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    currentInput.down  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    currentInput.left  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    currentInput.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
}

void OnlineGameView::sendInputToServer() {
    if (inputSendClock.getElapsedTime().asSeconds() < 0.05f) return;
    inputSendClock.restart();

    if (isHost) {
        // 主机直接设置自己的输入
        world.setPlayerInput(myId, currentInput);
    } else {
        // 客户端发送输入到服务器
        if (!network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerInput) << myId
          << currentInput.up << currentInput.down << currentInput.left << currentInput.right;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::sendAttack() {
    if (attackSendClock.getElapsedTime().asSeconds() < 0.5f) return;
    attackSendClock.restart();

    if (isHost) {
        // 主机直接执行攻击（攻击最近的敌人）
        auto state = world.getState();
        if (state.players.size() > 1) {
            // 找到最近的玩家
            int targetId = -1;
            float minDist = std::numeric_limits<float>::max();
            auto myPos = playerPositions[myId];
            for (const auto& [id, pos] : playerPositions) {
                if (id == myId) continue;
                float dist = std::hypot(pos.x - myPos.x, pos.y - myPos.y);
                if (dist < minDist) {
                    minDist = dist;
                    targetId = id;
                }
            }
            if (targetId != -1) {
                world.attack(myId, targetId);
            }
        }
    } else {
        // 客户端发送攻击请求到服务器
        if (!network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerAttack) << myId;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::broadcastState() {
    auto state = world.getState();
    sf::Packet p;
    p << static_cast<uint8_t>(NetMsgType::GameState);
    p << static_cast<uint32_t>(state.players.size());
    for (auto& ps : state.players) {
        p << ps.position.x << ps.position.y << ps.health << ps.maxHealth;
    }
    network.broadcast(p);
}

void OnlineGameView::processNetwork() {
    sf::Packet packet;
    auto result = network.receive(packet);
    if (!result) return;
    auto [type, sender] = *result;
    auto [ip, port] = sender;

    if (isHost) {
        if (type == NetMsgType::Connect) {
            // 新玩家连接
            if (!network.hasClient(ip, port)) {
                int newId = static_cast<int>(network.getClientCount()) + 1;
                network.addClient(ip, port);
                world.addPlayer(newId, sf::Vector2f(400, 300));
                playerNames[newId] = "Player " + std::to_string(newId);
                playerPositions[newId] = {400, 300};

                // 发送连接确认
                sf::Packet accept;
                accept << static_cast<uint8_t>(NetMsgType::ConnectAccept) << newId;
                // 发送所有现有玩家的名字
                accept << static_cast<uint32_t>(playerNames.size());
                for (const auto& [id, name] : playerNames) {
                    accept << id << name;
                }
                network.send(accept, ip, port);

                statusText.setString("Player " + std::to_string(newId) + " connected! Total: " +
                                    std::to_string(network.getClientCount() + 1));
            }
        }
        else if (type == NetMsgType::Disconnect) {
            // 玩家断开连接
            int id;
            packet >> id;
            network.removeClient(ip, port);
            world.removePlayer(id);
            playerNames.erase(id);
            playerPositions.erase(id);
            statusText.setString("Player " + std::to_string(id) + " disconnected. Total: " +
                                std::to_string(network.getClientCount() + 1));
        }
        else if (type == NetMsgType::PlayerInput) {
            int id; bool u,d,l,r;
            packet >> id >> u >> d >> l >> r;
            world.setPlayerInput(id, {u,d,l,r});
        }
        else if (type == NetMsgType::PlayerAttack) {
            int attackerId;
            packet >> attackerId;
            // 找到最近的玩家进行攻击
            auto state = world.getState();
            if (state.players.size() > 1) {
                int targetId = -1;
                float minDist = std::numeric_limits<float>::max();
                auto attackerPos = playerPositions[attackerId];
                for (const auto& [id, pos] : playerPositions) {
                    if (id == attackerId) continue;
                    float dist = std::hypot(pos.x - attackerPos.x, pos.y - attackerPos.y);
                    if (dist < minDist) {
                        minDist = dist;
                        targetId = id;
                    }
                }
                if (targetId != -1) {
                    world.attack(attackerId, targetId);
                }
            }
        }
    } else {
        // 客户端处理
        if (type == NetMsgType::ConnectAccept) {
            packet >> myId;
            connected = true;
            statusText.setString("Connected! Your ID = " + std::to_string(myId));

            // 接收所有玩家名字
            uint32_t count;
            packet >> count;
            for (uint32_t i = 0; i < count; ++i) {
                int id;
                std::string name;
                packet >> id >> name;
                playerNames[id] = name;
            }
            playerNames[myId] = myUsername + " (You)";
            myNameText.setString(playerNames[myId]);
        }
        else if (type == NetMsgType::GameState) {
            uint32_t cnt; packet >> cnt;
            GameState state;
            for (uint32_t i=0; i<cnt; ++i) {
                sf::Vector2f pos; int hp, maxHp;
                packet >> pos.x >> pos.y >> hp >> maxHp;
                state.players.push_back({pos, hp, maxHp});
            }
            lastReceivedState = std::move(state);

            // 更新所有玩家位置
            int idx = 0;
            for (const auto& ps : lastReceivedState.players) {
                playerPositions[idx] = ps.position;
                ++idx;
            }

            // 检查自己是否死亡
            if (lastReceivedState.players.size() > static_cast<uint32_t>(myId)) {
                int myHp = lastReceivedState.players[myId].health;
                if (myHp <= 0 && !gameOver) {
                    gameOver = true; victory = false;
                    gameResultText.setString("You Lost!");
                }
            }
        }
        else if (type == NetMsgType::Disconnect) {
            int id;
            packet >> id;
            playerNames.erase(id);
            playerPositions.erase(id);
            statusText.setString("Player " + std::to_string(id) + " disconnected.");
        }
    }
}

void OnlineGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left && backBtn)
            backBtn->handleEvent(mPos, true);
    }
    if (!gameOver) {
        updateLocalInput();
        sendInputToServer();  // 主机和客户端都需要发送输入
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Space) {
                sendAttack();  // 统一使用 sendAttack
            }
        }
    }
}

void OnlineGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    if (backBtn) backBtn->handleEvent(mousePos, false);
    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    processNetwork();

    if (isHost) {
        world.update(dt);
        broadcastState();

        // 更新所有玩家位置
        auto state = world.getState();
        int idx = 0;
        for (const auto& ps : state.players) {
            playerPositions[idx] = ps.position;
            ++idx;
        }

        // 检查主机是否死亡
        if (state.players.size() > 0 && state.players[0].health <= 0 && !gameOver) {
            gameOver = true; victory = false;
            gameResultText.setString("You Lost!");
        }
    }

    // 更新其他玩家显示文本
    otherPlayerTexts.clear();
    int yOffset = 100;
    for (const auto& [id, name] : playerNames) {
        if (id == myId) continue;

        sf::Text text(globalFont, "", 18);
        text.setPosition({50, static_cast<float>(yOffset)});
        text.setFillColor(sf::Color::Black);

        std::string info = name;
        if (lastReceivedState.players.size() > static_cast<uint32_t>(id)) {
            info += " HP: " + std::to_string(lastReceivedState.players[id].health);
        }
        text.setString(info);
        otherPlayerTexts.push_back(text);
        yOffset += 30;
    }
}

void OnlineGameView::draw(sf::RenderWindow& window) {
    // 绘制所有玩家
    sf::CircleShape shape(20.f);

    // 绘制自己（绿色）
    if (playerPositions.count(myId)) {
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(playerPositions[myId] - sf::Vector2f(20,20));
        window.draw(shape);
    }

    // 绘制其他玩家（不同颜色）
    std::vector<sf::Color> colors = {sf::Color::Red, sf::Color::Blue, sf::Color::Magenta,
                                     sf::Color::Cyan, sf::Color::Yellow, sf::Color(255, 165, 0)};
    int colorIdx = 0;
    for (const auto& [id, pos] : playerPositions) {
        if (id == myId) continue;
        shape.setFillColor(colors[colorIdx % colors.size()]);
        shape.setPosition(pos - sf::Vector2f(20,20));
        window.draw(shape);
        ++colorIdx;
    }

    if (backBtn) backBtn->draw(window);
    window.draw(statusText);
    window.draw(myNameText);

    // 绘制其他玩家信息
    for (const auto& text : otherPlayerTexts) {
        window.draw(text);
    }

    if (gameOver) window.draw(gameResultText);
}