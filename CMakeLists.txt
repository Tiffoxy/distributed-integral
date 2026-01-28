# Минимальная версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(MyIntegralCalculator)

# Используем C++17
set(CMAKE_CXX_STANDARD 17)

# Для Windows
if(WIN32)
    set(EXTRA_LIBS ws2_32)
endif()

# Создаем программу server из файла server.cpp
add_executable(server server.cpp)
target_link_libraries(server ${EXTRA_LIBS})

# Создаем программу client из файла client1.cpp  
add_executable(client client.cpp)
target_link_libraries(client ${EXTRA_LIBS})