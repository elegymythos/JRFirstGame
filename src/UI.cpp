/**
 * @file UI.cpp
 * @brief 用户界面模块实现
 */

#include "UI.hpp"
#include "Database.hpp"
#include "Network.hpp"
#include "GameLogic.hpp"
#include "MapSystem.hpp"
#include "Utils.hpp"
#include "I18n.hpp"
#include <set>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <map>

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
    if (hovered && isClicked && callback) callback();
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
    // Truncate to maxLength UTF-8 characters (not bytes)
    if (utf8Length(str) <= maxLength) {
        content = str;
    } else {
        // Need to truncate - find the byte position for maxLength characters
        size_t charCount = 0;
        size_t bytePos = 0;
        for (size_t i = 0; i < str.length() && charCount < maxLength; ) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if ((c & 0x80) == 0) {
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                i += 4;
            } else {
                i += 1;
            }
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
                // Backspace: remove last UTF-8 character (may be multi-byte)
                if (!content.empty()) {
                    // Find the start of the last UTF-8 character
                    size_t lastPos = content.length() - 1;
                    // Move backwards while we're in a continuation byte (10xxxxxx)
                    while (lastPos > 0 && (content[lastPos] & 0xC0) == 0x80) {
                        --lastPos;
                    }
                    content.erase(lastPos);
                }
            } else if (unicode >= 32 && utf8Length(content) < maxLength) {
                // Accept all printable Unicode characters (not just ASCII)
                // Use UTF-8 character count instead of byte count
                if (allowSpace || unicode != U' ') {
                    // Encode char32_t to UTF-8
                    if (unicode < 0x80) {
                        // 1-byte sequence: 0xxxxxxx
                        content += static_cast<char>(unicode);
                    } else if (unicode < 0x800) {
                        // 2-byte sequence: 110xxxxx 10xxxxxx
                        content += static_cast<char>(0xC0 | (unicode >> 6));
                        content += static_cast<char>(0x80 | (unicode & 0x3F));
                    } else if (unicode < 0x10000) {
                        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
                        content += static_cast<char>(0xE0 | (unicode >> 12));
                        content += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
                        content += static_cast<char>(0x80 | (unicode & 0x3F));
                    } else if (unicode < 0x110000) {
                        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
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
        // Use UTF-8 character count for password stars
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
        // Translate weapon name
        std::string weaponName = player.equippedWeapon->name;
        if (weaponName == "Iron Sword") weaponName = I18n::instance().t("iron_sword");
        else if (weaponName == "Flame Artifact") weaponName = I18n::instance().t("flame_artifact");
        else if (weaponName == "Shadow Dagger") weaponName = I18n::instance().t("shadow_dagger");

        addLine(I18n::instance().t("weapon") + " " + weaponName);
    } else {
        addLine(I18n::instance().t("weapon") + " " + I18n::instance().t("none"));
    }

    // 定位
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

// ======================== View 基类 ========================

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
    
    // Change color on hover
    if (langButtonHovered_) {
        langBg_.setFillColor(sf::Color(200, 220, 255));  // Light blue on hover
        langBg_.setOutlineColor(sf::Color(50, 100, 200));  // Darker blue outline
    } else {
        langBg_.setFillColor(sf::Color(240, 240, 240));  // Normal gray
        langBg_.setOutlineColor(sf::Color(100, 100, 100));  // Normal gray outline
    }
    
    window.draw(langBg_);
    window.draw(langText_);
}

bool View::handleLangButton(sf::Vector2f mousePos, bool isClicked, sf::Vector2f pos) {
    sf::FloatRect bounds(pos, {80, 28});
    langButtonHovered_ = bounds.contains(mousePos);  // Update hover state
    
    if (langButtonHovered_ && isClicked) {
        I18n::instance().toggleLanguage();
        languageChanged = true;  // 设置刷新标志
        return true;
    }
    return false;
}

// ======================== LoginView 实现 ========================

LoginView::LoginView(const sf::Font& font, std::function<void(std::string)> changeView,
                     std::string& username, Database& database)
    : View(font), db(database), usernameRef(username),
      usernameInput(globalFont, {540,200}, {200,40}, false, 16, false),
      passwordInput(globalFont, {540,280}, {200,40}, true, 16, false),
      confirmBtn(globalFont, I18n::instance().t("login"), {540,360}, {90,45},
            [this, changeView]() {
                std::string inputUser = usernameInput.getString();
                std::string inputPass = passwordInput.getString();
                auto cred = db.getCredentials(inputUser);
                if (cred && cred->second == hashPassword(inputPass, cred->first)) {
                    usernameRef = inputUser;
                    changeView("SELECT_LEVEL");
                } else {
                    std::string err = I18n::instance().t("invalid_cred");
                    statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                    passwordInput.clear();
                }
            }),
      backBtn(font, I18n::instance().t("back"), {650,360}, {90,45}, [changeView](){ changeView("MENU"); }),
      title(font, "", 35), statusText(font, "", 18) {
    std::string titleStr = I18n::instance().t("login_title");
    title.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    title.setPosition({590,100}); title.setFillColor(sf::Color::Blue);
    statusText.setPosition({540,330}); statusText.setFillColor(sf::Color::Red);
}

void LoginView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    usernameInput.handleEvent(event, mPos);
    passwordInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            confirmBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
            handleLangButton(mPos, true);
        }
    }
}

void LoginView::update(sf::Vector2f mPos, sf::Vector2u) {
    confirmBtn.handleEvent(mPos, false);
    backBtn.handleEvent(mPos, false);
}

void LoginView::draw(sf::RenderWindow& window) {
    window.draw(title);
    usernameInput.updateCursor(); passwordInput.updateCursor();
    usernameInput.draw(window); passwordInput.draw(window);
    confirmBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
    drawLangButton(window);
}

// ======================== SignUpView 实现 ========================

SignUpView::SignUpView(const sf::Font& font, std::function<void(std::string)> changeView, Database& database)
    : View(font), db(database),
      usernameInput(globalFont, {540,200}, {200,40}, false, 16, false),
      passwordInput(globalFont, {540,280}, {200,40}, true, 16, false),
      regBtn(globalFont, I18n::instance().t("signup"), {540,360}, {90,45},
            [this, changeView]() {
                std::string user = usernameInput.getString();
                std::string pass = passwordInput.getString();
                if (user.empty() || pass.empty()) {
                    std::string err = I18n::instance().t("empty_field");
                    statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                    return;
                }
                if (db.userExists(user)) {
                    std::string err = I18n::instance().t("user_exists");
                    statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                    return;
                }
                std::string salt = generateRandomSalt();
                std::string hash = hashPassword(pass, salt);
                if (db.addUser(user, salt, hash)) {
                    std::string msg = I18n::instance().t("reg_success");
                    statusText.setString(sf::String::fromUtf8(msg.begin(), msg.end()));
                    changeView("LOGIN");
                } else {
                    std::string err = I18n::instance().t("db_error");
                    statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                }
            }),
      backBtn(font, I18n::instance().t("back"), {650,360}, {90,45}, [changeView](){ changeView("MENU"); }),
      title(font, "", 35), statusText(font, "", 18) {
    std::string titleStr = I18n::instance().t("signup_title");
    title.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    statusText.setPosition({540,330}); statusText.setFillColor(sf::Color::Red);
    title.setPosition({570,100}); title.setFillColor(sf::Color::Green);
}

void SignUpView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    usernameInput.handleEvent(event, mPos);
    passwordInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            regBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
            handleLangButton(mPos, true);
        }
    }
}

void SignUpView::update(sf::Vector2f mPos, sf::Vector2u) {
    usernameInput.updateCursor(); passwordInput.updateCursor();
    regBtn.handleEvent(mPos, false); backBtn.handleEvent(mPos, false);
}

