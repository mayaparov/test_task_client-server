//
//  SocketServer.hpp
//  Test_task
//



#ifndef SocketServer_hpp
#define SocketServer_hpp

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Buffer_str.hpp"

// Класс для работы с сокетом
class SocketServer
{
    //Поля с данными для установки соединения
    int port;
    int sockfd, newsockfd = 0;
    sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // Очередь для хранения неотправленных сообщений
    std::queue<int> messageQueue;
    std::mutex messageQueueMutex;
    std::condition_variable messageQueueCV;

    //Поле для обработки ввода и вывода
    Buffer_str data;

    //Поле для корректной индикации подключения
    bool connection_is_down = false;

    //мьютекс для корректого отображения данных в консоли
    std::mutex consoleOutMutex;

  public:
    //Конструктор с инициализацией порта
    SocketServer(int port) : port(port)
    {
    }

    //Установка соединения
    void start();

  private:
    // Обработка ввода
    void processInput();

    //// Функция для отправки сообщений в программу №2
    bool sendMessage(int message);

    // Функция для обработки очереди сообщений в отдельном потоке
    void processMessageQueue();

    //Функция для проверки подключения (и автоматического восстановления)
    bool is_connected();

    // Обработка вывода
    void processOutput();

    //Установка подключения
    bool set_connection();

    // Проверка ввода
    bool isValidInput(const std::string &input);

    // Подсчет суммы элементов
    int calculateSum(const std::string &input);

    //Синхронизированный вывод данных в консоль
    void print_to_console(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(consoleOutMutex);
        std::cout << message;
    }
};

#endif /* SocketServer_hpp */
