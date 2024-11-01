//
//  Buffer_str.hpp
//  Test_task
//

#ifndef Buffer_str_hpp
#define Buffer_str_hpp

#include <mutex>
#include <stdio.h>

// Определение класса для хранения данных
class Buffer_str
{
  private:
    std::string data;           // Данные буфера
    std::mutex mutex;           // Мьютекс для синхронизации доступа
    std::condition_variable cv; // Условие для ожидания данных

  public:
    // Геттер для получения данных
    std::string get();

    // Сеттер для установки данных
    void set(const std::string &newData);

    //Геттер для получения мьютекса
    std::mutex *GetMutex()
    {
        return &this->mutex;
    }

    // Функция очистки буфера
    void clear();

    // Функция для ожидания данных (блокирует поток)
    void waitForNewData();
};

#endif /* Buffer_str_hpp */
