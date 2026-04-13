#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <set>
#include <optional>
#include <utility>

/**
 * @enum NetMsgType
 * @brief 网络消息类型枚举
 */
enum class NetMsgType : uint8_t {
    None,
    Connect,
    ConnectAccept,
    PlayerInput,
    PlayerAttack,
    GameState,
    Disconnect
};

/**
 * @struct PlayerInput
 * @brief 玩家输入状态
 */
struct PlayerInput {
    bool up = false, down = false, left = false, right = false;
};

/**
 * @struct GameState
 * @brief 游戏状态快照，用于网络同步
 */
struct GameState {
    struct PlayerState {
        sf::Vector2f position;
        int health = 100;
        int maxHealth = 100;
    };
    std::vector<PlayerState> players;
};

/**
 * @class NetworkManager
 * @brief 封装 UDP 网络通信，支持服务器/客户端模式
 */
class NetworkManager {
public:
    enum Role { None, Server, Client };

    NetworkManager();

    bool startServer(unsigned short port);
    bool connectToServer(const sf::IpAddress& ip, unsigned short port);
    void send(sf::Packet& packet, const sf::IpAddress& addr, unsigned short port);
    void broadcast(sf::Packet& packet);
    std::optional<std::pair<NetMsgType, std::pair<sf::IpAddress, unsigned short>>> receive(sf::Packet& outPacket);
    
    void addClient(const sf::IpAddress& ip, unsigned short port);
    void removeClient(const sf::IpAddress& ip, unsigned short port);
    Role getRole() const { return role; }
    const std::optional<sf::IpAddress>& getServerAddr() const { return serverAddr; }
    unsigned short getServerPort() const { return serverPort; }
    size_t getClientCount() const { return clients.size(); }

private:
    sf::UdpSocket socket;
    Role role = None;
    std::optional<sf::IpAddress> serverAddr;
    unsigned short serverPort = 0;
    std::set<std::pair<sf::IpAddress, unsigned short>> clients;
};