//
//  Buffer_str.cpp
//  Test_task
//


#include "Buffer_str.hpp"

// Геттер для получения данных
std::string Buffer_str::get()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->data;
}

// Сеттер для установки данных
void Buffer_str::set(const std::string &newData)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data = newData;
    this->cv.notify_one(); // Извещаем о наличии данных
}

// Функция очистки буфера
void Buffer_str::clear()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data.clear();
}

// Функция для ожидания данных (блокирует поток)
void Buffer_str::waitForNewData()
{
    std::unique_lock<std::mutex> lock(this->mutex);
    this->cv.wait(lock, [this] { return !this->data.empty(); });
}

