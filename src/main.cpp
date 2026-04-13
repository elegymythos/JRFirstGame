#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include "Database.hpp"
#include "Network.hpp"
#include "UI.hpp"
#ifdef EMBEDDED_FONT
    #include "embedded_font.hpp"
#endif
int main() {
    sf::RenderWindow window(sf::VideoMode({800,600}), "SFML 3.0 RPG Game System");
    window.setFramerateLimit(60);

    sf::Font font;
    #ifdef EMBEDDED_FONT
        if (!font.openFromMemory(embedded_font_data, embedded_font_size)) {
            std::cerr << "Failed to load embedded font!" << std::endl;
            return -1;
        }
    #else
        if (!font.openFromFile("assets/SmileySans-Oblique-2.ttf")) {
            std::cerr << "Failed to load font file!" << std::endl;
            return -1;
        }
    #endif

    Database db;
    NetworkManager network;
    std::string currentUsername;
    std::unique_ptr<View> currentView;
    std::string pendingView = "";
    bool welcomeShown = false;

    std::function<void(std::string)> changeView = [&](std::string name) {
        pendingView = name;
    };

    changeView("MENU");

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (currentView)
                currentView->handleEvent(*event, window);
        }

        if (!pendingView.empty()) {
            if (pendingView == "MENU")
                currentView = std::make_unique<MainMenuView>(font, changeView);
            else if (pendingView == "LOGIN")
                currentView = std::make_unique<LoginView>(font, changeView, currentUsername, db);
            else if (pendingView == "SIGNUP")
                currentView = std::make_unique<SignUpView>(font, changeView, db);
            else if (pendingView == "SELECT_LEVEL")
                currentView = std::make_unique<LevelSelectView>(font, changeView, !welcomeShown);
            else if (pendingView == "LEVEL_GAME")
                currentView = std::make_unique<ActualGameView>(font, changeView, currentUsername, db);
            else if (pendingView == "RANKINGS")
                currentView = std::make_unique<RankingsView>(font, changeView, db);
            else if (pendingView == "ONLINE_LOBBY")
                currentView = std::make_unique<OnlineLobbyView>(font, changeView, network);
            else if (pendingView == "ONLINE_GAME_HOST")
                currentView = std::make_unique<OnlineGameView>(font, changeView, network, true, currentUsername);
            else if (pendingView == "ONLINE_GAME_CLIENT")
                currentView = std::make_unique<OnlineGameView>(font, changeView, network, false, currentUsername);

            pendingView = "";
            welcomeShown = true;
        }

        sf::Vector2u windowSize = window.getSize();
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (currentView) {
            currentView->update(mousePos, windowSize);
            window.clear(sf::Color(240, 240, 240));
            currentView->draw(window);
            window.display();
        }
    }
    return 0;
}