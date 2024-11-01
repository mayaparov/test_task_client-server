//
//  main.cpp
//  Test_task
//

#include "SocketServer.hpp"

int main()
{
    SocketServer server(8080); // Запуск сервера на порту 8080
    server.start();
    return 0;
}
//