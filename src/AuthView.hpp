/**
 * @file AuthView.hpp
 * @brief 登录与注册视图
 *
 * 提供用户身份验证界面，包含登录（LoginView）和注册（SignUpView）两个视图。
 * 登录时从数据库读取盐值和哈希，用 SHA-256 验证密码；
 * 注册时生成随机盐值并存储哈希后的密码。
 * 验证通过后，用户名会写入 main.cpp 中的 currentUsername 引用。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

#include "ViewBase.hpp"   // 视图基类，提供语言切换按钮
#include "UIWidgets.hpp"  // Button、InputField 控件
#include "Database.hpp"   // 数据库操作接口

/**
 * @class LoginView
 * @brief 登录视图
 *
 * 用户输入用户名和密码，点击确认后与数据库中存储的
 * 盐值+哈希进行比对。验证成功则跳转到模式选择页面，
 * 失败则显示错误提示。
 */
class LoginView : public View {
public:
    /**
     * @brief 构造登录视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param username    当前用户名引用（验证成功时写入）
     * @param database    数据库引用
     */
    LoginView(const sf::Font& font, std::function<void(std::string)> changeView,
              std::string& username, Database& database);

    /// 处理 SFML 事件（鼠标点击、键盘输入等）
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    /// 每帧更新（按钮悬停检测）
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    /// 绘制登录界面
    void draw(sf::RenderWindow& window) override;

private:
    Database& db;                              ///< 数据库引用，用于查询用户凭证
    InputField usernameInput, passwordInput;   ///< 用户名和密码输入框
    Button confirmBtn, backBtn;                ///< 确认登录和返回按钮
    sf::Text title, statusText;                ///< 标题文字和状态提示文字
    std::string& usernameRef;                  ///< 当前用户名引用，登录成功时回写
};

/**
 * @class SignUpView
 * @brief 注册视图
 *
 * 用户输入用户名和密码，点击注册后：
 * 1. 检查用户名是否已存在
 * 2. 生成 16 字节随机盐值
 * 3. 用 SHA-256 + 盐值对密码做 10000 轮迭代哈希
 * 4. 将用户名、盐值、哈希存入数据库
 * 注册成功后自动跳转到登录页面。
 */
class SignUpView : public View {
public:
    /**
     * @brief 构造注册视图
     * @param font        全局字体
     * @param changeView  视图切换回调
     * @param database    数据库引用
     */
    SignUpView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database);

    /// 处理 SFML 事件
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    /// 每帧更新
    void update(sf::Vector2f mousePos, sf::Vector2u windowSize) override;
    /// 绘制注册界面
    void draw(sf::RenderWindow& window) override;

private:
    Database& db;                              ///< 数据库引用
    InputField usernameInput, passwordInput;   ///< 用户名和密码输入框
    Button regBtn, backBtn;                    ///< 注册和返回按钮
    sf::Text title, statusText;                ///< 标题和状态提示文字
};
