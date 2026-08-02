#include <network/client/Client.h>
#include <network/server/Server.h>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "client") == 0)
        run_client(13579);
    else
        run_server();
    return 0;
}

// structure of program

//main()
//    Server: run_server()
//        tcp_handshake()          // hidden detail
//        udp_game_server()
//            PlayerSlot slots[2]  // encapsulated
//            Level level          // encapsulated
//            
//            while running:
//                accept_late_joins()     // abstracted
//                receive_player_input()  // abstracted
//                
//                if tick_ready:
//                    tick_game_logic()       // moves entities, runs dead-zone camera
//                    for each player:
//                        manage_entity_visibility()  // spawn/despawn AoI
//                        build_and_send_snapshot()   // quantize + UDP
//    
//    Client: run_client()
//        tcp_handshake()
//        GameStateManager gsm
//        while window.open:
//            process_network_messages()   // TCP + UDP → ClientContext queues
//            send_player_input()          // keyboard → UDP
//            gsm.update(dt)
//            gsm.draw(window)