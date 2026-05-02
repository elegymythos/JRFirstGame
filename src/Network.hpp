/**
 * @file Network.hpp
 * @brief UDP 网络通信封装
 *
 * 基于 SFML 的 UdpSocket 实现简单的服务器/客户端网络通信。
 * 用于联机双人闯关模式：
 * - 服务器（房主）：绑定指定端口，接收客户端连接和输入，广播游戏状态
 * - 客户端：绑定随机端口，连接服务器，发送输入，接收游戏状态
 *
 * 通信协议：
 * 每个包的第一个字节是 NetMsgType 枚举值，后续数据依消息类型而定。
 * 所有通信使用非阻塞模式。
 */

#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <set>
#include <optional>
#include <utility>
#include <cstdint>

/**
 * @enum NetMsgType
 * @brief 网络消息类型枚举
 *
 * 每种消息的第一个字节为此枚举值，用于区分消息类型。
 */
enum class NetMsgType : uint8_t {
    None,            ///< 无效消息
    Connect,         ///< 客户端请求连接（携带职业和用户名）
    ConnectAccept,   ///< 服务器接受连接（返回分配的玩家ID）
    PlayerInput,     ///< 客户端发送输入状态（WASD）
    PlayerAttack,    ///< 客户端发送攻击信号
    PlayerRoll,      ///< 客户端发送翻滚信号（含方向）
    GameState,       ///< 服务器广播完整游戏状态（玩家/敌人/投射物/掉落物）
    GameStart,       ///< 房主通知客户端游戏开始
    PauseSync,       ///< 暂停状态同步
    Disconnect       ///< 断开连接
};

/**
 * @struct PlayerInput
 * @brief 玩家输入状态
 *
 * 仅记录四个方向键的按下状态，由客户端每帧发送给服务器。
 */
struct PlayerInput {
    bool up = false, down = false, left = false, right = false;
};

/**
 * @struct GameState
 * @brief 游戏状态快照
 *
 * 简化版游戏状态，仅包含玩家位置和血量。
 * 完整的联机状态同步在 OnlineView 中通过自定义包格式实现。
 */
struct GameState {
    /// 单个玩家的状态
    struct PlayerState {
        sf::Vector2f position;  ///< 位置
        int health = 100;       ///< 血量
        int maxHealth = 100;    ///< 血量上限
    };
    std::vector<PlayerState> players;  ///< 所有玩家状态
};

/**
 * @class NetworkManager
 * @brief UDP 网络通信管理器
 *
 * 封装 SFML UdpSocket，提供服务器/客户端两种角色。
 * 服务器维护客户端列表，支持广播。
 * 客户端记录服务器地址和端口。
 */
class NetworkManager {
public:
    /// 网络角色
    enum Role { None, Server, Client };

    NetworkManager();

    /**
     * @brief 启动服务器
     * @param port 监听端口
     * @return true=绑定成功
     *
     * 先断开现有连接，然后绑定指定端口，设为非阻塞模式。
     */
    bool startServer(unsigned short port);

    /**
     * @brief 连接到服务器
     * @param ip   服务器IP地址
     * @param port 服务器端口
     * @return true=绑定随机端口成功
     *
     * 绑定随机端口后发送 Connect 消息。
     */
    bool connectToServer(const sf::IpAddress& ip, unsigned short port);

    /// 断开连接，重置所有状态
    void disconnect();

    /// 设置待使用的端口（在大厅界面填写，进入游戏时读取）
    void setPendingPort(unsigned short port) { pendingPort = port; }
    /// 获取待使用的端口
    unsigned short getPendingPort() const { return pendingPort; }

    /// 向指定地址发送数据包
    void send(sf::Packet& packet, const sf::IpAddress& addr, unsigned short port);
    /// 向所有客户端广播数据包
    void broadcast(sf::Packet& packet);

    /**
     * @brief 接收一个数据包
     * @param outPacket 输出数据包
     * @return {消息类型, {发送者IP, 发送者端口}}，无数据时返回 nullopt
     */
    std::optional<std::pair<NetMsgType, std::pair<sf::IpAddress, unsigned short>>> receive(sf::Packet& outPacket);

    /// 添加客户端到列表
    void addClient(const sf::IpAddress& ip, unsigned short port);
    /// 从列表移除客户端
    void removeClient(const sf::IpAddress& ip, unsigned short port);
    /// 检查客户端是否已在列表中
    bool hasClient(const sf::IpAddress& ip, unsigned short port) const;

    /// 获取当前角色
    Role getRole() const { return role; }
    /// 获取服务器地址（客户端角色时有效）
    const std::optional<sf::IpAddress>& getServerAddr() const { return serverAddr; }
    /// 获取服务器端口
    unsigned short getServerPort() const { return serverPort; }
    /// 获取客户端数量
    size_t getClientCount() const { return clients.size(); }
    /// 获取所有客户端的引用
    const std::set<std::pair<sf::IpAddress, unsigned short>>& getAllClients() const { return clients; }

private:
    sf::UdpSocket socket;           ///< UDP 套接字
    Role role = None;               ///< 当前角色
    std::optional<sf::IpAddress> serverAddr;  ///< 服务器地址（客户端用）
    unsigned short serverPort = 0;  ///< 服务器端口
    unsigned short pendingPort = 54000;  ///< 待使用端口，默认54000
    std::set<std::pair<sf::IpAddress, unsigned short>> clients;  ///< 客户端列表（IP+端口）
};
