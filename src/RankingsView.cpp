/**
 * @file RankingsView.cpp
 * @brief 排行榜视图实现
 */

#include "RankingsView.hpp"
#include "I18n.hpp"
#include <sstream>

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