void SignUpView::draw(sf::RenderWindow& window) {
    window.draw(title);
    usernameInput.draw(window); passwordInput.draw(window);
    regBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
    drawLangButton(window);
}

// ======================== MainMenuView 实现 ========================

MainMenuView::MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView) : View(font) {
    buttons.emplace_back(font, I18n::instance().t("login"), sf::Vector2f{540,200}, sf::Vector2f{200,50}, [changeView](){ changeView("LOGIN"); });
    buttons.emplace_back(font, I18n::instance().t("signup"), sf::Vector2f{540,280}, sf::Vector2f{200,50}, [changeView](){ changeView("SIGNUP"); });
    buttons.emplace_back(font, I18n::instance().t("exit"), sf::Vector2f{540,360}, sf::Vector2f{200,50}, [](){ exit(0); });
}

void MainMenuView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2f mPos = window.mapPixelToCoords(mbp->position);
        for (auto& b : buttons) b.handleEvent(mPos, true);
        handleLangButton(mPos, true);
    }
}

void MainMenuView::update(sf::Vector2f mPos, sf::Vector2u) { for (auto& b : buttons) b.handleEvent(mPos, false); }
void MainMenuView::draw(sf::RenderWindow& window) { for (auto& b : buttons) b.draw(window); drawLangButton(window); }

// ======================== LevelSelectView 实现 ========================

LevelSelectView::LevelSelectView(const sf::Font& font, std::function<void(std::string)> cv,
                                 bool showWelcome, bool hasCharacter)
    : View(font), changeView(cv), welcomeMsg(font, "", 30) {
    if (showWelcome) {
        std::string welcome = I18n::instance().t("welcome");
        welcomeMsg.setString(sf::String::fromUtf8(welcome.begin(), welcome.end()));
        welcomeMsg.setFillColor(sf::Color::Blue);
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
        welcomeMsg.setPosition({640,200});
        currentState = State::Welcome;
    } else {
        currentState = State::ButtonsReady;
        std::string choose = I18n::instance().t("choose_mode");
        welcomeMsg.setString(sf::String::fromUtf8(choose.begin(), choose.end()));
        welcomeMsg.setFillColor(sf::Color::Blue);
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
        welcomeMsg.setPosition({640,200});
    }

    if (!hasCharacter) {
        buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("create_char"), sf::Vector2f{540,260}, sf::Vector2f{200,50},
            [cv](){ cv("CHAR_CREATE"); }));
    } else {
        buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("load_char"), sf::Vector2f{540,260}, sf::Vector2f{200,50},
            [cv](){ cv("CHAR_SELECT"); }));
    }
    buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("level_mode"), sf::Vector2f{540,320}, sf::Vector2f{200,50},
        [cv, hasCharacter](){
            if (hasCharacter) {
                cv("CHAR_SELECT");  // 先选择角色
            } else {
                cv("CHAR_CREATE");  // 没有角色则创建
            }
        }));
    buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("coop_mode"), sf::Vector2f{540,400}, sf::Vector2f{200,50},
        [cv](){ cv("ONLINE_LOBBY"); }));
    buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("rankings"), sf::Vector2f{440,480}, sf::Vector2f{200,50},
        [cv](){ cv("RANKINGS"); }));
    buttons.push_back(std::make_unique<Button>(font, I18n::instance().t("back"), sf::Vector2f{640,480}, sf::Vector2f{200,50},
        [cv](){ cv("MENU"); }));
}

void LevelSelectView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (currentState != State::ButtonsReady) return;
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            for (auto& b : buttons) b->handleEvent(mPos, true);
            handleLangButton(mPos, true);
        }
    }
}

void LevelSelectView::update(sf::Vector2f mPos, sf::Vector2u) {
    if (currentState == State::Welcome && timer.getElapsedTime().asSeconds() > delayTime) {
        currentState = State::ButtonsReady;
        std::string choose = I18n::instance().t("choose_mode");
        welcomeMsg.setString(sf::String::fromUtf8(choose.begin(), choose.end()));
        auto b = welcomeMsg.getLocalBounds();
        welcomeMsg.setOrigin({b.position.x + b.size.x/2.f, b.position.y + b.size.y/2.f});
    }
    if (currentState == State::ButtonsReady) {
        for (auto& b : buttons) b->handleEvent(mPos, false);
    }
}

void LevelSelectView::draw(sf::RenderWindow& window) {
    window.draw(welcomeMsg);
    if (currentState == State::ButtonsReady) {
        for (auto& b : buttons) b->draw(window);
    }
    drawLangButton(window);
}

// ======================== CharacterCreateView 实现 ========================

CharacterCreateView::CharacterCreateView(const sf::Font& font, std::function<void(std::string)> cv,
                                         const std::string& username, Database& database, int slot)
    : View(font), db(database), username_(username), slot_(slot), changeView(cv),
      title_(font, "", 30),
      attrText_(font, "", 18),
      classInfoText_(font, "", 18),
      statusText_(font, "", 18),
      warriorBtn_(font, I18n::instance().t("warrior"), {440, 300}, {120, 45}, [this](){ selectClass(CharacterClass::Warrior); }),
      mageBtn_(font, I18n::instance().t("mage"), {590, 300}, {120, 45}, [this](){ selectClass(CharacterClass::Mage); }),
      assassinBtn_(font, I18n::instance().t("assassin"), {740, 300}, {120, 45}, [this](){ selectClass(CharacterClass::Assassin); }),
      rerollBtn_(font, I18n::instance().t("reroll"), {540, 240}, {100, 40}, [this](){ rerollAttributes(); }),
      confirmBtn_(font, I18n::instance().t("confirm"), {540, 500}, {100, 50}, [this](){ confirmCreate(); }),
      backBtn_(font, I18n::instance().t("back"), {690, 500}, {100, 50}, [cv](){ cv("SELECT_LEVEL"); }) {
    std::string titleStr = I18n::instance().t("create_char_title");
    title_.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    title_.setPosition({490, 30}); title_.setFillColor(sf::Color::Blue);
    attrText_.setPosition({390, 80}); attrText_.setFillColor(sf::Color::Black);
    classInfoText_.setPosition({390, 370}); classInfoText_.setFillColor(sf::Color(0, 139, 139));
    statusText_.setPosition({390, 560}); statusText_.setFillColor(sf::Color::Red);
    rerollAttributes();
}

void CharacterCreateView::rerollAttributes() {
    currentAttributes_ = generateRandomAttributes();
    std::stringstream ss;
    ss << I18n::instance().t("attr_random") << "\n"
       << I18n::instance().t("str") << ": " << currentAttributes_.strength
       << "  " << I18n::instance().t("agi") << ": " << currentAttributes_.agility
       << "  " << I18n::instance().t("mag") << ": " << currentAttributes_.magic << "\n"
       << I18n::instance().t("int") << ": " << currentAttributes_.intelligence
       << "  " << I18n::instance().t("vit") << ": " << currentAttributes_.vitality
       << "  " << I18n::instance().t("luk") << ": " << currentAttributes_.luck << "\n"
       << I18n::instance().t("total") << ": " << currentAttributes_.total();
    std::string str = ss.str();
    attrText_.setString(sf::String::fromUtf8(str.begin(), str.end()));
}

