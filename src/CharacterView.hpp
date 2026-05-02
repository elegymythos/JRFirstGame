/**
 * @file CharacterView.hpp
 * @brief 角色创建与选择视图
 *
 * 提供两个视图：
 * - CharacterCreateView：创建新角色，选择职业（战士/法师/刺客），
 *   随机生成六大属性，可重新随机，确认后存入数据库。
 * - CharacterSelectView：展示已有角色列表（最多3个槽位），
 *   支持选择角色进入游戏、删除角色、在空槽位新建角色。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "ViewBase.hpp"   // 视图基类
#include "UIWidgets.hpp"  // Button 控件
#include "Database.hpp"   // 数据库操作接口

/**
 * @class CharacterCreateView
 * @brief 角色创建视图
 *
 * 流程：
 * 1. 自动随机生成六大属性（正态分布，范围[1,20]，总和上限80）
 * 2. 用户可点击"重新随机"刷新属性
 * 3. 选择职业：战士（力量+5/铁剑）、法师（魔法+5/火焰法器）、刺客（敏捷+5/暗影刺刀）
 * 4. 确认创建，自动分配空槽位，存入数据库
 */
class CharacterCreateView : public View {
public:
    /**
     * @brief 构造角色创建视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param username    当前登录用户名
     * @param database    数据库引用
     * @param slot        指定槽位（-1表示自动分配）
     */
    CharacterCreateView(const sf::Font& font, std::function<void(std::string)> changeView,
                        const std::string& username, Database& database, int slot = -1);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /// 重新随机生成六大属性
    void rerollAttributes();
    /// 选择职业并显示职业加成信息
    void selectClass(CharacterClass cls);
    /// 确认创建角色，存入数据库
    void confirmCreate();

    Database& db;                              ///< 数据库引用
    std::string username_;                     ///< 当前用户名
    int slot_;                                 ///< 角色槽位（0-2，-1为自动分配）
    std::function<void(std::string)> changeView; ///< 视图切换回调

    Attributes currentAttributes_;             ///< 当前随机生成的属性
    CharacterClass selectedClass_ = CharacterClass::Warrior; ///< 已选职业
    ClassConfig classConfig_;                  ///< 职业配置（加成、允许武器等）
    bool classSelected_ = false;               ///< 是否已选择职业

    sf::Text title_;                           ///< 标题"创建角色"
    sf::Text attrText_;                        ///< 属性数值显示
    sf::Text classInfoText_;                   ///< 职业加成信息
    sf::Text statusText_;                      ///< 状态提示（错误/成功）
    Button warriorBtn_, mageBtn_, assassinBtn_; ///< 三个职业选择按钮
    Button rerollBtn_, confirmBtn_, backBtn_;  ///< 重新随机、确认、返回按钮
};

/**
 * @class CharacterSelectView
 * @brief 角色选择视图
 *
 * 展示当前用户的3个角色槽位：
 * - 已占用槽位：显示职业、等级、分数，提供"选择"和"删除"按钮
 * - 空槽位：显示"空槽位"，提供"新建角色"按钮
 * 选择角色后，将槽位号存入 user_settings 表，然后进入游戏。
 */
class CharacterSelectView : public View {
public:
    /**
     * @brief 构造角色选择视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param username    当前用户名
     * @param database    数据库引用
     */
    CharacterSelectView(const sf::Font& font, std::function<void(std::string)> changeView,
                        const std::string& username, Database& database);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    void draw(sf::RenderWindow& window) override;

private:
    /// 从数据库重新加载角色列表并刷新界面
    void refreshSlots();

    Database& db;                              ///< 数据库引用
    std::string username_;                     ///< 当前用户名
    std::function<void(std::string)> changeView; ///< 视图切换回调

    std::vector<CharacterData> characters_;    ///< 当前用户的所有角色数据
    sf::Text title_;                           ///< 标题"选择角色"
    std::vector<sf::Text> slotTexts_;          ///< 每个槽位的描述文字
    std::vector<std::unique_ptr<Button>> selectBtns_;  ///< "选择"按钮列表
    std::vector<std::unique_ptr<Button>> deleteBtns_;  ///< "删除"按钮列表
    std::vector<std::unique_ptr<Button>> createBtns_;  ///< "新建"按钮列表
    Button backBtn_;                           ///< 返回按钮
    bool needsRefresh_ = false;                ///< 删除角色后需要刷新标记
};
