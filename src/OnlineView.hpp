/**
 * @file OnlineView.hpp
 * @brief 联机大厅与联机游戏视图（双人合作闯关）
 *
 * 设计思路：
 * - 主机端运行完整游戏逻辑（敌人、投射物、地图生成、分数计算、掉落物）
 * - 客户端只发送输入，接收并渲染主机同步的游戏状态
 * - 双人共享同一无限地图，各自独立移动和攻击
 * - 主机通过扩展的 GameState 同步：玩家位置/血量、敌人、投射物、分数、掉落物
 * - 联机模式与闯关模式玩法一致：掉落物、阶段胜利、完整UI
 *
 * 包含两个视图：
 * - OnlineLobbyView：联机大厅，输入IP和端口，创建或加入房间
 * - OnlineGameView：联机游戏，主机运行逻辑+广播状态，客户端发送输入+渲染
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ViewBase.hpp"
#include "UIWidgets.hpp"
#include "GameLogic.hpp"
#include "MapSystem.hpp"
#include "Network.hpp"
#include "Database.hpp"
#include "AudioManager.hpp"

/**
 * @class OnlineLobbyView
 * @brief 联机大厅视图
 *
 * 提供IP和端口输入框，以及"创建房间"和"加入房间"按钮。
 * 创建房间时以服务器模式启动，加入房间时以客户端模式连接。
 */
class OnlineLobbyView : public View {
public:
    OnlineLobbyView(const sf::Font& font, std::function<void(std::string)> changeView, NetworkManager& network);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    InputField ipInput, portInput;   ///< IP和端口输入框
    Button createBtn, joinBtn, backBtn;  ///< 创建房间、加入房间、返回按钮
    sf::Text statusText;             ///< 状态提示文字
    std::function<void(std::string)> changeView; ///< 视图切换回调
    NetworkManager& network;         ///< 网络管理器引用
};

/**
 * @struct SyncEnemy
 * @brief 同步的敌人数据（网络传输用）
 *
 * 主机将敌人数据序列化为此结构发送给客户端，
 * 客户端据此渲染敌人菱形和血条。
 */
struct SyncEnemy {
    int type;               ///< EnemyType 枚举值
    float posX, posY;       ///< 位置
    int health, maxHealth;  ///< 血量
    float radius;           ///< 碰撞半径
    int score;              ///< 击杀分数
};

/**
 * @struct SyncProjectile
 * @brief 同步的投射物数据（网络传输用）
 */
struct SyncProjectile {
    float posX, posY;       ///< 位置
    float dirX, dirY;       ///< 方向
    float speed;            ///< 速度
    int damage;             ///< 伤害
    float maxDistance, traveled;  ///< 最大距离和已飞行距离
    bool fromPlayer;        ///< 是否玩家发射
    uint8_t colorR, colorG, colorB;  ///< 投射物颜色RGB
};

/**
 * @struct SyncPlayer
 * @brief 同步的玩家数据（网络传输用）
 */
struct SyncPlayer {
    float posX, posY;       ///< 位置
    int health, maxHealth;  ///< 血量
    int totalScore;         ///< 累计分数
    int level;              ///< 等级
    std::string name;       ///< 玩家名字
    float radius;           ///< 碰撞半径
    bool isRolling;         ///< 翻滚状态
    bool isInvincible;      ///< 无敌状态
};

/**
 * @struct SyncDrop
 * @brief 同步的掉落物数据（网络传输用）
 */
struct SyncDrop {
    int type;               ///< DropType 枚举值
    float posX, posY;       ///< 位置
    float lifetime;         ///< 剩余存活时间
    float bobTimer;         ///< 浮动动画计时器
};

/**
 * @class OnlineGameView
 * @brief 联机游戏视图
 *
 * 主机端和客户端共用此类，通过 isHost 区分行为：
 *
 * 主机端（isHost=true）：
 * - 运行完整游戏逻辑（敌人AI、投射物碰撞、掉落物拾取）
 * - 接收客户端输入和攻击信号
 * - 每帧广播完整游戏状态（玩家/敌人/投射物/掉落物）
 * - 检查阶段性胜利，为所有玩家应用强化
 *
 * 客户端（isHost=false）：
 * - 发送输入状态（~30Hz）和攻击/翻滚信号
 * - 接收并解析主机广播的游戏状态
 * - 根据同步数据渲染游戏画面
 */