void CharacterCreateView::selectClass(CharacterClass cls) {
    selectedClass_ = cls;
    classConfig_ = getClassConfig(cls);
    classSelected_ = true;

    Attributes total = currentAttributes_ + classConfig_.bonus;
    std::stringstream ss;
    ss << I18n::instance().t("class_label") << " " << classConfig_.name << "\n"
       << I18n::instance().t("bonus") << " " << I18n::instance().t("str") << "+" << classConfig_.bonus.strength
       << " " << I18n::instance().t("agi") << "+" << classConfig_.bonus.agility
       << " " << I18n::instance().t("mag") << "+" << classConfig_.bonus.magic << "\n"
       << I18n::instance().t("total") << ": " << I18n::instance().t("str") << "=" << total.strength 
       << " " << I18n::instance().t("agi") << "=" << total.agility
       << " " << I18n::instance().t("mag") << "=" << total.magic 
       << " " << I18n::instance().t("int") << "=" << total.intelligence
       << " " << I18n::instance().t("vit") << "=" << total.vitality 
       << " " << I18n::instance().t("luk") << "=" << total.luck;
    std::string str = ss.str();
    classInfoText_.setString(sf::String::fromUtf8(str.begin(), str.end()));
}

void CharacterCreateView::confirmCreate() {
    if (!classSelected_) {
        std::string err = I18n::instance().t("select_class_first");
        statusText_.setString(sf::String::fromUtf8(err.begin(), err.end()));
        return;
    }

    CharacterData data;
    data.username = username_;
    data.slot = slot_;
    if (data.slot < 0) {
        // Auto-assign: find first empty slot
        auto existing = db.loadAllCharacters(username_);
        std::set<int> usedSlots;
        for (auto& c : existing) usedSlots.insert(c.slot);
        for (int s = 0; s < 3; ++s) {
            if (usedSlots.find(s) == usedSlots.end()) { data.slot = s; break; }
        }
        if (data.slot < 0) data.slot = 0;  // fallback
    }
    data.classType = selectedClass_;
    data.level = 1;
    data.experience = 0;
    data.attributes = currentAttributes_;
    db.saveCharacter(data);
    changeView("SELECT_LEVEL");
}

void CharacterCreateView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            warriorBtn_.handleEvent(mPos, true);
            mageBtn_.handleEvent(mPos, true);
            assassinBtn_.handleEvent(mPos, true);
            rerollBtn_.handleEvent(mPos, true);
            confirmBtn_.handleEvent(mPos, true);
            backBtn_.handleEvent(mPos, true);
        }
    }
}

void CharacterCreateView::update(sf::Vector2f mPos, sf::Vector2u) {
    warriorBtn_.handleEvent(mPos, false);
    mageBtn_.handleEvent(mPos, false);
    assassinBtn_.handleEvent(mPos, false);
    rerollBtn_.handleEvent(mPos, false);
    confirmBtn_.handleEvent(mPos, false);
    backBtn_.handleEvent(mPos, false);
}

void CharacterCreateView::draw(sf::RenderWindow& window) {
    window.draw(title_);
    window.draw(attrText_);
    warriorBtn_.draw(window);
    mageBtn_.draw(window);
    assassinBtn_.draw(window);
    rerollBtn_.draw(window);
    window.draw(classInfoText_);
    confirmBtn_.draw(window);
    backBtn_.draw(window);
    window.draw(statusText_);
    drawLangButton(window);  // 添加语言切换按钮
}

// ======================== CharacterSelectView 实现（多角色） ========================

CharacterSelectView::CharacterSelectView(const sf::Font& font, std::function<void(std::string)> cv,
                                         const std::string& username, Database& database)
    : View(font), db(database), username_(username), changeView(cv),
      title_(font, "", 30),
      backBtn_(font, I18n::instance().t("back"), {590, 500}, {100, 50}, [cv](){ cv("SELECT_LEVEL"); }) {
    std::string titleStr = I18n::instance().t("select_char_title");
    title_.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    title_.setPosition({440, 30}); title_.setFillColor(sf::Color::Blue);
    refreshSlots();
}

void CharacterSelectView::refreshSlots() {
    characters_ = db.loadAllCharacters(username_);
    slotTexts_.clear();
    selectBtns_.clear();
    deleteBtns_.clear();
    createBtns_.clear();

    std::set<int> usedSlots;
    for (auto& c : characters_) usedSlots.insert(c.slot);

    for (int s = 0; s < 3; ++s) {
        float y = 100 + s * 120;
        auto it = std::find_if(characters_.begin(), characters_.end(),
                               [s](const CharacterData& c){ return c.slot == s; });

        if (it != characters_.end()) {
            // Occupied slot
            std::string className;
            switch (it->classType) {
                case CharacterClass::Warrior: className = I18n::instance().t("warrior"); break;
                case CharacterClass::Mage: className = I18n::instance().t("mage"); break;
                case CharacterClass::Assassin: className = I18n::instance().t("assassin"); break;
            }
            std::stringstream ss;
            ss << I18n::instance().t("char_slot") << " " << s << ": " << className
               << " " << I18n::instance().t("lv") << it->level
               << " " << I18n::instance().t("score") << it->score;
            std::string slotStr = ss.str();
            slotTexts_.emplace_back(globalFont, sf::String::fromUtf8(slotStr.begin(), slotStr.end()), 20);
            slotTexts_.back().setPosition({390, y});
            slotTexts_.back().setFillColor(sf::Color::Black);

            selectBtns_.push_back(std::make_unique<Button>(globalFont, I18n::instance().t("select_char"),
                sf::Vector2f{740, y}, sf::Vector2f{80, 35},
                [this, s](){
                    // 保存选中的槽位到数据库，然后进入游戏
                    db.saveSelectedSlot(username_, s);
                    changeView("LEVEL_GAME");
                }));
            deleteBtns_.push_back(std::make_unique<Button>(globalFont, I18n::instance().t("delete_char"),
                sf::Vector2f{840, y}, sf::Vector2f{80, 35},
                [this, s](){ db.deleteCharacter(username_, s); needsRefresh_ = true; }));
        } else {
            // Empty slot
            std::stringstream ss;
            ss << I18n::instance().t("char_slot") << " " << s << ": " << I18n::instance().t("empty_slot");
            std::string slotStr = ss.str();
            slotTexts_.emplace_back(globalFont, sf::String::fromUtf8(slotStr.begin(), slotStr.end()), 20);
            slotTexts_.back().setPosition({390, y});
            slotTexts_.back().setFillColor(sf::Color(128, 128, 128));

            createBtns_.push_back(std::make_unique<Button>(globalFont, I18n::instance().t("new_char"),
                sf::Vector2f{740, y}, sf::Vector2f{100, 35},
                [this, s](){ /* Navigate to char create with slot */ changeView("CHAR_CREATE"); }));
        }
    }
}

void CharacterSelectView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            backBtn_.handleEvent(mPos, true);
            for (auto& btn : selectBtns_) btn->handleEvent(mPos, true);
            for (auto& btn : deleteBtns_) btn->handleEvent(mPos, true);
            for (auto& btn : createBtns_) btn->handleEvent(mPos, true);
        }
    }
}

void CharacterSelectView::update(sf::Vector2f mPos, sf::Vector2u) {
    // Perform deferred refresh if needed
    if (needsRefresh_) {
        needsRefresh_ = false;
        refreshSlots();
    }
    
    backBtn_.handleEvent(mPos, false);
    for (auto& btn : selectBtns_) btn->handleEvent(mPos, false);
    for (auto& btn : deleteBtns_) btn->handleEvent(mPos, false);
    for (auto& btn : createBtns_) btn->handleEvent(mPos, false);
}

void CharacterSelectView::draw(sf::RenderWindow& window) {
    window.draw(title_);
    for (auto& t : slotTexts_) window.draw(t);
    for (auto& btn : selectBtns_) btn->draw(window);
    for (auto& btn : deleteBtns_) btn->draw(window);
    for (auto& btn : createBtns_) btn->draw(window);
    backBtn_.draw(window);
    drawLangButton(window);  // 添加语言切换按钮
}

// ======================== ActualGameView 实现（重构） ========================

