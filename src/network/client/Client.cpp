// Client.cpp
#include "Client.h"
#include <network/NetworkCommon.h>
#include <network/NetTypes.h>
#include <entities/Entity.h>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <network/client/ClientContext.h>
#include <mgmt/GameStateManager.h>
#include <entities/animation/AnimationSet.h>
#include <res/SceneResources.h>
#include <res/Cfg.h>
#include <network/client/ClientNetworkProcessor.h>

void run_client(int server_tcp_port) {
    const sf::IpAddress server_ip(127, 0, 0, 1);  //24, 35, 13, 61);  // change to real IP

    // ---------- TCP handshake ----------
    unsigned short my_udp_port;
    int player_id;
    sf::TcpSocket tcp_socket;
    sf::UdpSocket udp_socket = tcp_handshake_client(server_ip, server_tcp_port,
        &my_udp_port, &player_id, tcp_socket);
    if (udp_socket.getLocalPort() == 0) return;

    udp_socket.setBlocking(false);
    tcp_socket.setBlocking(false);

    const unsigned short server_udp_port = 57913;
    bool tcp_ok_received = false;

    printf("[Client %d] Sending UDP 'OK' and waiting for TCP confirmation...\n", player_id);
    while (!tcp_ok_received) {
        const char* ok_msg = "OK";
        udp_socket.send(ok_msg, std::strlen(ok_msg), server_ip, server_udp_port);
        sf::Packet packet;
        if (tcp_socket.receive(packet) == sf::Socket::Status::Done) {
            std::string confirm;
            packet >> confirm;
            if (confirm == "OK") {
                tcp_ok_received = true;
                printf("[Client %d] Received TCP 'OK' – ready.\n", player_id);
                break;
            }
        }
        sf::sleep(sf::milliseconds(100));
    }
    tcp_socket.setBlocking(false);

    Cfg::Initialize();

    // ---- Animation setup ----  good long as the client is running
    AnimationSet playerAnimSet;
    initPlayerAnimations(playerAnimSet);
    std::unordered_map<EntityType, AnimationSet*> entityAnimSets = {
        { EntityType::Player, &playerAnimSet }
    };

    printf("[Client %d] Starting main loop, Animation setup correctly...\n", player_id);

    // ---------- Client context ----------
    ClientContext ctx{ .serverIp = server_ip };
    ctx.udpSocket = &udp_socket;
    ctx.tcpSocket = &tcp_socket;
    ctx.serverIp = server_ip;
    ctx.serverPort = server_udp_port;
    ctx.playerId = player_id;
    ctx.myEntityId = player_id;
    ctx.currentZone = 1;
    ctx.currentLevel = 1;

    // ---------- Window ----------
    sf::RenderWindow window(sf::VideoMode({ 1600, 900 }), "Game Client");
    window.setFramerateLimit(60);

    GameStateManager gsm(ctx);
    ctx.gsm = &gsm;

    gsm.registerState("title", std::make_unique<TitleState>(&window, ctx));
    gsm.registerState("overworld", std::make_unique<OverworldState>(&window, ctx, entityAnimSets));

    gsm.registerState("play", std::make_unique<PlayState>(&window, ctx, entityAnimSets));
    gsm.switchTo("title");   // start on title screen

    ClientNetworkProcessor network(ctx);

    sf::Clock clock;
    while (window.isOpen()) {
        // Process window events
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            else
                gsm.handleEvent(*event);
        }

		network.processIncoming();  // handles both TCP and UDP messages


        // ---- Input sending (centralised) ----
        // We'll read keyboard and send movement input here, regardless of state.
        if (window.hasFocus()) {
            std::string input;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) input += 'L';
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) input += 'R';
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) input += 'U';      // jump
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
                input += '1';      // attack1
				printf("[Client %d] Sending attack1 input\n", player_id);

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) input += '2';      // attack2
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) input += '3';      // attack3

            network.sendInput(input);
            ctx.latestInput = input;
        }

        // Update & draw current state
        gsm.update(clock.restart());
        gsm.draw(window);
    }

}