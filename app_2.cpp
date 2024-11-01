#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Функция для установки подключения
bool connectToServer(int &sockfd)
{
    //Установка параметров подключения
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Локальный адрес
    serv_addr.sin_port = htons(8080);                   // Порт сервера

    while (true)
    { // "Бесконечный" цикл для ожидания соединения
        //Без подключения к программе №1 этой программе всё равно делать нечего
        // Создание нового сокета перед каждой попыткой подключения
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            std::cerr << "Ошибка создания сокета" << std::endl;
            continue; // Повторная попытка подключения
        }

        //Обработка попытки подключения
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            //std::cerr << "Ошибка подключения" << std::endl;
            close(sockfd); // Необходимо закрыть сокет перед повторной попыткой подключения
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue; // Повторная попытка подключения
        }

        // Проверка кода ошибки сокета после подключения
        int error_code = 0;
        socklen_t error_code_size = sizeof(error_code);
        try
        {
            //Попытка получить ошибку
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);

            if (error_code != 0)
                throw std::runtime_error(std::strerror(error_code));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка getsockopt: " << e.what() << std::endl;
            close(sockfd); // Необходимо закрыть сокет перед повторной попыткой подключения
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue; // Повторная попытка подключения
        }

        return true; // Соединение успешно установлено
    }
}

int main()
{
    int sockfd = 0; // Инициализируем sockfd как 0

    // Установка подключения
    if (!connectToServer(sockfd))
    {
        std::cerr << "Ошибка: Не удалось установить подключение к серверу." << std::endl;
        return 1;
    }

    int data;

    while (true)
    {
        // Прием данных от сервера
        if (recv(sockfd, &data, sizeof(data), 0) > 0)
        {
            std::cout << "Полученные данные: " << data << std::endl;

            // Проверка полученного значения
            if (data > 99 && data % 32 == 0)
            {
                std::cout << "Данные верны!" << std::endl;
            }
            else
            {
                std::cerr << "Ошибка данных!" << std::endl;
            }

            // Отправка подтверждения
            int confirmation = 1;
            send(sockfd, &confirmation, sizeof(confirmation), 0);
        }
        else
        {
            std::cerr << "Ошибка приема данных" << std::endl;
            // Закрываем сокет
            close(sockfd);

            // Повторная попытка установки подключения
            if (!connectToServer(sockfd))
            {
                std::cerr << "Ошибка: Не удалось установить подключение к серверу." << std::endl;
                return 1;
            }
        }
    }

    // Закрытие сокета
    close(sockfd);
    return 0;
}
