#include "ClientNetworkProcessor.h"

ClientNetworkProcessor::ClientNetworkProcessor(ClientContext& context)
    : ctx(context) {}

void ClientNetworkProcessor::processIncoming() {
    // --- TCP ---
    sf::Packet tcpPacket;
    while (ctx.tcpSocket->receive(tcpPacket) == sf::Socket::Status::Done) {
        handleTcpMessage(tcpPacket);
    }

    // --- UDP ---
    sf::Packet udpPacket;
    std::optional<sf::IpAddress> sender;
    unsigned short senderPort;
    while (ctx.udpSocket->receive(udpPacket, sender, senderPort)
        == sf::Socket::Status::Done) {
        handleUdpMessage(udpPacket);
    }
}

void ClientNetworkProcessor::handleTcpMessage(sf::Packet& packet) {
    NetMsgType type;
    packet >> type;

    switch (type) {
    case NetMsgType::AssignPlayerEntity: {
        AssignPlayerMessage msg;
        packet >> msg;
        ctx.myEntityId = msg.entityId;
        printf("[Client] Assigned player entity %u\n", msg.entityId);
        break;
    }
    case NetMsgType::LoadZone: {
        LoadZoneMessage msg;
        packet >> msg;
        ctx.currentZone = msg.zoneNumber;
        ctx.currentLevel = -1;
        ctx.gsm->switchTo("overworld");
        break;
    }
    case NetMsgType::LoadLevel: {
        LoadLevelMessage msg;
        packet >> msg;
        ctx.currentLevel = msg.levelNumber;
        ctx.currentZone = -1;
        ctx.gsm->switchTo("play");
        break;
    }
    case NetMsgType::SpawnEntity: {
        SpawnMessage msg;
        packet >> msg;
        ctx.pendingSpawns.push_back(msg);
        break;
    }
    case NetMsgType::DestroyEntity: {
        DestroyMessage msg;
        packet >> msg;
        ctx.pendingDestroys.push_back(msg);
        break;
    }
    }
}

void ClientNetworkProcessor::handleUdpMessage(sf::Packet& packet) {
    NetMsgType type;
    packet >> type;

    if (type == NetMsgType::FrameSnapshot) {
        ctx.latestSnapshot = std::move(packet);
        ctx.hasSnapshot = true;
    }
}

void ClientNetworkProcessor::sendInput(const std::string& input) {
    if (input.empty()) return;
    ctx.udpSocket->send(input.c_str(), input.size(),
        ctx.serverIp, ctx.serverPort);
}