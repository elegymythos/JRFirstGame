/**
 * @file AuthView.cpp
 * @brief 登录/注册视图实现
 */

#include "AuthView.hpp"
#include "Database.hpp"
#include "Utils.hpp"
#include "I18n.hpp"

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