ActualGameView::ActualGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               const std::string& name, Database& database)
    : View(font), db(database), username(name), player({400,300}),
      attrPanel(font),
      infoText(font, "", 20),
      gameOverText(font, "", 40),
      pauseText(font, "", 40),
      victoryText(font, "", 40),
      backBtn(font, I18n::instance().t("back_menu"), {1150,20}, {120,40}, [cv](){ cv("SELECT_LEVEL"); }),
      restartBtn(font, I18n::instance().t("restart"), {590,400}, {100,50}, [this](){ restartGame(); }),
      resumeBtn(font, I18n::instance().t("resume"), {590,350}, {100,50}, [this](){ 
          if (paused) {
              // Resuming - accumulate paused time
              float currentTime = gameClock.getElapsedTime().asSeconds();
              pausedTime += (currentTime - lastPauseTime);
              paused = false;
          }
      }),
      pauseBackBtn(font, I18n::instance().t("back_menu"), {590,420}, {100,50}, [this, cv](){ 
          // Save game before leaving
          db.saveCharacter(buildCharacterData());
          db.saveRanking(username, player.levelData.level, player.totalScore);
          cv("SELECT_LEVEL"); 
      }),
      victoryContinueBtn(font, I18n::instance().t("continue"), {540,400}, {100,50}, [this](){ continueFromVictory(); }),
      victoryBackBtn(font, I18n::instance().t("back_menu"), {640,400}, {100,50}, [this, cv](){ 
          // Save game before leaving
          db.saveCharacter(buildCharacterData());
          db.saveRanking(username, player.levelData.level, player.totalScore);
          cv("SELECT_LEVEL"); 
      }),
      changeView(cv) {
    player.username = username;

    // 尝试加载角色数据（使用选中的槽位）
    int slot = db.getSelectedSlot(username);
    auto charData = db.loadCharacter(username, slot);
    if (charData) {
        player.baseAttributes = charData->attributes;
        player.charClass = charData->classType;
        player.classConfig = getClassConfig(charData->classType);
        player.levelData.level = charData->level;
        player.levelData.experience = charData->experience;
        player.levelData.expToNextLevel = 100 * charData->level;
        player.totalScore = charData->score;  // Load saved score
        stageVictoryShown = charData->victoryShown;  // Load victory shown state
        // Restore game time: set pausedTime so that display shows correct elapsed time
        // gameClock starts at 0 on construction, so we set pausedTime = -gameTime
        // so that (gameClock - pausedTime) = gameTime
        pausedTime = -charData->gameTime;
    } else {
        player.baseAttributes = generateRandomAttributes();
        player.charClass = CharacterClass::Warrior;
        player.classConfig = getClassConfig(CharacterClass::Warrior);
    }

    // 初始化武器并自动装备职业对应武器
    player.weaponManager.getList().initDefaults();
    auto weapon = player.weaponManager.getList().findByType(player.classConfig.allowedWeapon);
    if (weapon) player.equipWeapon(weapon->name);
    player.recalcCombatStats();
    player.health = player.maxHealth;

    // 初始化摄像机
    gameView.setSize(sf::Vector2f(800, 600));
    gameView.setCenter(player.position);

    infoText.setPosition({10,10}); infoText.setFillColor(sf::Color::Black);
    std::string goStr = I18n::instance().t("game_over");
    gameOverText.setString(sf::String::fromUtf8(goStr.begin(), goStr.end()));
    gameOverText.setPosition({490,300}); gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setCharacterSize(30);
    
    // Initialize pause text
    std::string pauseStr = I18n::instance().t("paused");
    pauseText.setString(sf::String::fromUtf8(pauseStr.begin(), pauseStr.end()));
    pauseText.setPosition({590,280}); pauseText.setFillColor(sf::Color::Blue);
    pauseText.setCharacterSize(40);
    
    // Initialize victory text
    std::string victoryStr = I18n::instance().t("stage_victory");
    victoryText.setString(sf::String::fromUtf8(victoryStr.begin(), victoryStr.end()));
    victoryText.setPosition({540,280}); victoryText.setFillColor(sf::Color::Green);
    victoryText.setCharacterSize(40);
}

void ActualGameView::attack() {
    if (!player.canAttack()) return;

    if (player.equippedWeapon && player.equippedWeapon->isRanged) {
        // 远程攻击：发射投射物
        // 找最近敌人方向
        sf::Vector2f dir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(player.position, e.position);
            if (d < minDist) { minDist = d; dir = normalize(e.position - player.position); }
        }
        Projectile proj;
        proj.position = player.position;
        proj.direction = dir;
        proj.speed = 400.f;
        proj.damage = player.calculateDamage();
        proj.maxDistance = player.attackRange;
        proj.traveled = 0.f;
        proj.fromPlayer = true;
        projectiles.push_back(proj);
    } else {
        // 近战攻击
        bool hit = false;
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float dist = distance(player.position, e.position);
            if (dist <= player.attackRange) {
                e.health -= player.calculateDamage();
                hit = true;
            }
        }
        if (hit) {
            attackEffects.push_back(player.position);
            effectTimer = 0.2f;
        }
    }
    player.resetAttackCooldown();
}

void ActualGameView::handleEnemyAttacks(float dt) {
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;

        if (e.isRanged) {
            // 远程敌人发射投射物
            if (e.canAttack()) {
                float dist = distance(e.position, player.position);
                if (dist <= e.attackRange) {
                    Projectile proj;
                    proj.position = e.position;
                    proj.direction = normalize(player.position - e.position);
                    proj.speed = 250.f;
                    proj.damage = e.attackDamage;
                    proj.maxDistance = e.attackRange;
                    proj.traveled = 0.f;
                    proj.fromPlayer = false;
                    projectiles.push_back(proj);
                    e.resetAttackCooldown();
                }
            }
        } else {
            // 近战敌人碰撞攻击
            float dist = distance(player.position, e.position);
            if (dist < player.radius + e.radius) {
                if (e.canAttack()) {
                    player.health -= e.attackDamage;
                    e.resetAttackCooldown();
                    sf::Vector2f dir = player.position - e.position;
                    if (dir != sf::Vector2f(0,0)) dir = normalize(dir);
                    player.position += dir * 20.f;
                    // No clamping - infinite map
                    if (player.health <= 0) {
                        gameOver = true;
                        // Save character and ranking on game over
                        db.saveCharacter(buildCharacterData());
                        db.saveRanking(username, player.levelData.level, player.totalScore);
                    }
                }
            }
        }
    }
}

void ActualGameView::updateProjectiles(float dt) {
    for (auto& proj : projectiles) {
        proj.update(dt);
    }

    // 投射物碰撞检测
    for (auto& proj : projectiles) {
        if (proj.fromPlayer) {
            for (auto& e : enemies) {
                if (!e.isAlive()) continue;
                float dist = distance(proj.position, e.position);
                if (dist < e.radius + 4.f) {
                    e.health -= proj.damage;
                    proj.traveled = proj.maxDistance;  // 标记为已命中
                    break;
                }
            }
        } else {
            float dist = distance(proj.position, player.position);
            if (dist < player.radius + 4.f) {
                player.health -= proj.damage;
                proj.traveled = proj.maxDistance;
                if (player.health <= 0) {
                    gameOver = true;
                    db.saveCharacter(buildCharacterData());
                    db.saveRanking(username, player.levelData.level, player.totalScore);
                }
            }
        }
    }

    // 移除过期投射物
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p){ return p.isExpired(); }), projectiles.end());
}

