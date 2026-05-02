/**
 * @file RankingsView.hpp
 * @brief 排行榜视图
 *
 * 从数据库加载前10名排行记录（按等级降序+分数降序），
 * 以列表形式展示。无数据时显示"暂无排行数据"。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>

#include "ViewBase.hpp"
#include "UIWidgets.hpp"
#include "Database.hpp"

/**
 * @class RankingsView
 * @brief 排行榜视图
 *
 * 构造时自动从数据库加载排行数据。
 * 每条记录显示：排名、用户名、等级、分数。
 */
class RankingsView : public View {
public:
    /**
     * @brief 构造排行榜视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param database    数据库引用
     */
    RankingsView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /// 从数据库重新加载排行数据并刷新显示
    void refreshRankings();

    Database& db;                              ///< 数据库引用
    sf::Text title;                            ///< 标题"排行榜 (前10)"
    std::vector<sf::Text> rankTexts;           ///< 每条排行记录的文字
    Button backBtn;                            ///< 返回按钮
    std::function<void(std::string)> changeView; ///< 视图切换回调
};
