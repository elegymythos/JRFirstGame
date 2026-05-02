/**
 * @file I18n.hpp
 * @brief 国际化系统 - 支持中文/英文切换
 *
 * 轻量级单例国际化系统，使用哈希表存储翻译键值对。
 * 每个键对应一对翻译：{英文, 中文}。
 * 默认语言为中文（ZH），可通过 L 键或右上角按钮切换。
 * 找不到翻译键时，直接返回键本身作为显示文本。
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @enum Language
 * @brief 支持的语言
 */
enum class Language { EN, ZH };

/**
 * @class I18n
 * @brief 国际化单例
 *
 * 使用方式：I18n::instance().t("key")
 * 翻译表在构造时初始化，覆盖所有界面文字。
 */
class I18n {
public:
    /// 获取单例引用
    static I18n& instance();

    /// 设置当前语言
    void setLanguage(Language lang);
    /// 获取当前语言
    Language getLanguage() const;
    /// 切换语言（EN↔ZH）
    void toggleLanguage();

    /**
     * @brief 获取翻译文本
     * @param key 翻译键
     * @return 当前语言的翻译文本，找不到则返回 key 本身
     */
    std::string t(const std::string& key) const;

    /**
     * @brief 获取当前语言显示名称
     * @return "EN" 或 "中文"
     */
    std::string getLanguageName() const;

private:
    I18n();
    Language current_ = Language::ZH;  ///< 当前语言，默认中文

    /// 翻译表: key → {英文, 中文}
    std::unordered_map<std::string, std::pair<std::string, std::string>> translations_;

    /// 初始化所有翻译键值对
    void initTranslations();
};
