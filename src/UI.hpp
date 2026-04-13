/**
 * @file UI.hpp
 * @brief 用户界面模块头文件
 *
 * 包含所有视图类、控件类的声明。实现位于 UI.cpp 中。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

// 包含游戏逻辑和网络模块（因为在类中使用了值类型成员）
#include "GameLogic.hpp"
#include "Network.hpp"

// 前向声明外部依赖（不拥有完整定义的类）
class Database;
class NetworkManager;

// ======================== UI 控件类 ========================

/**
 * @class Button
 * @brief 简单的按钮控件，支持点击回调和悬停效果
 */
class Button {
public:
    using Callback = std::function<void()>;

    Button(const sf::Font& font, const std::string& label, sf::Vector2f pos, sf::Vector2f size, Callback onClick);

    // 允许移动，禁止拷贝
    Button(Button&&) noexcept = default;
    Button& operator=(Button&&) noexcept = default;
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void handleEvent(sf::Vector2f mousePos, bool isClicked);
    void draw(sf::RenderWindow& window) const;

private:
    sf::RectangleShape shape;
    sf::Text text;
    Callback callback;
};

/**
 * @class InputField
 * @brief 文本输入框控件，支持密码模式、光标闪烁、长度限制
 */
class InputField {
public:
    InputField(const sf::Font& font, sf::Vector2f pos, sf::Vector2f size,
               bool passwordMode = false, size_t maxLen = 16, bool allowSpace = true);

    void setString(const std::string& str);
    void updateCursor();
    void handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    void clear();
    void draw(sf::RenderWindow& window) const;
    std::string getString() const;

private:
    void updateDisplayText();

    sf::Clock cursorClock;
    bool showCursor = false;
    sf::RectangleShape box;
    sf::Text text;
    std::string content;
    bool isFocused = false;
    bool isPassword = false;
    size_t maxLength;
    bool allowSpace = true;
};

// ======================== 视图基类 ========================

/**
 * @class View
 * @brief 所有界面视图的抽象基类
 */
class View {
protected:
    const sf::Font& globalFont;
public:
    View(const sf::Font& font);
    virtual ~View() = default;
    virtual void handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;
    virtual void update(sf::Vector2f mousePos, sf::Vector2u windowSize) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
};

// ======================== 具体视图类声明 ========================

/**
 * @class LoginView
 * @brief 登录界面视图
 */
class LoginView : public View {
public:
    LoginView(const sf::Font& font, std::function<void(std::string)> changeView,
              std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    Database& db;
    InputField usernameInput, passwordInput;
    Button confirmBtn, backBtn;
    sf::Text title, statusText;
    std::string& usernameRef;
};

/**
 * @class SignUpView
 * @brief 注册界面视图
 */
class SignUpView : public View {
public:
    SignUpView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    Database& db;
    InputField usernameInput, passwordInput;
    Button regBtn, backBtn;
    sf::Text title, statusText;
};

/**
 * @class MainMenuView
 * @brief 主菜单视图
 */
class MainMenuView : public View {
public:
    MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::vector<Button> buttons;
};

/**
 * @class LevelSelectView
 * @brief 关卡/模式选择视图
 */
class LevelSelectView : public View {
public:
    LevelSelectView(const sf::Font& font, std::function<void(std::string)> changeView, bool showWelcome = true);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class State { Welcome, ButtonsReady };
    State currentState = State::Welcome;
    sf::Clock timer;
    float delayTime = 2.0f;
    sf::Text welcomeMsg;
    std::vector<std::unique_ptr<Button>> buttons;
    std::function<void(std::string)> changeView;
};

/**
 * @class ActualGameView
 * @brief 单机游戏视图
 */
class ActualGameView : public View {
public:
    ActualGameView(const sf::Font& font, std::function<void(std::string)> changeView,
                   const std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void spawnEnemies();
    void attack();
    void updateInfoText(sf::Vector2u windowSize);
    void restartGame();

    Database& db;
    std::string username;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<sf::Vector2f> attackEffects;
    float effectTimer = 0.f;
    sf::Text infoText;
    sf::Text gameOverText;
    Button backBtn;
    Button restartBtn;
    sf::Clock deltaClock;
    sf::Clock attackClock;
    std::function<void(std::string)> changeView;
    bool gameOver = false;
    int killCount = 0;
};

/**
 * @class RankingsView
 * @brief 排行榜显示视图
 */
class RankingsView : public View {
public:
    RankingsView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void refreshRankings();

    Database& db;
    sf::Text title;
    std::vector<sf::Text> rankTexts;
    Button backBtn;
    std::function<void(std::string)> changeView;
};

/**
 * @class OnlineLobbyView
 * @brief 在线大厅视图（创建/加入房间）
 */
class OnlineLobbyView : public View {
public:
    OnlineLobbyView(const sf::Font& font, std::function<void(std::string)> changeView, NetworkManager& network);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    InputField ipInput, portInput;
    Button createBtn, joinBtn, backBtn;
    sf::Text statusText;
    std::function<void(std::string)> changeView;
    NetworkManager& network;
};

/**
 * @class OnlineGameView
 * @brief 在线游戏视图（双人对战）
 */
class OnlineGameView : public View {
public:
    OnlineGameView(const sf::Font& font, std::function<void(std::string)> changeView,
                   NetworkManager& network, bool isHost, const std::string& username);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void updateLocalInput();
    void sendInputToServer();
    void sendAttack();
    void broadcastState();
    void processNetwork();

    std::string myUsername;
    std::map<int, std::string> playerNames;  // 玩家ID到名字的映射
    std::function<void(std::string)> changeView;
    NetworkManager& network;
    bool isHost;
    int myId;
    GameWorld world;
    GameState lastReceivedState;
    std::unique_ptr<Button> backBtn;
    sf::Text statusText;
    sf::Text myNameText;
    std::vector<sf::Text> otherPlayerTexts;  // 其他玩家的名字和血量显示
    sf::Text gameResultText;
    sf::Clock deltaClock;
    sf::Clock inputSendClock;
    sf::Clock attackSendClock;
    sf::Clock disconnectCheckClock;  // 检测断开连接的时钟
    std::map<int, sf::Vector2f> playerPositions;  // 所有玩家的位置
    PlayerInput currentInput;
    bool gameOver = false;
    bool victory = false;
    bool connected = false;  // 是否已连接
};