void ActualGameView::updateInfoText() {
    // Game time (subtract paused time)
    float rawTime = gameClock.getElapsedTime().asSeconds();
    float adjustedTime = rawTime - pausedTime;
    int totalSeconds = static_cast<int>(adjustedTime);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    std::stringstream ss;
    ss << username << "  " << I18n::instance().t("hp") << player.health << "/" << player.maxHealth
       << "  " << I18n::instance().t("lv") << player.levelData.level
       << "  " << I18n::instance().t("score") << player.totalScore
       << "  " << I18n::instance().t("time") << " " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;

    // Weapon info
    if (player.equippedWeapon) {
        // Translate weapon name
        std::string weaponName = player.equippedWeapon->name;
        if (weaponName == "Iron Sword") weaponName = I18n::instance().t("iron_sword");
        else if (weaponName == "Flame Artifact") weaponName = I18n::instance().t("flame_artifact");
        else if (weaponName == "Shadow Dagger") weaponName = I18n::instance().t("shadow_dagger");

        ss << "  " << I18n::instance().t("weapon") << weaponName
           << " (" << I18n::instance().t("range") << static_cast<int>(player.attackRange) << ")";
    }

    ss << "  " << I18n::instance().t("controls");
    std::string info = ss.str();
    infoText.setString(sf::String::fromUtf8(info.begin(), info.end()));
}

void ActualGameView::restartGame() {
    player.position = {400, 300};
    player.velocity = {0, 0};
    player.health = player.maxHealth;
    player.killCount = 0;
    player.totalScore = 0;
    player.levelData = LevelData();
    player.attackCooldown = 0.f;
    enemies.clear();
    projectiles.clear();
    attackEffects.clear();
    effectTimer = 0.f;
    gameOver = false;
    killCount = 0;
    mapSystem = MapSystem();
}

void ActualGameView::continueFromVictory() {
    // Resume from victory - same logic as resume from pause
    float currentTime = gameClock.getElapsedTime().asSeconds();
    pausedTime += (currentTime - lastPauseTime);
    stageVictory = false;
}

CharacterData ActualGameView::buildCharacterData() const {
    CharacterData data;
    data.username = username;
    data.classType = player.charClass;
    data.level = player.levelData.level;
    data.experience = player.levelData.experience;
    data.score = player.totalScore;
    data.gameTime = gameClock.getElapsedTime().asSeconds() - pausedTime;
    data.victoryShown = stageVictoryShown;
    data.attributes = player.baseAttributes;
    return data;
}

void ActualGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    
    // Handle pause toggle with Escape key
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Escape && !gameOver && !stageVictory) {
            if (!paused) {
                // Starting pause - record the time
                lastPauseTime = gameClock.getElapsedTime().asSeconds();
            } else {
                // Resuming - accumulate paused time
                float currentTime = gameClock.getElapsedTime().asSeconds();
                pausedTime += (currentTime - lastPauseTime);
            }
            paused = !paused;
            return;
        }
    }
    
    // If in stage victory, only handle victory buttons (use default view coordinates)
    if (stageVictory) {
        sf::Vector2f uiMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mbp->button == sf::Mouse::Button::Left) {
                victoryContinueBtn.handleEvent(uiMousePos, true);
                victoryBackBtn.handleEvent(uiMousePos, true);
            }
        }
        return;
    }
    
    // If paused, only handle pause menu buttons (use default view coordinates)
    if (paused) {
        sf::Vector2f uiMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), window.getDefaultView());
        if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mbp->button == sf::Mouse::Button::Left) {
                resumeBtn.handleEvent(uiMousePos, true);
                pauseBackBtn.handleEvent(uiMousePos, true);
            }
        }
        return;
    }
    
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            backBtn.handleEvent(mPos, true);
            // 游戏中不处理语言按钮点击
            if (gameOver) restartBtn.handleEvent(mPos, true);
            else attack();
        }
    }
    if (!gameOver) {
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Space) attack();
            if (kp->code == sf::Keyboard::Key::Tab) attrPanel.toggle();
            // 游戏中不处理语言切换按键
            if (kp->code == sf::Keyboard::Key::P) {
                // Save game mid-game
                db.saveCharacter(buildCharacterData());
                db.saveRanking(username, player.levelData.level, player.totalScore);
            }
        }
    }
}

void ActualGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    backBtn.handleEvent(mousePos, false);
    
    // If in stage victory, only update victory buttons (use default view coordinates)
    if (stageVictory) {
        victoryContinueBtn.handleEvent(mousePos, false);
        victoryBackBtn.handleEvent(mousePos, false);
        return;
    }
    
    // If paused, only update pause menu buttons (use default view coordinates)
    if (paused) {
        resumeBtn.handleEvent(mousePos, false);
        pauseBackBtn.handleEvent(mousePos, false);
        return;
    }
    
    if (gameOver) { restartBtn.handleEvent(mousePos, false); return; }
    
    // Check for stage victory (score >= 50) - only trigger once
    if (player.totalScore >= 50 && !stageVictoryShown) {
        stageVictory = true;
        stageVictoryShown = true;  // Prevent re-triggering
        // Record pause time for victory
        lastPauseTime = gameClock.getElapsedTime().asSeconds();
        // Save game on victory
        db.saveCharacter(buildCharacterData());
        db.saveRanking(username, player.levelData.level, player.totalScore);
        return;
    }

    // 玩家移动
    sf::Vector2f move(0,0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) move.y -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) move.y += 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) move.x -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) move.x += 1;
    if (move != sf::Vector2f(0,0)) move = normalize(move);
    player.velocity = move * player.speed;
    player.update(dt);

    // 地图区域检测与敌人刷新
    auto newEnemies = mapSystem.checkAndSpawn(player.position);
    float diffScale = mapSystem.getDifficultyScale();
    // Also scale by time and score (reduced scaling)
    float timeScale = 1.0f + gameClock.getElapsedTime().asSeconds() / 600.f;  // +0.17 per 100s
    float scoreScale = 1.0f + player.totalScore / 1000.f;  // +0.1 per 100 score
    float enemyScale = diffScale * timeScale * scoreScale;
    for (auto& spawn : newEnemies) {
        enemies.emplace_back(spawn.type, spawn.position);
        enemies.back().scaleStats(enemyScale);
    }

    // 敌人AI更新
    for (auto& e : enemies) {
        if (e.isAlive()) {
            e.setTarget(player.position);
            e.update(dt);
        }
    }

    // 敌人攻击
    handleEnemyAttacks(dt);

    // 投射物更新
    updateProjectiles(dt);

    // 处理击杀
    for (auto& e : enemies) {
        if (!e.isAlive() && e.health <= 0) {
            // 刚死亡，计分
            player.totalScore += e.score;
            player.levelData.gainExp(e.score);
            player.recalcCombatStats();
            killCount++;
        }
    }
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e){ return !e.isAlive(); }), enemies.end());

    // 攻击效果
    if (effectTimer > 0) {
        effectTimer -= dt;
        if (effectTimer <= 0) attackEffects.clear();
    }

    // 摄像机跟随
    mapSystem.updateCamera(gameView, player.position);

    // 属性面板更新
    attrPanel.update(player);

    updateInfoText();
}

void ActualGameView::draw(sf::RenderWindow& window) {
    window.setView(gameView);

    // 地图背景
    mapSystem.drawBackground(window);

    // 敌人（菱形+名字）
    for (auto& e : enemies) e.drawWithName(window, &globalFont);

    // 投射物
    for (auto& proj : projectiles) proj.draw(window);

    // 玩家（圆形+名字）
    player.drawWithName(window, &globalFont);

    // 攻击效果
    for (const auto& pos : attackEffects) {
        sf::CircleShape effect(30.f);
        effect.setPosition(pos - sf::Vector2f(30,30));
        effect.setFillColor(sf::Color(255,255,0,128));
        window.draw(effect);
    }

    // 恢复默认视图绘制UI
    window.setView(window.getDefaultView());

    // UI层
    window.draw(infoText);
    backBtn.draw(window);
    attrPanel.draw(window);
    // 游戏中不显示语言切换按钮
    if (gameOver) {
        window.draw(gameOverText);
        restartBtn.draw(window);
    }
    
    // Stage victory overlay
    if (stageVictory) {
        // Semi-transparent overlay
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);
        
        window.draw(victoryText);
        victoryContinueBtn.draw(window);
        victoryBackBtn.draw(window);
    }
    
    // Pause menu overlay
    if (paused) {
        // Semi-transparent overlay
        sf::RectangleShape overlay(sf::Vector2f(1280, 720));
        overlay.setPosition({0, 0});
        overlay.setFillColor(sf::Color(0, 0, 0, 128));
        window.draw(overlay);
        
        window.draw(pauseText);
        resumeBtn.draw(window);
        pauseBackBtn.draw(window);
    }
}

