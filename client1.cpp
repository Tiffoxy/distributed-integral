#include <iostream>
#include<WinSock2.h>//чтобы работать с сокетами
#include<WS2tcpip.h> // для доп.функц.
#include<Windows.h>
#include<string>
#include <thread>
#pragma comment(lib, "ws2_32.lib")  // подключаем библиотеку сокетов
using namespace std;
/* 
* функция для определения доступных ядер процессора
* Резервируем часть ядер для ОС
*/
int getAvailableCores() 
{
    unsigned int cores = thread::hardware_concurrency();//hardware_concurrency возвращает количество ядер,может вовзращать 0 если не определенно
    if (cores == 0)
    {
        cout << "Не удалось определить ядра, использую 2" << endl;
        return 2;
    }
    cout << "Всего ядер в системе: " << cores << endl;
    // если ядер много, резервируем часть для системы
    if (cores >= 16)
    {
        int available = cores - 4;  // резервируем 4 ядра
        cout << "Доступно для вычислений: " << available << " ядер" << endl;
        return available;
    }
    else
    {
        if (cores >= 8)
        {
            int available = cores - 2;  // резервируем 2 ядра
            cout << "Доступно для вычислений: " << available << " ядер" << endl;
            return available;
        }
        else
        {
            if (cores >= 4)
            {
                int available = cores - 1;  // резервируем 1 ядро
                cout << "Доступно для вычислений: " << available << " ядер" << endl;
                return available;
            }
            else
            {
                // Для 1-3 ядер отдаём все
                cout << "Доступно для вычислений: " << cores << " ядер" << endl;
                return cores;
            }
        }
    }
}
/*
* функция для интегрирования
*/
double Func(double x)
{

    if (x <= 0 || abs(x - 1.0) < 1e-10)//иначе будет деление на 0,а это ошибка
    {
        return 0.0;
    }
    return 1.0 / log(x);
}
/*
* функция для вычисление интеграла методом трапеции
*/
double calculateByTrapezoids(double a, double b, double h) {
    double sum = 0;
    double x = a;
    while (x < b) {
        double real_h = min(h, b - x);
        double f1 = Func(x);
        double f2 = Func(x + real_h);
        sum += (f1 + f2) * real_h / 2.0;
        x += real_h;
    }
    return sum;
}
/*
* функция для вычисление интеграла методом простых прямоугольников
*/
double calculateIntegral(double a, double b, double h)
{
    if (h <= 0.0)
    {
        cout << "Шаг должен быть положительный";
        return 0;
    }
    else
    {
        if (a > b)//если границы перепутанны меняем их местами,но такого быть по идее не должно т.к. проверяется еще в коде сервера
        {
            double c = a;
            a = b;
            b = c;
        }
        double S = 0;
        double x = a;        
        while (x < b)
        {
            double real_h = h;//определяем реальную ширину отрезка
            if (b - x < h * 1e-10)
            {
                break; // Выходим из цикла, чтобы не создавать лишний прямоугольник
            }
            //прверяем не выходит ли за границу
            if (x + h > b)
            {
                real_h = b - x;  // т.к. последний отрезок может быть короче нашего шага,и чтоьы не считать излишнюю площадь
            }
            double middleOfSigment;
            //сам метод простых прямоугольников
            middleOfSigment = (x + x + real_h) / 2.0;
            double funcValue = Func(middleOfSigment);
            double rectanglArea = funcValue * real_h;

            x += real_h;  // увеличиваем на real_h
            S += rectanglArea;
        }
        return S;
    }
}
/*
* модульные тесты
*/
void test1() 
{
    // Тест 1: интеграл от 0 до 1 функции x² должен быть 1/3
    // (но у нас функция 1/ln(x), так что тестируем на простой)
    cout << "Тест 1: ";
    double result = calculateIntegral(2, 3, 0.001);
    // Здесь нужно знать ожидаемый результат
    cout << "Результат: " << result << std::endl;
}
void test2() 
{
    // Тест 2: если a > b, функция должна их поменять местами
    cout << "Тест 2: a > b - ";
    double result = calculateIntegral(3, 2, 0.001);
    cout << "Результат: " << result << std::endl;
}
void test3() 
{
    // Тест 3: шаг <= 0 должен обрабатываться
    cout << "Тест 3: нулевой шаг - ";
    double result = calculateIntegral(2, 3, 0);
    cout << "Результат: " << result << " (должен быть 0 или ошибка)" << std::endl;
}

int main()
{
    SetConsoleOutputCP(1251);
    cout << "Запуск модульных тестов" << endl;
    test1();
    test2();
    test3();
    cout << "\nОкончание модульных тестов" << endl;
    cout << "Вы запустили клиента" << endl;
    int numCores = getAvailableCores();//определяем кол-во доступных ядер
    //инициализируем билиотеку Winsock
    WSADATA wsaData;    
    cout << "Обнаруженно ядер процессора = " << numCores;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "\nОшибка инициализации Winsock!" << endl;
        return 1;
    }
    //создание клиентского сокета
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "\nОшибка создания сокета!" << endl;
        WSACleanup();
        return 1;
    }
    //настривааю адрес сервера
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(11507);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        cout << "\nНеверный IP-адрес!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    //подключаемся к серверу
    cout << "\nидет подключение к серверу" << endl;
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        cout << "\nНе удалось подключиться к серверу!" << endl;
        cout << "\nЗапусти сервер первым!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    cout << "Мы подключились^^" << endl;
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cout << "Не получил сообщение от сервера" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    buffer[bytesReceived] = '\0';
    string server_message(buffer);
    if (server_message == "Количество_ядер")
    {
        cout << "Сервер запросил информацию о ядрах" << endl;
        // Отправляем количество ядер
        string cores_response = to_string(numCores);
        if (send(clientSocket, cores_response.c_str(), cores_response.size(), 0) == SOCKET_ERROR)
        {
            cout << "Ошибка отправки информации о ядрах!" << endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
    }
    //получаем задание от сервера
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) 
    {
        cout << "Не получил задание от сервера!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    buffer[bytesReceived] = '\0';
    server_message = string(buffer);
    double a, b, h;
    cout << "\nПолучил задание от сервера:" << endl;
    sscanf_s(buffer, "%lf %lf %lf", &a, &b, &h);
    cout << "a = " << a << ", b = " << b << ", h = " << h << endl;
    cout << "\nВыберите метод (1-прямоугольники, 2-трапеции): ";
    int method;
    cin >> method;
    buffer[bytesReceived] = '\0';
    double result;
    //производим вычисление
    if (method == 1) 
    {
        result = calculateIntegral(a, b, h);
        cout << "Используется метод прямоугольников" << endl;        
    }
    else 
    {
        result = calculateByTrapezoids(a, b, h);
        cout << "Используется метод трапеций" << endl;
    }
    //отправка результатов сервру
    string response = to_string(result);
    cout << "отправили серверу " << result << endl;
    if (send(clientSocket, response.c_str(), response.size(), 0) == SOCKET_ERROR) 
    {
        cout << "Ошибка отправки результата!" << endl;
    }
    else 
    {
        cout << "Результат отправлен серверу!" << endl;
    }

    //очистка
    closesocket(clientSocket);
    WSACleanup();
    cout << "Клиент завершил работу" << endl;

}