/**
 * @file MenuView.hpp
 * @brief 主菜单与模式选择视图
 *
 * 包含两个视图：
 * - MainMenuView：主菜单，提供登录、注册、退出三个按钮
 * - LevelSelectView：模式选择，登录成功后进入，
 *   提供创建/选择角色、闯关模式、双人闯关、排行榜等选项
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <memory>

#include "ViewBase.hpp"
#include "UIWidgets.hpp"
#include "Database.hpp"

/**
 * @class MainMenuView
 * @brief 主菜单视图
 *
 * 三个按钮：登录、注册、退出。
 * 界面简洁，是程序启动后的第一个视图。
 */
class MainMenuView : public View {
public:
    /**
     * @brief 构造主菜单
     * @param font        全局字体
     * @param changeView  视图切换回调
     */
    MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::vector<Button> buttons;  ///< 菜单按钮列表（登录、注册、退出）
};

/**
 * @class LevelSelectView
 * @brief 模式选择视图
 *
 * 登录成功后首次进入时显示2秒欢迎动画，之后显示按钮：
 * - 创建角色 / 选择角色（取决于是否已有角色）
 * - 闯关模式（进入角色选择或创建）
 * - 双人闯关（进入联机大厅）
 * - 排行榜
 * - 返回
 */
class LevelSelectView : public View {
public:
    /**
     * @brief 构造模式选择视图
     * @param font          全局字体
     * @param changeView    视图切换回调
     * @param showWelcome   是否显示欢迎动画（首次进入时为true）
     * @param hasCharacter  当前用户是否已有角色
     */
    LevelSelectView(const sf::Font& font, std::function<void(std::string)> changeView,
                    bool showWelcome = true, bool hasCharacter = false);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /// 界面状态：欢迎动画中 或 按钮就绪
    enum class State { Welcome, ButtonsReady };
    State currentState = State::Welcome;  ///< 当前状态
    sf::Clock timer;                       ///< 欢迎动画计时器
    float delayTime = 2.0f;                ///< 欢迎动画持续时间（秒）
    sf::Text welcomeMsg;                   ///< 欢迎文字或"选择模式"标题
    std::vector<std::unique_ptr<Button>> buttons;  ///< 功能按钮列表
    std::function<void(std::string)> changeView;   ///< 视图切换回调
};
