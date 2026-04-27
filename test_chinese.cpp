#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>

int main() {
    // Create window
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Chinese Text Test");
    window.setFramerateLimit(60);

    // Load font from file
    sf::Font font;
    if (!font.openFromFile("assets/SmileySans-Oblique-2.ttf")) {
        std::cerr << "Failed to load font from file!" << std::endl;
        return -1;
    }
    std::cout << "Font loaded from file successfully!" << std::endl;

    // Create text with Chinese characters
    sf::Text text1(font, "测试中文显示", 40);
    text1.setPosition({100, 100});
    text1.setFillColor(sf::Color::Black);

    sf::Text text2(font, "登录 注册 用户名", 30);
    text2.setPosition({100, 200});
    text2.setFillColor(sf::Color::Blue);

    sf::Text text3(font, "你好世界 Hello World", 35);
    text3.setPosition({100, 300});
    text3.setFillColor(sf::Color::Red);

    // Test UTF-8 string
    std::string utf8_str = "UTF-8字符串: 得意黑";
    sf::Text text4(font, utf8_str, 30);
    text4.setPosition({100, 400});
    text4.setFillColor(sf::Color::Green);

    std::cout << "Text created. Check if Chinese characters are displayed correctly." << std::endl;
    std::cout << "Text1 bounds: " << text1.getLocalBounds().size.x << " x " << text1.getLocalBounds().size.y << std::endl;
    std::cout << "Text2 bounds: " << text2.getLocalBounds().size.x << " x " << text2.getLocalBounds().size.y << std::endl;

    // Main loop
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear(sf::Color::White);
        window.draw(text1);
        window.draw(text2);
        window.draw(text3);
        window.draw(text4);
        window.display();
    }

    return 0;
}
