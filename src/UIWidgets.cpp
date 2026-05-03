/**
 * @file UIWidgets.cpp
 * @brief UI基础控件实现
 */

#include "UIWidgets.hpp"
#include "I18n.hpp"

// ======================== Button 实现 ========================

Button::Button(const sf::Font& font, const std::string& label, sf::Vector2f pos, sf::Vector2f size, Callback onClick)
    : text(font, sf::String::fromUtf8(label.begin(), label.end()), 20), callback(onClick) {
    shape.setSize(size);
    shape.setPosition(pos);
    shape.setFillColor(sf::Color::White);
    shape.setOutlineThickness(2);
    shape.setOutlineColor(sf::Color::Black);
    text.setFillColor(sf::Color::Black);
    sf::FloatRect b = text.getLocalBounds();
    text.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    text.setPosition({pos.x + size.x / 2.f, pos.y + size.y / 2.f});
}

void Button::handleEvent(sf::Vector2f mousePos, bool isClicked) {
    bool hovered = shape.getGlobalBounds().contains(mousePos);
    shape.setFillColor(hovered ? sf::Color(200, 200, 255) : sf::Color::White);
    if (hovered && isClicked && callback) {
        AudioManager::instance().playSFX("button_click");
        callback();
    }
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(shape);
    window.draw(text);
}

// ======================== InputField 实现 ========================

InputField::InputField(const sf::Font& font, sf::Vector2f pos, sf::Vector2f size,
                       bool passwordMode, size_t maxLen, bool allowSpace)
    : text(font, "", 20), isPassword(passwordMode), maxLength(maxLen), allowSpace(allowSpace) {
    box.setSize(size);
    box.setPosition(pos);
    box.setFillColor(sf::Color::White);
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color(200, 200, 200));
    text.setFillColor(sf::Color::Black);
    text.setPosition({pos.x + 10.f, pos.y + 5.f});
}

void InputField::setString(const std::string& str) {
    if (utf8Length(str) <= maxLength) {
        content = str;
    } else {
        size_t charCount = 0;
        size_t bytePos = 0;
        for (size_t i = 0; i < str.length() && charCount < maxLength; ) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if ((c & 0x80) == 0) i += 1;
            else if ((c & 0xE0) == 0xC0) i += 2;
            else if ((c & 0xF0) == 0xE0) i += 3;
            else if ((c & 0xF8) == 0xF0) i += 4;
            else i += 1;
            charCount++;
            bytePos = i;
        }
        content = str.substr(0, bytePos);
    }
    updateDisplayText();
}

void InputField::updateCursor() {
    if (isFocused) {
        if (cursorClock.getElapsedTime().asSeconds() > 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
            updateDisplayText();
        }
    } else {
        if (showCursor) { showCursor = false; updateDisplayText(); }
    }
}

void InputField::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        isFocused = box.getGlobalBounds().contains(mousePos);
        if (isFocused) { showCursor = true; cursorClock.restart(); }
        else showCursor = false;
        updateDisplayText();
        box.setOutlineColor(isFocused ? sf::Color::Blue : sf::Color(200, 200, 200));
    }
    if (isFocused) {
        if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
            char32_t unicode = textEvent->unicode;
            if (unicode == U'\b') {
                if (!content.empty()) {
                    size_t lastPos = content.length() - 1;
                    while (lastPos > 0 && (content[lastPos] & 0xC0) == 0x80) {
                        --lastPos;
                    }
                    content.erase(lastPos);
                }
            } else if (unicode >= 32 && utf8Length(content) < maxLength) {
                if (allowSpace || unicode != U' ') {
                    if (unicode < 0x80) {
                        content += static_cast<char>(unicode);
                    } else if (unicode < 0x800) {
                        content += static_cast<char>(0xC0 | (unicode >> 6));
                        content += static_cast<char>(0x80 | (unicode & 0x3F));
                    } else if (unicode < 0x10000) {
                        content += static_cast<char>(0xE0 | (unicode >> 12));
                        content += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                        content += static_cast<char>(0x80 | (unicode & 0x3F));
                    } else if (unicode < 0x110000) {
                        content += static_cast<char>(0xF0 | (unicode >> 18));
                        content += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
                        content += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                        content += static_cast<char>(0x80 | (unicode & 0x3F));
                    }
                }
            }
            updateDisplayText();
        }
    }
}

