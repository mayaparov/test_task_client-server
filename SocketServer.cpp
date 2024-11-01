//
//  SocketServer.cpp
//  Test_task
//


#include "SocketServer.hpp"
//Методы для отправки сообщения

//Установка соединения
void SocketServer::start()
{
    // Создание сокета
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd == -1)
    {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return;
    }
    const int opt_val = 1;
    socklen_t opt_val_size = sizeof(opt_val);
    setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, opt_val_size);

    // Заполнение структуры адреса
    memset(&this->serv_addr, 0, sizeof(this->serv_addr));
    this->serv_addr.sin_family = AF_INET;
    this->serv_addr.sin_addr.s_addr = INADDR_ANY;
    this->serv_addr.sin_port = htons(port);

    // Привязка сокета к адресу
    if (bind(this->sockfd, (struct sockaddr *)&this->serv_addr, sizeof(this->serv_addr)) == -1)
    {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        return;
    }

    // Прослушивание сокета
    if (listen(this->sockfd, 5) == -1)
    {
        std::cerr << "Ошибка прослушивания сокета" << std::endl;
        return;
    }

    // Запуск потоков
    std::thread thread1(&SocketServer::processMessageQueue, this);
    std::thread thread2(&SocketServer::processOutput, this);
    std::thread thread3(&SocketServer::processInput, this);
    
    // Ожидание завершения потоков
    thread1.join();
    thread2.join();
    thread3.join();
}

// Функция для отправки сообщений в программу №2
bool SocketServer::sendMessage(int message)
{
    if (this->is_connected())
    {
        try
        {
            // Отправляем сообщение
            auto temp = send(this->newsockfd, &message, sizeof(message), 0);
            if (temp <= 0 || errno == EPIPE)
                throw std::runtime_error("отсутствие соединения с программой №2");

            // Ожидание подтверждения от программы №2
            int confirmation = 0;
            struct timeval timeout
            {
                3, 0
            }; // Таймаут 3 секунды
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(this->newsockfd, &readfds);

            int result = select(this->newsockfd + 1, &readfds, nullptr, nullptr, &timeout);
            if (result > 0 && FD_ISSET(this->newsockfd, &readfds))
            {
                // Получаем подтверждение
                recv(this->newsockfd, &confirmation, sizeof(confirmation), 0);
                
                return true; // Сообщение успешно отправлено и подтверждено
            }
            else
            {
                throw std::runtime_error("Подтверждение не получено!");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка отправки сообщения: " << e.what() << std::endl;
            this->newsockfd = 0;
            return false;
        }
    }
    else
    {
        return false; // На всякий случай, если сообщение не отправится - программа попадёт сюда
    }
}

// Функция для обработки очереди сообщений в отдельном потоке
void SocketServer::processMessageQueue()
{
    //Переменная для сохранения последнего отправленного сообщения
    int message_backup = -1;
    bool last_msg_wasnt_sent = false;

    //Установка соединения
    this->is_connected();
    while (true)
    {
        std::unique_lock<std::mutex> lock(this->messageQueueMutex);
        //поток нужно остановить только если очередь пуста и последне сообщение было отправлено
        if (this->messageQueue.empty() && !last_msg_wasnt_sent)
            this->messageQueueCV.wait(lock, [this]() { return !this->messageQueue.empty(); }); // Ждем, пока не появится сообщение

        if (!last_msg_wasnt_sent)
        {
            // Извлекаем сообщение из начала очереди
            int message = this->messageQueue.front();

            // Отправляем сообщение в программу №2
            if (this->sendMessage(message))
            {
                // std::lock_guard<std::mutex> lock(this->messageQueueMutex);

                //на случай если отправка неудачна сохраняем последнее сообщение
                message_backup = message;
                this->messageQueue.pop(); //В случае удачной отправки удаляем сообщение из очереди
                last_msg_wasnt_sent = false;
            }
            else
            {
                if (message_backup != -1)
                    last_msg_wasnt_sent = true;
            }
        } //отправляем сообщение из бэкапа
        else if (message_backup != -1)
        {
            if (this->sendMessage(message_backup))
            {
                // std::lock_guard<std::mutex> lock(this->messageQueueMutex);

                message_backup = -1; //В случае удачной отправки удаляем сообщение из бэкапа
                last_msg_wasnt_sent = false;
            }
        }
    }
}

// Проверка соединения
bool SocketServer::is_connected()
{

    if (this->newsockfd > 0)
    {
        int error_code;
        socklen_t error_code_size = sizeof(error_code);

        //пытаемся получить ошибку
        try
        {
            getsockopt(this->newsockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);

            if (error_code != 0)
                throw std::runtime_error(std::strerror(error_code));
        }
        catch (const std::exception &e)
        {
            // Ловим исключение и выводим сообщение об ошибке в std::cerr
            std::cerr << "Ошибка getsockopt: " << e.what() << std::endl;

            //Если соединение не установлено, то закрываем this->newsockfd
            this->newsockfd = 0;
            return this->is_connected(); //вызываем для получения сообщения об ошибке и запуске попыток установить соединение

        } //Если ошибки не возникло - подключение есть


        this->connection_is_down = false;
        return true;
    }
    else
    {
        if (!this->connection_is_down)
        {
            this->connection_is_down = true;
            //будет создан только один поток и выведено только одно сообщение
            std::thread connectionThread([this]() {
                while (true)
                    this->set_connection();
            });
            connectionThread.detach();
        }

        return false;
        ;
    }
    return false;
}

//Установка подключения
bool SocketServer::set_connection()
{
    this->clilen = sizeof(this->cli_addr);
    this->newsockfd = accept(this->sockfd, (struct sockaddr *)&this->cli_addr, (socklen_t *)&this->clilen);
    return this->is_connected();
}

// Подсчет суммы элементов
int SocketServer::calculateSum(const std::string &input)
{
    int sum = 0;
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (std::isdigit(input[i]))
        {
            sum += std::stoi(input.substr(i, 1));
        }
    }
    return sum;
}

