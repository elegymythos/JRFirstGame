/**
 * @file ViewBase.hpp
 * @brief 视图基类，提供语言切换按钮等通用功能
 *
 * 所有视图（菜单、登录、游戏、排行榜等）的公共基类。
 * 提供：
 * - globalFont 引用：所有子类共享的全局字体
 * - 语言切换按钮：右上角显示当前语言，点击切换中/英文
 * - languageChanged 标记：语言切换后通知主循环重建当前视图
 * - 三个纯虚函数：handleEvent、update、draw，子类必须实现
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "I18n.hpp"

/**
 * @class View
 * @brief 视图基类
 *
 * 主循环中通过 std::unique_ptr<View> 管理当前视图，
 * 视图切换通过 changeView 回调（设置 pendingView 字符串）实现。
 */
class View {
protected:
    const sf::Font& globalFont;     ///< 全局字体引用（子类可直接使用）
    bool languageChanged = false;   ///< 语言是否刚切换（主循环据此重建视图）

public:
    /// 构造视图，保存全局字体引用
    View(const sf::Font& font);
    virtual ~View() = default;

    /// 处理 SFML 事件（键盘、鼠标等）
    virtual void handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;
    /// 每帧更新逻辑
    virtual void update(sf::Vector2f mousePos, sf::Vector2u windowSize) = 0;
    /// 绘制画面
    virtual void draw(sf::RenderWindow& window) = 0;

    /// 语言切换后是否需要重建视图
    bool needsRefresh() const { return languageChanged; }
    /// 清除刷新标记
    void clearRefreshFlag() { languageChanged = false; }

    /**
     * @brief 绘制语言切换按钮
     * @param window 渲染窗口
     * @param pos    按钮位置，默认右上角 (1180, 10)
     *
     * 显示当前语言名称（"EN"或"中文"），悬停时高亮。
     */
    void drawLangButton(sf::RenderWindow& window, sf::Vector2f pos = {1180, 10});

    /**
     * @brief 处理语言切换按钮的点击
     * @param mousePos  鼠标位置
     * @param isClicked 是否为点击事件
     * @param pos       按钮位置
     * @return true=点击了语言按钮（已切换语言）
     */
    bool handleLangButton(sf::Vector2f mousePos, bool isClicked, sf::Vector2f pos = {1180, 10});

private:
    sf::RectangleShape langBg_;          ///< 语言按钮背景矩形
    sf::Text langText_;                  ///< 语言按钮文字
    bool langButtonHovered_ = false;     ///< 语言按钮是否被悬停
};
