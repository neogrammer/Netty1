#include "Server.h"
#include "ServerGameLoop.h"
#include <network/NetworkCommon.h>
#include <SFML/Network.hpp>
#include <cstdio>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

// Global pointer for signal handler
static ServerGameLoop* g_gameLoop = nullptr;

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        if (g_gameLoop) g_gameLoop->shutdown();
        return TRUE;
    }
    return FALSE;
}
#else
void signalHandler(int /*signal*/) {
    if (g_gameLoop) g_gameLoop->shutdown();
}
#endif

void run_server() {
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    signal(SIGINT, signalHandler);
#endif

    sf::TcpListener listener;
    if (listener.listen(13579) != sf::Socket::Status::Done) {
        fprintf(stderr, "[Server] TCP listener failed on port 13579\n");
        return;
    }
    printf("[Server] TCP handshake on port 13579\n");

    sf::TcpSocket tcpSockets[2];
    unsigned short clientPorts[2];
    int playerCount = 0;

    if (!tcp_accept_player(listener, tcpSockets[0], clientPorts[0], 0)) {
        fprintf(stderr, "[Server] First player failed to connect.\n");
        return;
    }
    playerCount = 1;
    listener.setBlocking(false);
    tcpSockets[0].setBlocking(false);

    ServerGameLoop gameLoop(listener, tcpSockets, clientPorts, playerCount);
    g_gameLoop = &gameLoop;
    gameLoop.run();
}