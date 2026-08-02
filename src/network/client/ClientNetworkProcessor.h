#pragma once
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <mgmt/GameStateManager.h>
#include <network/client/ClientContext.h>

class ClientNetworkProcessor {
public:
    ClientNetworkProcessor(ClientContext& ctx);

    // Call once per frame. Processes all pending TCP and UDP messages.
    void processIncoming();

    // Send player input (keyboard state → UDP)
    void sendInput(const std::string& input);

private:
    void handleTcpMessage(sf::Packet& packet);
    void handleUdpMessage(sf::Packet& packet);

    ClientContext& ctx;
};