/**
 * @file main.cpp
 * @brief 程序入口和主循环
 */

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <functional>
#include <iostream>

#ifdef __APPLE__
    #include <CoreFoundation/CoreFoundation.h>
    #include <sys/param.h>
#endif

#include "Database.hpp"
#include "Network.hpp"
#include "I18n.hpp"
#include "ViewBase.hpp"
#include "AuthView.hpp"
#include "MenuView.hpp"
#include "CharacterView.hpp"
#include "GameView.hpp"
#include "RankingsView.hpp"
#include "OnlineView.hpp"

#ifdef EMBEDDED_FONT
    #include "embedded_font.hpp"
#endif

#ifndef EMBEDDED_FONT
    #ifdef __APPLE__
        std::string getResourcePath(const std::string& relativePath) {
            CFURLRef appUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
            char path[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(appUrl, TRUE, (UInt8*)path, PATH_MAX)) {
                CFRelease(appUrl);
                return std::string(path) + "/" + relativePath;
            }
            CFRelease(appUrl);
            return relativePath;
        }
    #else
        std::string getResourcePath(const std::string& relativePath) {
            return relativePath;
        }
    #endif
#endif

int main() {
    // 创建窗口
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "JRFirstGame");
    window.setFramerateLimit(60);

    // 加载字体
    sf::Font font;
#ifdef EMBEDDED_FONT
    if (!font.openFromMemory(embedded_font_data, embedded_font_size)) {
        std::cerr << "Failed to load embedded font!" << std::endl;
        return -1;
    }
#else
    std::string fontPath = getResourcePath("assets/SmileySans-Oblique-2.ttf");
    if (!font.openFromFile(fontPath)) {
        std::cerr << "Failed to load font file! Path: " << fontPath << std::endl;
        return -1;
    }
#endif

    // 初始化数据库和网络
    Database db;
    NetworkManager network;
    std::string currentUsername;
    std::unique_ptr<View> currentView;
    std::string pendingView = "";
    bool welcomeShown = false;

    // 视图切换回调
    std::function<void(std::string)> changeView = [&](std::string name) {
        pendingView = name;
    };

    // 初始视图：主菜单
    changeView("MENU");

    // 记录当前视图名称，用于语言切换时刷新
    std::string currentViewName = "MENU";
    bool needRefresh = false;

    // 主循环
    while (window.isOpen()) {
        // 处理事件
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (currentView) {
                currentView->handleEvent(*event, window);
                if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                    if (kp->code == sf::Keyboard::Key::L) {
                        I18n::instance().toggleLanguage();
                        needRefresh = true;
                    }
                }
                if (currentView->needsRefresh()) {
                    needRefresh = true;
                    currentView->clearRefreshFlag();
                }
            }
        }

        // 语言切换后刷新当前视图
        if (needRefresh && !currentViewName.empty()) {
            changeView(currentViewName);
            needRefresh = false;
        }

        // 切换视图
        if (!pendingView.empty()) {
            currentViewName = pendingView;
            if (pendingView == "EXIT") {
                window.close();
                pendingView = "";
                break;
            }
            if (pendingView == "MENU")
                currentView = std::make_unique<MainMenuView>(font, changeView);
            else if (pendingView == "LOGIN")
                currentView = std::make_unique<LoginView>(font, changeView, currentUsername, db);
            else if (pendingView == "SIGNUP")
                currentView = std::make_unique<SignUpView>(font, changeView, db);
            else if (pendingView == "SELECT_LEVEL")
                currentView = std::make_unique<LevelSelectView>(font, changeView, !welcomeShown, db.hasCharacter(currentUsername));
            else if (pendingView == "CHAR_CREATE")
                currentView = std::make_unique<CharacterCreateView>(font, changeView, currentUsername, db);
            else if (pendingView == "CHAR_SELECT")
                currentView = std::make_unique<CharacterSelectView>(font, changeView, currentUsername, db);
            else if (pendingView == "LEVEL_GAME")
                currentView = std::make_unique<ActualGameView>(font, changeView, currentUsername, db);
            else if (pendingView == "RANKINGS")
                currentView = std::make_unique<RankingsView>(font, changeView, db);
            else if (pendingView == "ONLINE_LOBBY")
                currentView = std::make_unique<OnlineLobbyView>(font, changeView, network);
            else if (pendingView == "ONLINE_GAME_HOST")
                currentView = std::make_unique<OnlineGameView>(font, changeView, network, true, currentUsername, db);
            else if (pendingView == "ONLINE_GAME_CLIENT")
                currentView = std::make_unique<OnlineGameView>(font, changeView, network, false, currentUsername, db);

            pendingView = "";
            welcomeShown = true;
        }

        // 更新当前视图
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
