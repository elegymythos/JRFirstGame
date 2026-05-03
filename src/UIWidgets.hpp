/**
 * @file UIWidgets.hpp
 * @brief UI基础控件：Button、InputField、AttributePanel
 *
 * 提供三个可复用的 UI 控件：
 * - Button：矩形按钮，支持悬停变色和点击回调
 * - InputField：文本输入框，支持密码模式、UTF-8输入、最大长度限制
 * - AttributePanel：属性面板，Tab键切换显示，展示角色完整属性信息
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>

#include "GameLogic.hpp"  // Player, Attributes
#include "I18n.hpp"       // 国际化翻译
#include "AudioManager.hpp"

// ======================== Button ========================

/**
 * @class Button
 * @brief 矩形按钮控件
 *
 * 白色背景+黑色边框，鼠标悬停时变为浅蓝色。
 * 点击时调用回调函数。禁止拷贝（含不可拷贝的回调），
 * 允许移动（用于存入 unique_ptr 的场景）。
 */
class Button {
public:
    using Callback = std::function<void()>;  ///< 点击回调类型

    /**
     * @brief 构造按钮
     * @param font     全局字体
     * @param label    按钮文字
     * @param pos      左上角位置
     * @param size     按钮尺寸
     * @param onClick  点击回调
     */
    Button(const sf::Font& font, const std::string& label, sf::Vector2f pos, sf::Vector2f size, Callback onClick);

    // 允许移动语义（用于 unique_ptr 容器）
    Button(Button&&) noexcept = default;
    Button& operator=(Button&&) noexcept = default;
    // 禁止拷贝（std::function 不可拷贝）
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    /**
     * @brief 处理鼠标事件
     * @param mousePos  鼠标位置
     * @param isClicked 是否为点击事件（true=点击，false=仅悬停检测）
     */
    void handleEvent(sf::Vector2f mousePos, bool isClicked);
    /// 绘制按钮（矩形+文字）
    void draw(sf::RenderWindow& window) const;

private:
    sf::RectangleShape shape;  ///< 按钮矩形
    sf::Text text;             ///< 按钮文字
    Callback callback;         ///< 点击回调
};

// ======================== InputField ========================

/**
 * @class InputField
 * @brief 文本输入框控件
 *
 * 功能：
 * - 点击获取焦点（蓝色边框），点击其他区域失去焦点
 * - 支持键盘输入（含中文UTF-8编码）
 * - 密码模式：显示星号代替实际字符
 * - 最大字符长度限制（按UTF-8字符数计算，非字节数）
 * - 闪烁光标（焦点时每0.5秒切换）
 * - 退格键删除（正确处理UTF-8多字节字符）
 */
class InputField {
public:
    /**
     * @brief 构造输入框
     * @param font         全局字体
     * @param pos          左上角位置
     * @param size         输入框尺寸
     * @param passwordMode 是否为密码模式（显示星号）
     * @param maxLen       最大字符数（UTF-8字符数）
     * @param allowSpace   是否允许输入空格
     */
    InputField(const sf::Font& font, sf::Vector2f pos, sf::Vector2f size,
               bool passwordMode = false, size_t maxLen = 16, bool allowSpace = true);

    /// 设置输入内容（截断到 maxLength）
    void setString(const std::string& str);
    /// 更新光标闪烁状态
    void updateCursor();
    /// 处理 SFML 事件（鼠标点击获取焦点、键盘输入字符）
    void handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    /// 清空输入内容
    void clear();
    /// 绘制输入框（矩形+文字+光标）
    void draw(sf::RenderWindow& window) const;
    /// 获取输入内容
    std::string getString() const;

private:
    /// 更新显示文字（密码模式替换为星号，焦点时追加光标）
    void updateDisplayText();

    /**
     * @brief 计算UTF-8字符串的字符数（非字节数）
     * @param str UTF-8字符串
     * @return 字符数
     *
     * 根据首字节的高位模式判断字符占用的字节数：
     * 0xxxxxxx: 1字节, 110xxxxx: 2字节, 1110xxxx: 3字节, 11110xxx: 4字节
     */
    static size_t utf8Length(const std::string& str) {
        size_t count = 0;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if ((c & 0x80) == 0) i += 1;       // ASCII: 1字节
            else if ((c & 0xE0) == 0xC0) i += 2; // 2字节UTF-8
            else if ((c & 0xF0) == 0xE0) i += 3; // 3字节UTF-8（中文）
            else if ((c & 0xF8) == 0xF0) i += 4; // 4字节UTF-8（emoji）
            else i += 1;                         // 异常字节，跳过
            ++count;
        }
        return count;
    }

    sf::Clock cursorClock;           ///< 光标闪烁时钟
    bool showCursor = false;         ///< 当前是否显示光标
    sf::RectangleShape box;          ///< 输入框矩形
    sf::Text text;                   ///< 显示文字
    std::string content;             ///< 实际输入内容
    bool isFocused = false;          ///< 是否获得焦点
    bool isPassword = false;         ///< 是否密码模式
    size_t maxLength;                ///< 最大字符数
    bool allowSpace = true;          ///< 是否允许空格
};

// ======================== AttributePanel ========================

/**
 * @class AttributePanel
 * @brief 属性面板控件
 *
 * 半透明黑色背景，显示角色完整属性信息：
 * - 六大属性（基础+加成=最终）
 * - 职业、等级、经验、分数
 * - 装备武器名称
 * Tab键切换显示/隐藏。
 */
class AttributePanel {
public:
    /**
     * @brief 构造属性面板
     * @param font 全局字体
     */
    AttributePanel(const sf::Font& font);

    /// 切换显示/隐藏
    void toggle();
    /// 是否可见
    bool isVisible() const;

    /**
     * @brief 更新属性面板内容
     * @param player 玩家对象（从中提取属性、职业、武器等信息）
     */
    void update(const Player& player);
    /// 绘制属性面板（背景+文字行）
    void draw(sf::RenderWindow& window) const;

private:
    const sf::Font& font_;                ///< 字体引用
    bool visible_ = false;                ///< 是否可见
    sf::RectangleShape background_;       ///< 半透明背景
    std::vector<sf::Text> lines_;         ///< 属性信息文字行
};
