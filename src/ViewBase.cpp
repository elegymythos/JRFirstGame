/**
 * @file ViewBase.cpp
 * @brief 视图基类实现
 */

#include "ViewBase.hpp"
#include "I18n.hpp"

View::View(const sf::Font& font) : globalFont(font), langText_(font, "", 14) {
    langBg_.setSize({80, 28});
    langBg_.setFillColor(sf::Color(240, 240, 240));
    langBg_.setOutlineThickness(1);
    langBg_.setOutlineColor(sf::Color(100, 100, 100));
    langText_.setFillColor(sf::Color::Black);
}

void View::drawLangButton(sf::RenderWindow& window, sf::Vector2f pos) {
    langBg_.setPosition(pos);
    std::string langName = I18n::instance().getLanguageName();
    langText_.setString(sf::String::fromUtf8(langName.begin(), langName.end()));
    langText_.setPosition({pos.x + 10, pos.y + 4});

    if (langButtonHovered_) {
        langBg_.setFillColor(sf::Color(200, 220, 255));
        langBg_.setOutlineColor(sf::Color(50, 100, 200));
    } else {
        langBg_.setFillColor(sf::Color(240, 240, 240));
        langBg_.setOutlineColor(sf::Color(100, 100, 100));
    }

    window.draw(langBg_);
    window.draw(langText_);
}

bool View::handleLangButton(sf::Vector2f mousePos, bool isClicked, sf::Vector2f pos) {
    sf::FloatRect bounds(pos, {80, 28});
    langButtonHovered_ = bounds.contains(mousePos);

    if (langButtonHovered_ && isClicked) {
        I18n::instance().toggleLanguage();
        languageChanged = true;
        return true;
    }
    return false;
}
