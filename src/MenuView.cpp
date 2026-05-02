/**
 * @file MenuView.cpp
 * @brief 主菜单与模式选择视图实现
 */

#include "MenuView.hpp"
#include "I18n.hpp"

// ======================== MainMenuView 实现 ========================

MainMenuView::MainMenuView(const sf::Font& font, std::function<void(std::string)> changeView) : View(font) {
    buttons.emplace_back(font, I18n::instance().t("login"), sf::Vector2f{540,200}, sf::Vector2f{200,50}, [changeView](){ changeView("LOGIN"); });
    buttons.emplace_back(font, I18n::instance().t("signup"), sf::Vector2f{540,280}, sf::Vector2f{200,50}, [changeView](){ changeView("SIGNUP"); });
    buttons.emplace_back(font, I18n::instance().t("exit"), sf::Vector2f{540,360}, sf::Vector2f{200,50}, [changeView](){ changeView("EXIT"); });
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
                cv("CHAR_SELECT");
            } else {
                cv("CHAR_CREATE");
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