// ======================== RankingsView 实现（重构） ========================

RankingsView::RankingsView(const sf::Font& font, std::function<void(std::string)> cv, Database& database)
    : View(font), db(database), changeView(cv),
      title(font, "", 30),
      backBtn(font, I18n::instance().t("back"), {580,20}, {120,40}, [cv](){ cv("SELECT_LEVEL"); }) {
    std::string titleStr = I18n::instance().t("rankings_title");
    title.setString(sf::String::fromUtf8(titleStr.begin(), titleStr.end()));
    title.setPosition({490,50}); title.setFillColor(sf::Color::Blue);
    refreshRankings();
}

void RankingsView::refreshRankings() {
    rankTexts.clear();
    auto records = db.loadRankings();
    if (records.empty()) {
        std::string str = I18n::instance().t("no_rankings");
        sf::Text empty(globalFont, sf::String::fromUtf8(str.begin(), str.end()), 20);
        empty.setPosition({540,150}); empty.setFillColor(sf::Color::Black);
        rankTexts.push_back(empty);
        return;
    }
    int y = 150;
    for (size_t i=0; i<records.size(); ++i) {
        std::stringstream ss;
        ss << (i+1) << ". " << records[i].username
           << "  Lv:" << records[i].level
           << "  Score:" << records[i].score;
        std::string str = ss.str();
        sf::Text line(globalFont, sf::String::fromUtf8(str.begin(), str.end()), 20);
        line.setPosition({440, static_cast<float>(y)});
        line.setFillColor(sf::Color::Black);
        rankTexts.push_back(line);
        y += 30;
    }
}

void RankingsView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) backBtn.handleEvent(mPos, true);
    }
}

void RankingsView::update(sf::Vector2f mousePos, sf::Vector2u) { backBtn.handleEvent(mousePos, false); }
void RankingsView::draw(sf::RenderWindow& window) {
    window.draw(title);
    for (const auto& t : rankTexts) window.draw(t);
    backBtn.draw(window);
    drawLangButton(window);
}

// ======================== OnlineLobbyView 实现 ========================

OnlineLobbyView::OnlineLobbyView(const sf::Font& font, std::function<void(std::string)> cv, NetworkManager& net)
    : View(font), ipInput(font, {540,150}, {200,40}, false, 15, false),
      portInput(font, {540,220}, {200,40}, false, 5, false),
      createBtn(font, I18n::instance().t("host_coop"), {540,290}, {200,50}, [this,cv](){
            std::string portStr = portInput.getString();
            if (portStr.empty()) {
                std::string err = I18n::instance().t("fill_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) {
                std::string err = I18n::instance().t("invalid_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            network.setPendingPort(port);
            cv("ONLINE_GAME_HOST");
      }),
      joinBtn(font, I18n::instance().t("join_coop"), {540,360}, {200,50}, [this,cv](){
            std::string ip = ipInput.getString();
            std::string portStr = portInput.getString();
            if (ip.empty() || portStr.empty()) {
                std::string err = I18n::instance().t("fill_ip_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            int port;
            try { port = std::stoi(portStr); if (port<1||port>65535) throw std::out_of_range(""); }
            catch(...) {
                std::string err = I18n::instance().t("invalid_port");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            auto resolved = sf::IpAddress::resolve(ip);
            if (!resolved) {
                std::string err = I18n::instance().t("invalid_ip");
                statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
                return;
            }
            network.setPendingPort(port);
            std::string conn = I18n::instance().t("connecting");
            statusText.setString(sf::String::fromUtf8(conn.begin(), conn.end()));
            if (network.connectToServer(*resolved, port)) {
                std::string succ = I18n::instance().t("connected");
                statusText.setString(sf::String::fromUtf8(succ.begin(), succ.end()));
                cv("ONLINE_GAME_CLIENT");
            } else {
                std::string fail = I18n::instance().t("conn_failed");
                statusText.setString(sf::String::fromUtf8(fail.begin(), fail.end()));
            }
      }),
      backBtn(font, I18n::instance().t("back"), {540,430}, {200,50}, [cv](){ cv("SELECT_LEVEL"); }),
      statusText(font, "", 18), changeView(cv), network(net) {
    ipInput.setString("127.0.0.1"); portInput.setString("54000");
    statusText.setPosition({540,500}); statusText.setFillColor(sf::Color::Red);
}

void OnlineLobbyView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    ipInput.handleEvent(event, mPos);
    portInput.handleEvent(event, mPos);
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left) {
            createBtn.handleEvent(mPos, true);
            joinBtn.handleEvent(mPos, true);
            backBtn.handleEvent(mPos, true);
        }
    }
}

void OnlineLobbyView::update(sf::Vector2f mousePos, sf::Vector2u) {
    ipInput.updateCursor(); portInput.updateCursor();
    createBtn.handleEvent(mousePos, false);
    joinBtn.handleEvent(mousePos, false);
    backBtn.handleEvent(mousePos, false);
}

void OnlineLobbyView::draw(sf::RenderWindow& window) {
    std::string ipLabelStr = I18n::instance().t("server_ip");
    sf::Text ipLabel(globalFont, sf::String::fromUtf8(ipLabelStr.begin(), ipLabelStr.end()), 18);
    ipLabel.setPosition({540,125}); ipLabel.setFillColor(sf::Color::Black); window.draw(ipLabel);

    std::string portLabelStr = I18n::instance().t("port");
    sf::Text portLabel(globalFont, sf::String::fromUtf8(portLabelStr.begin(), portLabelStr.end()), 18);
    portLabel.setPosition({540,195}); portLabel.setFillColor(sf::Color::Black); window.draw(portLabel);

    ipInput.draw(window); portInput.draw(window);
    createBtn.draw(window); joinBtn.draw(window); backBtn.draw(window);
    window.draw(statusText);
}

// ======================== OnlineGameView 实现（双人闯关） ========================

OnlineGameView::OnlineGameView(const sf::Font& font, std::function<void(std::string)> cv,
                               NetworkManager& net, bool host, const std::string& username)
    : View(font), myUsername(username), changeView(cv), network(net), isHost(host),
      attrPanel(font),
      statusText(font, "", 18),
      myNameText(font, "", 20),
      gameResultText(font, "", 30) {
    backBtn = std::make_unique<Button>(font, I18n::instance().t("back_menu"), sf::Vector2f{1150,20}, sf::Vector2f{120,40},
                                       [cv](){ cv("SELECT_LEVEL"); });
    statusText.setPosition({10,550}); statusText.setFillColor(sf::Color::Red);
    myNameText.setPosition({50, 50}); myNameText.setFillColor(sf::Color::Green);
    gameResultText.setPosition({490, 300}); gameResultText.setFillColor(sf::Color::Blue);
    gameResultText.setCharacterSize(40);

    gameView.setSize(sf::Vector2f(800, 600));
    gameView.setCenter({400, 300});

    if (isHost) {
        unsigned short port = network.getPendingPort();
        if (!network.startServer(port)) {
            std::string err = I18n::instance().t("server_start_failed") + std::to_string(port);
            statusText.setString(sf::String::fromUtf8(err.begin(), err.end()));
        } else {
            std::string succ = I18n::instance().t("server_started") + std::to_string(port);
            statusText.setString(sf::String::fromUtf8(succ.begin(), succ.end()));
            connected = true;
        }
        world.addPlayer(0, {400,300});
        myId = 0;
        playerNames[0] = myUsername + " (Host)";
        myNameText.setString(playerNames[0]);

        // 初始化主机玩家
        Player hostPlayer({400, 300});
        hostPlayer.username = myUsername;
        hostPlayer.baseAttributes = generateRandomAttributes();
        hostPlayer.charClass = CharacterClass::Warrior;
        hostPlayer.classConfig = getClassConfig(CharacterClass::Warrior);
        hostPlayer.weaponManager.getList().initDefaults();
        auto w = hostPlayer.weaponManager.getList().findByType(WeaponType::Sword);
        if (w) hostPlayer.equipWeapon(w->name);
        hostPlayer.recalcCombatStats();
        hostPlayer.health = hostPlayer.maxHealth;
        coopPlayers[0] = std::move(hostPlayer);
    } else {
        myId = -1;
        myNameText.setString(myUsername + " (Client)");
        std::string conn = I18n::instance().t("connecting_server");
        statusText.setString(sf::String::fromUtf8(conn.begin(), conn.end()));
    }
    playerPositions[0] = {400, 300};
}

void OnlineGameView::updateLocalInput() {
    currentInput.up    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    currentInput.down  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    currentInput.left  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    currentInput.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
}

void OnlineGameView::sendInputToServer() {
    if (inputSendClock.getElapsedTime().asSeconds() < 0.05f) return;
    inputSendClock.restart();

    if (isHost) {
        world.setPlayerInput(myId, currentInput);
    } else {
        if (!network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerInput) << myId
          << currentInput.up << currentInput.down << currentInput.left << currentInput.right;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::sendAttack() {
    if (attackSendClock.getElapsedTime().asSeconds() < 0.5f) return;
    attackSendClock.restart();

    if (isHost) {
        attackEnemies();
    } else {
        if (!network.getServerAddr().has_value()) return;
        sf::Packet p;
        p << static_cast<uint8_t>(NetMsgType::PlayerAttack) << myId;
        network.send(p, *network.getServerAddr(), network.getServerPort());
    }
}

void OnlineGameView::attackEnemies() {
    if (!coopPlayers.count(myId)) return;
    auto& me = coopPlayers[myId];
    if (!me.canAttack()) return;

    if (me.equippedWeapon && me.equippedWeapon->isRanged) {
        sf::Vector2f dir(1, 0);
        float minDist = std::numeric_limits<float>::max();
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float d = distance(me.position, e.position);
            if (d < minDist) { minDist = d; dir = normalize(e.position - me.position); }
        }
        Projectile proj;
        proj.position = me.position;
        proj.direction = dir;
        proj.speed = 400.f;
        proj.damage = me.calculateDamage();
        proj.maxDistance = me.attackRange;
        proj.traveled = 0.f;
        proj.fromPlayer = true;
        projectiles.push_back(proj);
    } else {
        for (auto& e : enemies) {
            if (!e.isAlive()) continue;
            float dist = distance(me.position, e.position);
            if (dist <= me.attackRange) {
                e.health -= me.calculateDamage();
            }
        }
    }
    me.resetAttackCooldown();
}

void OnlineGameView::updateEnemies(float dt) {
    // 找最近玩家作为目标
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        sf::Vector2f closestPos = e.position;
        float minDist = std::numeric_limits<float>::max();
        for (auto& [id, p] : coopPlayers) {
            float d = distance(e.position, p.position);
            if (d < minDist) { minDist = d; closestPos = p.position; }
        }
        e.setTarget(closestPos);
        e.update(dt);
    }
}

void OnlineGameView::updateProjectiles(float dt) {
    for (auto& proj : projectiles) proj.update(dt);

    for (auto& proj : projectiles) {
        if (proj.fromPlayer) {
            for (auto& e : enemies) {
                if (!e.isAlive()) continue;
                if (distance(proj.position, e.position) < e.radius + 4.f) {
                    e.health -= proj.damage;
                    proj.traveled = proj.maxDistance;
                    break;
                }
            }
        } else {
            for (auto& [id, p] : coopPlayers) {
                if (distance(proj.position, p.position) < p.radius + 4.f) {
                    p.health -= proj.damage;
                    proj.traveled = proj.maxDistance;
                    if (p.health <= 0 && id == myId) {
                        gameOver = true; victory = false;
                        gameResultText.setString("You Lost!");
                    }
                    break;
                }
            }
        }
    }

    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](const Projectile& p){ return p.isExpired(); }), projectiles.end());
}

