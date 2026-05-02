/**
 * @file GameView.hpp
 * @brief 单机闯关游戏视图
 *
 * 核心游戏视图，管理单机模式下的完整游戏循环：
 * - 从数据库加载角色存档（属性、职业、等级、分数、武器）
 * - 每帧处理玩家输入（WASD移动、攻击、翻滚、暂停、存档）
 * - 地图系统自动生成敌人（基于区域网格，越远越强）
 * - 战斗系统：扇形近战/远程投射物，刀光动画，伤害数字
 * - 掉落物系统：击杀敌人概率掉落血瓶和属性提升道具
 * - 阶段性胜利：分数达到阈值时暂停，应用职业强化，回满血
 * - 暂停/存档：Esc暂停，P键中途存档
 * - 游戏结束：保存角色数据和排行榜记录
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>

#include "ViewBase.hpp"
#include "UIWidgets.hpp"
#include "GameLogic.hpp"
#include "MapSystem.hpp"
#include "Database.hpp"

/**
 * @class ActualGameView
 * @brief 单机闯关游戏视图
 *
 * "ActualGameView" 这个名字来自早期开发，实际就是单机闯关模式的主视图。
 * 联机模式的视图是 OnlineView 中的 OnlineGameView。
 */
class ActualGameView : public View {
public:
    /**
     * @brief 构造游戏视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param username    当前用户名
     * @param database    数据库引用
     *
     * 构造时从数据库加载角色存档，初始化武器和战斗属性。
     */
    ActualGameView(const sf::Font& font, std::function<void(std::string)> changeView,
                   const std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /* ---- 战斗逻辑 ---- */
    /// 执行攻击（近战扇形或远程投射物）
    void attack();
    /// 更新顶部信息栏文字
    void updateInfoText();
    /// 重新开始游戏（重置所有状态）
    void restartGame();
    /// 处理敌人攻击玩家（近战碰撞和远程射击）
    void handleEnemyAttacks(float dt);
    /// 更新所有投射物位置和碰撞检测
    void updateProjectiles(float dt);
    /// 从阶段胜利继续游戏
    void continueFromVictory();
    /// 从当前游戏状态构建角色数据（用于存档）
    CharacterData buildCharacterData() const;

    /* ---- 掉落物 ---- */
    /// 敌人死亡时概率生成掉落物
    void spawnDrops(const Enemy& enemy);
    /// 更新掉落物存活时间和浮动动画
    void updateDrops(float dt);
    /// 检测玩家与掉落物的碰撞，执行拾取效果
    void checkPickups();

    /* ---- 基本状态 ---- */
    Database& db;                              ///< 数据库引用
    std::string username;                      ///< 当前用户名
    Player player;                             ///< 玩家对象
    std::vector<Enemy> enemies;                ///< 当前场景所有敌人
    std::vector<Projectile> projectiles;       ///< 所有投射物
    std::vector<DropItem> drops;               ///< 所有掉落物

    /// 拾取反馈特效（向上飘浮的文字，如"HP+33"、"STR+2"）
    struct PickupEffect {
        sf::Vector2f position;                 ///< 显示位置
        sf::Color color;                       ///< 文字颜色
        std::string text;                      ///< 显示文字
        float timer = 3.0f;                    ///< 剩余显示时间
    };
    std::vector<PickupEffect> pickupEffects;   ///< 拾取特效列表
    std::vector<DamageNumber> damageNumbers;   ///< 伤害数字列表
    MapSystem mapSystem;                       ///< 地图区域管理
    sf::View gameView;                         ///< 游戏摄像机视图
    AttributePanel attrPanel;                  ///< 属性面板（Tab切换）

    /* ---- UI 元素 ---- */
    sf::Text infoText;                         ///< 顶部信息栏
    sf::Text gameOverText;                     ///< "游戏结束"文字
    sf::Text pauseText;                        ///< "暂停"文字
    sf::Text victoryText;                      ///< "阶段胜利"文字
    Button backBtn;                            ///< 返回菜单按钮
    Button restartBtn;                         ///< 重新开始按钮
    Button resumeBtn;                          ///< 继续游戏按钮（暂停时）
    Button pauseBackBtn;                       ///< 暂停时返回菜单按钮
    Button victoryContinueBtn;                 ///< 阶段胜利继续按钮
    Button victoryBackBtn;                     ///< 阶段胜利返回按钮

    /* ---- 计时 ---- */
    sf::Clock deltaClock;                      ///< 帧间隔时钟
    sf::Clock gameClock;                       ///< 游戏总时长时钟
    float pausedTime = 0.f;                    ///< 累计暂停时间
    float lastPauseTime = 0.f;                 ///< 上次暂停开始时间
    std::function<void(std::string)> changeView; ///< 视图切换回调

    /* ---- 游戏状态 ---- */
    bool gameOver = false;                     ///< 游戏是否结束
    bool paused = false;                       ///< 是否暂停
    bool stageVictory = false;                 ///< 是否处于阶段胜利界面
    int currentVictoryStage = 0;               ///< 已达到的最高胜利阶段
    int killCount = 0;                         ///< 击杀数

    /// 阶段性胜利分值阈值：100/1000/2000/5000/10000
    static constexpr int STAGE_THRESHOLDS[] = {100, 1000, 2000, 5000, 10000};
    static constexpr int STAGE_COUNT = 5;      ///< 阶段总数
};