// Обработка вывода
void SocketServer::processOutput()
{
    while (true)
    {

        //Блокирование потока
        this->data.waitForNewData();

        //Получение сообщения
        std::string message = this->data.get();

        // Очистка буфера
        this->data.clear();

        //Страховка на случай, если сообщение пустое
        if (message.empty())
            continue;

        // Подсчет суммы элементов
        int temp = this->calculateSum(message);

        this->print_to_console("Полученная строка: " + message + "\nСумма чисел: " + std::to_string(temp) + "\n");

        // Отправка данных в очередь сообщений
        // std::lock_guard<std::mutex> lockQueue(this->messageQueueMutex); // Используем отдельный мьютекс для очереди
        this->messageQueue.push(temp);

        // Разбудим поток обработки очереди, если она была пустой
        this->messageQueueCV.notify_one();
    }
}

//Методы для обработки ввода

// Проверка ввода
bool SocketServer::isValidInput(const std::string &input)
{
    if (input.size() > 64 || input.size() == 0)
    {
        return false;
    }
    for (char c : input)
    {
        if (!std::isdigit(c))
        {
            return false;
        }
    }
    return true;
}

//Обработка ввода
void SocketServer::processInput()
{

    while (true)
    {
        std::string input;
        //Задержка для корректного вывода в консоль
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        print_to_console("Введите строку: \n");
        std::getline(std::cin, input);
        
        if (input.empty())
            continue;

        // Проверка ввода
        if (isValidInput(input))
        {
            // Сортировка и замена четных элементов
            std::sort(input.begin(), input.end(), std::greater<char>());
            for (size_t i = 0; i < input.size(); ++i)
            {
                if (std::stoi(input.substr(i, 1)) % 2 == 0)
                {
                    input.replace(i, 1, "KB");
                    i++;
                }
            }

            // Синхронизация с потоком вывода
            this->data.set(input);
        }
        else
        {
            std::cerr << "Некорректный ввод!" << std::endl;
        }
    }
}