class OnlineGameView : public View {
public:
    /**
     * @brief 构造联机游戏视图
     * @param font      全局字体
     * @param changeView 视图切换回调
     * @param network   网络管理器引用
     * @param isHost    是否为主机
     * @param username  当前用户名
     * @param database  数据库引用
     */
    OnlineGameView(const sf::Font& font, std::function<void(std::string)> changeView,
                   NetworkManager& network, bool isHost, const std::string& username,
                   Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /* ---- 网络通信 ---- */
    /// 发送输入状态给服务器（客户端）或直接应用（主机）
    void sendInputToServer();
    /// 发送攻击信号
    void sendAttackSignal();
    /// 广播完整游戏状态给所有客户端（主机）
    void broadcastFullState();
    /// 处理接收到的网络消息
    void processNetwork();

    /* ---- 主机端游戏逻辑 ---- */
    /// 主机端每帧更新：玩家移动、敌人生成/AI、碰撞检测
    void hostUpdateGame(float dt);
    /// 主机端处理敌人攻击玩家
    void hostHandleEnemyAttacks(float dt);
    /// 主机端更新投射物和碰撞
    void hostUpdateProjectiles(float dt);
    /// 主机端处理击杀（分配分数、生成掉落物）
    void hostProcessKill();

    /* ---- 掉落物系统（主机端） ---- */
    void spawnDrops(const Enemy& enemy);
    void updateDrops(float dt);
    void checkPickups();

    /* ---- 攻击逻辑 ---- */
    /// 执行指定玩家的攻击（近战或远程）
    void performAttack(int playerId);

    /* ---- 渲染 ---- */
    /// 绘制游戏世界（地图、敌人、投射物、掉落物）
    void drawGameWorld(sf::RenderWindow& window);
    /// 绘制所有玩家（圆形+名字+血条+刀光）
    void drawPlayers(sf::RenderWindow& window);
    /// 绘制UI层（信息栏、按钮、阶段胜利界面）
    void drawUI(sf::RenderWindow& window);

    /* ---- 基本状态 ---- */
    std::string myUsername;                     ///< 当前用户名
    std::function<void(std::string)> changeView; ///< 视图切换回调
    NetworkManager& network;                    ///< 网络管理器引用
    Database& db;                               ///< 数据库引用
    bool isHost;                                ///< 是否为主机
    int myId = -1;                              ///< 自己的玩家ID
    bool connected = false;                     ///< 是否已连接

    /* ---- 玩家数据 ---- */
    /// 主机端：完整玩家对象（含属性、武器、战斗状态）
    std::map<int, Player> coopPlayers;
    /// 客户端：从主机同步的简化玩家数据
    std::map<int, SyncPlayer> syncPlayers;
    /// 玩家ID到名字的映射
    std::map<int, std::string> playerNames;

    /* ---- 敌人与投射物 ---- */
    std::vector<Enemy> enemies;                 ///< 主机端：完整敌人对象
    std::vector<Projectile> projectiles;        ///< 主机端：完整投射物
    std::vector<SyncEnemy> syncEnemies;         ///< 客户端：同步的敌人
    std::vector<SyncProjectile> syncProjectiles; ///< 客户端：同步的投射物

    /* ---- 掉落物系统 ---- */
    std::vector<DropItem> drops;                ///< 主机端：掉落物
    std::vector<SyncDrop> syncDrops;            ///< 客户端：同步的掉落物

    /// 拾取反馈特效
    struct PickupEffect {
        sf::Vector2f position;
        sf::Color color;
        std::string text;
        float timer = 3.0f;
    };
    std::vector<PickupEffect> pickupEffects;
    std::vector<DamageNumber> damageNumbers;    ///< 伤害数字

    /* ---- 地图与摄像机 ---- */
    MapSystem mapSystem;                        ///< 地图区域管理
    sf::View gameView;                          ///< 游戏摄像机
    AttributePanel attrPanel;                   ///< 属性面板

    /* ---- UI元素 ---- */
    std::unique_ptr<Button> backBtn;            ///< 返回菜单按钮
    std::unique_ptr<Button> startBtn;           ///< 房主开始游戏按钮
    std::unique_ptr<Button> victoryContinueBtn; ///< 阶段胜利继续按钮
    std::unique_ptr<Button> victoryBackBtn;     ///< 阶段胜利返回按钮
    sf::Text statusText;                        ///< 状态提示
    sf::Text infoText;                          ///< 信息栏
    sf::Text gameResultText;                    ///< 游戏结果文字
    sf::Text victoryText;                       ///< 阶段胜利文字
    sf::Clock deltaClock;                       ///< 帧间隔时钟
    sf::Clock inputSendClock;                   ///< 输入发送频率控制
    sf::Clock attackSendClock;                  ///< 攻击信号频率控制
    sf::Clock gameClock;                        ///< 游戏总时长

    /* ---- 输入与游戏状态 ---- */
    PlayerInput currentInput;                   ///< 当前帧的输入状态
    bool attackRequested = false;               ///< 是否请求攻击
    bool gameOver = false;                      ///< 游戏是否结束
    bool paused = false;                        ///< 是否暂停
    bool gameStarted = false;                   ///< 房主点击开始后才刷怪
    bool stageVictory = false;                  ///< 是否处于阶段胜利
    int currentVictoryStage = 0;                ///< 已达到的最高胜利阶段
    float pausedTime = 0.f;                     ///< 累计暂停时间
    float lastPauseTime = 0.f;                  ///< 上次暂停开始时间
    int nextPlayerId = 1;                       ///< 递增分配玩家ID
    int killCount = 0;                          ///< 击杀数

    /// 阶段性胜利分值阈值
    static constexpr int STAGE_THRESHOLDS[] = {100, 1000, 2000, 5000, 10000};
    static constexpr int STAGE_COUNT = 5;
};
