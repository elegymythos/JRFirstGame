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

// 包含游戏逻辑和网络模块
#include "GameLogic.hpp"
#include "Network.hpp"
#include "MapSystem.hpp"
#include "Database.hpp"

// 前向声明外部依赖
class NetworkManager;

// ======================== UI 控件类 ========================

class Button {
public:
    using Callback = std::function<void()>;

    Button(const sf::Font& font, const std::string& label, sf::Vector2f pos, sf::Vector2f size, Callback onClick);

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
    
    // Count UTF-8 characters (not bytes)
    static size_t utf8Length(const std::string& str) {
        size_t count = 0;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if ((c & 0x80) == 0) {
                // 1-byte character
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                // 2-byte character
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                // 3-byte character
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                // 4-byte character
                i += 4;
            } else {
                // Invalid UTF-8, skip one byte
                i += 1;
            }
            ++count;
        }
        return count;
    }

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

// ======================== 属性面板（叠加组件） ========================

class AttributePanel {
public:
    AttributePanel(const sf::Font& font);

    void toggle();
    bool isVisible() const;

    void update(const Player& player);
    void draw(sf::RenderWindow& window) const;

private:
    const sf::Font& font_;
    bool visible_ = false;
    sf::RectangleShape background_;
    std::vector<sf::Text> lines_;
};

// ======================== 视图基类 ========================

class View {
protected:
    const sf::Font& globalFont;
    bool languageChanged = false;  // 语言切换标志
public:
    View(const sf::Font& font);
    virtual ~View() = default;
    virtual void handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;
    virtual void update(sf::Vector2f mousePos, sf::Vector2u windowSize) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    /**
     * @brief 检查是否需要刷新（语言切换后）
     */
    bool needsRefresh() const { return languageChanged; }
    void clearRefreshFlag() { languageChanged = false; }

    /**
     * @brief 绘制语言切换按钮（右上角）
     */
    void drawLangButton(sf::RenderWindow& window, sf::Vector2f pos = {1180, 10});

    /**
     * @brief 处理语言按钮点击
     */
    bool handleLangButton(sf::Vector2f mousePos, bool isClicked, sf::Vector2f pos = {1180, 10});

private:
    sf::RectangleShape langBg_;
    sf::Text langText_;
    bool langButtonHovered_ = false;  // Hover state for language button
};

// ======================== 具体视图类声明 ========================

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

class MainMenuView : public View {
public:
    MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::vector<Button> buttons;
};

class LevelSelectView : public View {
public:
    LevelSelectView(const sf::Font& font, std::function<void(std::string)> changeView,
                    bool showWelcome = true, bool hasCharacter = false);

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

// ======================== 角色创建视图 ========================

class CharacterCreateView : public View {
public:
    CharacterCreateView(const sf::Font& font, std::function<void(std::string)> changeView,
                        const std::string& username, Database& database, int slot = -1);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void rerollAttributes();
    void selectClass(CharacterClass cls);
    void confirmCreate();

    Database& db;
    std::string username_;
    int slot_;  // -1 = auto assign
    std::function<void(std::string)> changeView;

    Attributes currentAttributes_;
    CharacterClass selectedClass_ = CharacterClass::Warrior;
    ClassConfig classConfig_;
    bool classSelected_ = false;

    sf::Text title_;
    sf::Text attrText_;
    sf::Text classInfoText_;
    sf::Text statusText_;
    Button warriorBtn_, mageBtn_, assassinBtn_;
    Button rerollBtn_, confirmBtn_, backBtn_;
};

// ======================== 角色选择视图（多角色） ========================

class CharacterSelectView : public View {
public:
    CharacterSelectView(const sf::Font& font, std::function<void(std::string)> changeView,
                        const std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void refreshSlots();

    Database& db;
    std::string username_;
    std::function<void(std::string)> changeView;

    std::vector<CharacterData> characters_;
    sf::Text title_;
    std::vector<sf::Text> slotTexts_;
    std::vector<std::unique_ptr<Button>> selectBtns_;
    std::vector<std::unique_ptr<Button>> deleteBtns_;
    std::vector<std::unique_ptr<Button>> createBtns_;
    Button backBtn_;
    bool needsRefresh_ = false;  // Flag to defer refresh after event handling
};

// ======================== 单机游戏视图（重构） ========================

class ActualGameView : public View {
public:
    ActualGameView(const sf::Font& font, std::function<void(std::string)> changeView,
                   const std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    void attack();
    void updateInfoText();
    void restartGame();
    void handleEnemyAttacks(float dt);
    void updateProjectiles(float dt);
    void continueFromVictory();  // Handle victory continue
    CharacterData buildCharacterData() const;  // Build save data

    Database& db;
    std::string username;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<sf::Vector2f> attackEffects;
    float effectTimer = 0.f;
    MapSystem mapSystem;
    sf::View gameView;
    AttributePanel attrPanel;

    sf::Text infoText;
    sf::Text gameOverText;
    sf::Text pauseText;  // Pause screen text
    sf::Text victoryText;  // Stage victory text
    Button backBtn;
    Button restartBtn;
    Button resumeBtn;  // Resume button for pause menu
    Button pauseBackBtn;  // Back to menu button for pause menu
    Button victoryContinueBtn;  // Continue button for victory screen
    Button victoryBackBtn;  // Back to menu button for victory screen
    sf::Clock deltaClock;
    sf::Clock gameClock;  // Total game time for difficulty scaling
    float pausedTime = 0.f;  // Accumulated time while paused
    float lastPauseTime = 0.f;  // Time when pause started
    std::function<void(std::string)> changeView;
    bool gameOver = false;
    bool paused = false;  // Pause state
    bool stageVictory = false;  // Stage victory state
    bool stageVictoryShown = false;  // Prevent re-triggering victory at same score
    int killCount = 0;
};

// ======================== 排行榜视图 ========================

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

// ======================== 联机大厅视图 ========================

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

// ======================== 联机游戏视图（双人闯关） ========================

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
    void attackEnemies();
    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void handleEnemyAttacks(float dt);

    std::string myUsername;
    std::map<int, std::string> playerNames;
    std::function<void(std::string)> changeView;
    NetworkManager& network;
    bool isHost;
    int myId;
    GameWorld world;
    GameState lastReceivedState;

    // 双人闯关：敌人与投射物
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    MapSystem mapSystem;
    sf::View gameView;
    AttributePanel attrPanel;

    // 双人玩家数据
    std::map<int, Player> coopPlayers;

    std::unique_ptr<Button> backBtn;
    sf::Text statusText;
    sf::Text myNameText;
    std::vector<sf::Text> otherPlayerTexts;
    sf::Text gameResultText;
    sf::Clock deltaClock;
    sf::Clock inputSendClock;
    sf::Clock attackSendClock;
    sf::Clock disconnectCheckClock;
    std::map<int, sf::Vector2f> playerPositions;
    PlayerInput currentInput;
    bool gameOver = false;
    bool victory = false;
    bool connected = false;
};