void OnlineGameView::handleEnemyAttacks(float dt) {
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        for (auto& [id, p] : coopPlayers) {
            if (e.isRanged) {
                if (e.canAttack() && distance(e.position, p.position) <= e.attackRange) {
                    Projectile proj;
                    proj.position = e.position;
                    proj.direction = normalize(p.position - e.position);
                    proj.speed = 250.f;
                    proj.damage = e.attackDamage;
                    proj.maxDistance = e.attackRange;
                    proj.traveled = 0.f;
                    proj.fromPlayer = false;
                    projectiles.push_back(proj);
                    e.resetAttackCooldown();
                }
            } else {
                if (distance(p.position, e.position) < p.radius + e.radius && e.canAttack()) {
                    p.health -= e.attackDamage;
                    e.resetAttackCooldown();
                    if (p.health <= 0 && id == myId) {
                        gameOver = true; victory = false;
                        gameResultText.setString("You Lost!");
                    }
                }
            }
        }
    }
}

void OnlineGameView::broadcastState() {
    auto state = world.getState();
    sf::Packet p;
    p << static_cast<uint8_t>(NetMsgType::GameState);
    p << static_cast<uint32_t>(state.players.size());
    for (auto& ps : state.players) {
        p << ps.position.x << ps.position.y << ps.health << ps.maxHealth;
    }
    network.broadcast(p);
}