void InputField::updateDisplayText() {
    if (isPassword) {
        std::string stars(utf8Length(content), '*');
        if (isFocused && showCursor) stars += "_";
        text.setString(sf::String::fromUtf8(stars.begin(), stars.end()));
    } else {
        std::string display = content + (isFocused && showCursor ? "_" : "");
        text.setString(sf::String::fromUtf8(display.begin(), display.end()));
    }
}

void InputField::clear() { content.clear(); updateDisplayText(); }
void InputField::draw(sf::RenderWindow& window) const { window.draw(box); window.draw(text); }
std::string InputField::getString() const { return content; }

// ======================== AttributePanel 实现 ========================

AttributePanel::AttributePanel(const sf::Font& font) : font_(font) {
    background_.setSize(sf::Vector2f(220, 260));
    background_.setFillColor(sf::Color(0, 0, 0, 160));
    background_.setPosition(sf::Vector2f(570, 10));
}

void AttributePanel::toggle() { visible_ = !visible_; }
bool AttributePanel::isVisible() const { return visible_; }

void AttributePanel::update(const Player& player) {
    lines_.clear();
    if (!visible_) return;

    Attributes finalAttr = player.getFinalAttributes();
    Attributes levelBonus = player.levelData.getLevelBonus();

    auto addLine = [&](const std::string& str) {
        sf::Text t(font_, sf::String::fromUtf8(str.begin(), str.end()), 14);
        t.setFillColor(sf::Color::White);
        lines_.push_back(t);
    };

    addLine("=== Attributes (Tab) ===");
    addLine("STR: " + std::to_string(player.baseAttributes.strength) +
            "+" + std::to_string(player.classConfig.bonus.strength + levelBonus.strength) +
            "=" + std::to_string(finalAttr.strength));
    addLine("AGI: " + std::to_string(player.baseAttributes.agility) +
            "+" + std::to_string(player.classConfig.bonus.agility + levelBonus.agility) +
            "=" + std::to_string(finalAttr.agility));
    addLine("MAG: " + std::to_string(player.baseAttributes.magic) +
            "+" + std::to_string(player.classConfig.bonus.magic + levelBonus.magic) +
            "=" + std::to_string(finalAttr.magic));
    addLine("INT: " + std::to_string(player.baseAttributes.intelligence) +
            "+" + std::to_string(player.classConfig.bonus.intelligence + levelBonus.intelligence) +
            "=" + std::to_string(finalAttr.intelligence));
    addLine("VIT: " + std::to_string(player.baseAttributes.vitality) +
            "+" + std::to_string(player.classConfig.bonus.vitality + levelBonus.vitality) +
            "=" + std::to_string(finalAttr.vitality));
    addLine("LUK: " + std::to_string(player.baseAttributes.luck) +
            "+" + std::to_string(player.classConfig.bonus.luck + levelBonus.luck) +
            "=" + std::to_string(finalAttr.luck));
    addLine("");
    addLine("Class: " + std::string(player.classConfig.name));
    addLine("Level: " + std::to_string(player.levelData.level));
    addLine("EXP: " + std::to_string(player.levelData.experience) +
            "/" + std::to_string(player.levelData.expToNextLevel));
    addLine("Score: " + std::to_string(player.totalScore));
    if (player.equippedWeapon) {
        std::string weaponName = player.equippedWeapon->name;
        if (weaponName == "Iron Sword") weaponName = I18n::instance().t("iron_sword");
        else if (weaponName == "Flame Artifact") weaponName = I18n::instance().t("flame_artifact");
        else if (weaponName == "Shadow Dagger") weaponName = I18n::instance().t("shadow_dagger");

        addLine(I18n::instance().t("weapon") + " " + weaponName);
    } else {
        addLine(I18n::instance().t("weapon") + " " + I18n::instance().t("none"));
    }

    float y = 15.f;
    for (auto& t : lines_) {
        t.setPosition(sf::Vector2f(575, y));
        y += 18.f;
    }
}

void AttributePanel::draw(sf::RenderWindow& window) const {
    if (!visible_) return;
    window.draw(background_);
    for (const auto& t : lines_) window.draw(t);
}
