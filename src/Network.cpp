#include <cstdint>
#include "Network.hpp"

NetworkManager::NetworkManager() : role(None) {}

bool NetworkManager::startServer(unsigned short port) {
    // 先断开现有连接
    disconnect();
    role = Server;
    if (socket.bind(port) != sf::Socket::Status::Done) return false;
    socket.setBlocking(false);
    return true;
}

bool NetworkManager::connectToServer(const sf::IpAddress& ip, unsigned short port) {
    // 先断开现有连接
    disconnect();
    role = Client;
    serverAddr = ip;
    serverPort = port;
    // 客户端需要绑定一个随机端口来接收数据
    if (socket.bind(sf::UdpSocket::AnyPort) != sf::Socket::Status::Done) return false;
    socket.setBlocking(false);
    // 发送连接请求
    sf::Packet p;
    p << static_cast<uint8_t>(NetMsgType::Connect);
    (void)socket.send(p, *serverAddr, serverPort);
    return true;
}

void NetworkManager::disconnect() {
    socket.unbind();
    role = None;
    serverAddr = std::nullopt;
    serverPort = 0;
    clients.clear();
}

void NetworkManager::send(sf::Packet& packet, const sf::IpAddress& addr, unsigned short port) {
    (void)socket.send(packet, addr, port);
}

void NetworkManager::broadcast(sf::Packet& packet) {
    for (auto& [addr, port] : clients) {
        (void)socket.send(packet, addr, port);
    }
}

std::optional<std::pair<NetMsgType, std::pair<sf::IpAddress, unsigned short>>> 
NetworkManager::receive(sf::Packet& outPacket) {
    std::optional<sf::IpAddress> fromIp;
    unsigned short fromPort;
    if (socket.receive(outPacket, fromIp, fromPort) == sf::Socket::Status::Done && fromIp.has_value()) {
        uint8_t type;
        outPacket >> type;
        if (type > static_cast<uint8_t>(NetMsgType::Disconnect)) return std::nullopt;
        return std::make_pair(static_cast<NetMsgType>(type), std::make_pair(*fromIp, fromPort));
    }
    return std::nullopt;
}

void NetworkManager::addClient(const sf::IpAddress& ip, unsigned short port) {
    clients.emplace(ip, port);
}

void NetworkManager::removeClient(const sf::IpAddress& ip, unsigned short port) {
    clients.erase({ip, port});
}

bool NetworkManager::hasClient(const sf::IpAddress& ip, unsigned short port) const {
    return clients.count({ip, port}) > 0;
}