void OnlineGameView::processNetwork() {
    sf::Packet packet;
    auto result = network.receive(packet);
    if (!result) return;
    auto [type, sender] = *result;
    auto [ip, port] = sender;

    if (isHost) {
        if (type == NetMsgType::Connect) {
            if (!network.hasClient(ip, port)) {
                int newId = static_cast<int>(network.getClientCount()) + 1;
                network.addClient(ip, port);
                world.addPlayer(newId, sf::Vector2f(400, 300));
                playerNames[newId] = "Player " + std::to_string(newId);
                playerPositions[newId] = {400, 300};

                Player newPlayer({400, 300});
                newPlayer.username = "Player " + std::to_string(newId);
                newPlayer.baseAttributes = generateRandomAttributes();
                newPlayer.charClass = CharacterClass::Warrior;
                newPlayer.classConfig = getClassConfig(CharacterClass::Warrior);
                newPlayer.weaponManager.getList().initDefaults();
                auto w = newPlayer.weaponManager.getList().findByType(WeaponType::Sword);
                if (w) newPlayer.equipWeapon(w->name);
                newPlayer.recalcCombatStats();
                newPlayer.health = newPlayer.maxHealth;
                coopPlayers[newId] = std::move(newPlayer);

                sf::Packet accept;
                accept << static_cast<uint8_t>(NetMsgType::ConnectAccept) << newId;
                accept << static_cast<uint32_t>(playerNames.size());
                for (const auto& [id, name] : playerNames) accept << id << name;
                network.send(accept, ip, port);

                statusText.setString("Player " + std::to_string(newId) + " connected!");
            }
        }
        else if (type == NetMsgType::Disconnect) {
            int id; packet >> id;
            network.removeClient(ip, port);
            world.removePlayer(id);
            playerNames.erase(id);
            playerPositions.erase(id);
            coopPlayers.erase(id);
        }
        else if (type == NetMsgType::PlayerInput) {
            int id; bool u,d,l,r;
            packet >> id >> u >> d >> l >> r;
            world.setPlayerInput(id, {u,d,l,r});
        }
        else if (type == NetMsgType::PlayerAttack) {
            int attackerId; packet >> attackerId;
            // 客户端攻击敌人
            if (coopPlayers.count(attackerId)) {
                auto& attacker = coopPlayers[attackerId];
                if (attacker.canAttack()) {
                    for (auto& e : enemies) {
                        if (!e.isAlive()) continue;
                        if (distance(attacker.position, e.position) <= attacker.attackRange) {
                            e.health -= attacker.calculateDamage();
                        }
                    }
                    attacker.resetAttackCooldown();
                }
            }
        }
    } else {
        if (type == NetMsgType::ConnectAccept) {
            packet >> myId;
            connected = true;
            statusText.setString("Connected! Your ID = " + std::to_string(myId));
            uint32_t count; packet >> count;
            for (uint32_t i = 0; i < count; ++i) {
                int id; std::string name;
                packet >> id >> name;
                playerNames[id] = name;
            }
            playerNames[myId] = myUsername + " (You)";
            myNameText.setString(playerNames[myId]);
        }
        else if (type == NetMsgType::GameState) {
            uint32_t cnt; packet >> cnt;
            GameState state;
            for (uint32_t i=0; i<cnt; ++i) {
                sf::Vector2f pos; int hp, maxHp;
                packet >> pos.x >> pos.y >> hp >> maxHp;
                state.players.push_back({pos, hp, maxHp});
            }
            lastReceivedState = std::move(state);
            int idx = 0;
            for (const auto& ps : lastReceivedState.players) {
                playerPositions[idx] = ps.position;
                ++idx;
            }
            if (lastReceivedState.players.size() > static_cast<uint32_t>(myId)) {
                if (lastReceivedState.players[myId].health <= 0 && !gameOver) {
                    gameOver = true; victory = false;
                    gameResultText.setString("You Lost!");
                }
            }
        }
        else if (type == NetMsgType::Disconnect) {
            int id; packet >> id;
            playerNames.erase(id);
            playerPositions.erase(id);
            statusText.setString("Player " + std::to_string(id) + " disconnected.");
        }
    }
}

void OnlineGameView::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mbp->button == sf::Mouse::Button::Left && backBtn)
            backBtn->handleEvent(mPos, true);
    }
    if (!gameOver) {
        updateLocalInput();
        sendInputToServer();
        if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::Space) sendAttack();
            if (kp->code == sf::Keyboard::Key::Tab) attrPanel.toggle();
        }
    }
}

void OnlineGameView::update(sf::Vector2f mousePos, sf::Vector2u windowSize) {
    if (backBtn) backBtn->handleEvent(mousePos, false);
    float dt = deltaClock.restart().asSeconds();
    if (dt > 0.1f) dt = 0.1f;

    processNetwork();

    if (isHost) {
        world.update(dt);

        // 同步coopPlayers位置
        auto state = world.getState();
        int idx = 0;
        for (const auto& ps : state.players) {
            playerPositions[idx] = ps.position;
            if (coopPlayers.count(idx)) {
                coopPlayers[idx].position = ps.position;
                coopPlayers[idx].health = ps.health;
            }
            ++idx;
        }

        // 地图区域敌人刷新
        if (coopPlayers.count(myId)) {
            auto newEnemies = mapSystem.checkAndSpawn(coopPlayers[myId].position);
            for (auto& spawn : newEnemies) {
                enemies.emplace_back(spawn.type, spawn.position);
            }
        }

        // 敌人AI
        updateEnemies(dt);

        // 敌人攻击
        handleEnemyAttacks(dt);

        // 投射物
        updateProjectiles(dt);

        // 处理击杀
        for (auto& e : enemies) {
            if (!e.isAlive() && e.health <= 0) {
                for (auto& [id, p] : coopPlayers) {
                    p.totalScore += e.score;
                    p.levelData.gainExp(e.score);
                    p.recalcCombatStats();
                }
            }
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e){ return !e.isAlive(); }), enemies.end());

        broadcastState();
    }

    // 摄像机跟随自己
    if (playerPositions.count(myId)) {
        mapSystem.updateCamera(gameView, playerPositions[myId]);
    }

    // 属性面板
    if (coopPlayers.count(myId)) attrPanel.update(coopPlayers[myId]);

    // 其他玩家信息
    otherPlayerTexts.clear();
    int yOffset = 100;
    for (const auto& [id, name] : playerNames) {
        if (id == myId) continue;
        sf::Text text(globalFont, "", 18);
        text.setPosition({50, static_cast<float>(yOffset)});
        text.setFillColor(sf::Color::Black);
        std::string info = name;
        if (lastReceivedState.players.size() > static_cast<uint32_t>(id)) {
            info += " HP: " + std::to_string(lastReceivedState.players[id].health);
        }
        text.setString(info);
        otherPlayerTexts.push_back(text);
        yOffset += 30;
    }
}

void OnlineGameView::draw(sf::RenderWindow& window) {
    window.setView(gameView);

    // 地图背景
    mapSystem.drawBackground(window);

    // 敌人
    for (auto& e : enemies) e.drawWithName(window, &globalFont);

    // 投射物
    for (auto& proj : projectiles) proj.draw(window);

    // 绘制所有玩家
    std::vector<sf::Color> colors = {sf::Color::Green, sf::Color::Blue, sf::Color::Magenta,
                                     sf::Color::Cyan, sf::Color::Yellow, sf::Color(255, 165, 0)};
    int colorIdx = 0;
    for (const auto& [id, pos] : playerPositions) {
        sf::CircleShape shape(18.f);
        if (id == myId) {
            shape.setFillColor(sf::Color::Cyan);
        } else {
            shape.setFillColor(colors[colorIdx % colors.size()]);
            ++colorIdx;
        }
        shape.setPosition(pos - sf::Vector2f(18, 18));
        window.draw(shape);

        // 名字
        if (playerNames.count(id)) {
            std::string name = playerNames[id];
            sf::Text nameText(globalFont, sf::String::fromUtf8(name.begin(), name.end()), 12);
            nameText.setFillColor(sf::Color::White);
            nameText.setPosition(sf::Vector2f(pos.x - nameText.getLocalBounds().size.x / 2, pos.y + 20));
            window.draw(nameText);
        }

        // 血条
        float barW = 40.f, barH = 6.f;
        sf::RectangleShape bg(sf::Vector2f(barW, barH));
        bg.setFillColor(sf::Color::Red);
        bg.setPosition(sf::Vector2f(pos.x - barW/2, pos.y - 24));
        window.draw(bg);
        int hp = 100, maxHp = 100;
        if (lastReceivedState.players.size() > static_cast<uint32_t>(id)) {
            hp = lastReceivedState.players[id].health;
            maxHp = lastReceivedState.players[id].maxHealth;
        }
        float pct = static_cast<float>(hp) / maxHp;
        sf::RectangleShape bar(sf::Vector2f(barW * pct, barH));
        bar.setFillColor(sf::Color::Green);
        bar.setPosition(sf::Vector2f(pos.x - barW/2, pos.y - 24));
        window.draw(bar);
    }

    // 恢复默认视图
    window.setView(window.getDefaultView());

    if (backBtn) backBtn->draw(window);
    window.draw(statusText);
    window.draw(myNameText);
    for (const auto& text : otherPlayerTexts) window.draw(text);
    attrPanel.draw(window);
    if (gameOver) window.draw(gameResultText);
}
