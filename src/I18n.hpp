/**
 * @file I18n.hpp
 * @brief 国际化系统 - 支持中文/英文切换
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 支持的语言
 */
enum class Language { EN, ZH };

/**
 * @class I18n
 * @brief 轻量级国际化系统
 */
class I18n {
public:
    static I18n& instance();

    void setLanguage(Language lang);
    Language getLanguage() const;
    void toggleLanguage();

    /**
     * @brief 获取翻译文本
     * @param key 翻译键
     * @return 当前语言的翻译文本，找不到则返回key本身
     */
    std::string t(const std::string& key) const;

    /**
     * @brief 获取当前语言名称
     */
    std::string getLanguageName() const;

private:
    I18n();
    Language current_ = Language::ZH;

    // 翻译表: key -> {en, zh}
    std::unordered_map<std::string, std::pair<std::string, std::string>> translations_;

    void initTranslations();
};
