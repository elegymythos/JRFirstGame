/**
 * @file main.cpp
 * @brief 程序入口和主循环
 */

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <functional>
#include <iostream>

#include "Database.hpp"
#include "Network.hpp"
#include "UI.hpp"

#ifdef EMBEDDED_FONT
    #include "embedded_font.hpp"
#endif

int main() {
    // 创建窗口
    sf::RenderWindow window(sf::VideoMode({800, 600}), "JRFirstGame");
    window.setFramerateLimit(60);

    // 加载字体
    sf::Font font;
#ifdef EMBEDDED_FONT
    // 嵌入字体模式：直接从内存加载，无需任何文件系统 API
    if (!font.openFromMemory(embedded_font_data, embedded_font_size)) {
        std::cerr << "Failed to load embedded font!" << std::endl;
        return -1;
    }
#else
    // 外部字体文件模式：需要处理跨平台路径（仅在未嵌入字体时编译）
    #ifdef __APPLE__
        #include <CoreFoundation/CoreFoundation.h>
        #include <sys/param.h>   // for PATH_MAX
        std::string getResourcePath(const std::string& relativePath) {
            CFURLRef appUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
            char path[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(appUrl, TRUE, (UInt8*)path, PATH_MAX)) {
                CFRelease(appUrl);
                return std::string(path) + "/" + relativePath;
            }
            CFRelease(appUrl);
            return relativePath; // fallback
        }
    #else
        std::string getResourcePath(const std::string& relativePath) {
            return relativePath;
        }
    #endif

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

    // 主循环
    while (window.isOpen()) {
        // 处理事件
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (currentView)
                currentView->handleEvent(*event, window);
        }

        // 切换视图
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