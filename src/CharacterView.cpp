/**
 * @file CharacterView.cpp
 * @brief 角色创建与选择视图实现
 */

#include "CharacterView.hpp"
#include "Database.hpp"
#include "I18n.hpp"
#include <set>
#include <algorithm>
#include <sstream>

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
        auto existing = db.loadAllCharacters(username_);
        std::set<int> usedSlots;
        for (auto& c : existing) usedSlots.insert(c.slot);
        for (int s = 0; s < 3; ++s) {
            if (usedSlots.find(s) == usedSlots.end()) { data.slot = s; break; }
        }
        if (data.slot < 0) {
            std::string err = I18n::instance().t("slots_full");
            statusText_.setString(sf::String::fromUtf8(err.begin(), err.end()));
            return;
        }
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
    drawLangButton(window);
}

// ======================== CharacterSelectView 实现 ========================

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
                    db.saveSelectedSlot(username_, s);
                    changeView("LEVEL_GAME");
                }));
            deleteBtns_.push_back(std::make_unique<Button>(globalFont, I18n::instance().t("delete_char"),
                sf::Vector2f{840, y}, sf::Vector2f{80, 35},
                [this, s](){ db.deleteCharacter(username_, s); needsRefresh_ = true; }));
        } else {
            std::stringstream ss;
            ss << I18n::instance().t("char_slot") << " " << s << ": " << I18n::instance().t("empty_slot");
            std::string slotStr = ss.str();
            slotTexts_.emplace_back(globalFont, sf::String::fromUtf8(slotStr.begin(), slotStr.end()), 20);
            slotTexts_.back().setPosition({390, y});
            slotTexts_.back().setFillColor(sf::Color(128, 128, 128));

            createBtns_.push_back(std::make_unique<Button>(globalFont, I18n::instance().t("new_char"),
                sf::Vector2f{740, y}, sf::Vector2f{100, 35},
                [this, s](){ changeView("CHAR_CREATE"); }));
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
    drawLangButton(window);
}
