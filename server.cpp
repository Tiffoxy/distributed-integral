#include <iostream>
#include<WinSock2.h>//чтобы работать с сокетами
#include<WS2tcpip.h> // для доп.функц.
#include<Windows.h>
#include<string>
#include <vector>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <iomanip>// для setprecision
#pragma comment(lib, "ws2_32.lib")  // подключаем библиотеку сокетов
using namespace std;
int main()
{
    SetConsoleOutputCP(1251);
    cout << fixed << setprecision(10);// Для точного вывода чисел
    ofstream logfile("log.txt");//создаем файл для логов
    cout << "Сервер подключен" << endl;
    logfile << "Сервер запущен" << endl;//пишем в файл чтобы знать когда запустили сервер
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "Ошибка инициализации Winsock!" << endl;
        return 1;
    }
    //получаем параметры задачи
    double a, b, h;
    cout << "\nВведите нижний предел для всей задачи: ";
    cin >> a;
    logfile << "Введеный нижний предел = " << a << endl;
    cout << "Введите верхний предел для всей задачи: ";
    cin >> b;
    logfile << "Введеный верхний предел = " << b << endl;
    cout << "Введите шаг интегрирования: ";
    cin >> h;
    logfile << "Введеный шаг интегрирования = " << h << endl;
    if (a > b)//если один предел больше другого
    {
        swap(a, b);
    }
    //определяем количество клиентов
    int numberOfClient;
    cout << "Сколько клиентов подключится для решения задания? ";
    cin >> numberOfClient;
    //создаем серверный сокет
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);//ипользуем протокол tcp чтобы точно пришли все данные
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Ошибка создания сокета!" << endl;
        WSACleanup();
        return 1;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;//чтобы слущать на всх портах
    serverAddr.sin_port = htons(11507);
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "Ошибка bind,попробуйте изменить порт" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    listen(serverSocket, numberOfClient);
    //ждем-с пока подключится клиент       
    cout << "\nЖдем покдлючение клиента" << endl;
    vector<double> results(numberOfClient, 0.0);  //динамический массив(вектор) на количество клиентов, все 0.0
    vector<thread> threads;
    vector<SOCKET> clientSockets;
    vector<int> coresPerClient(numberOfClient, 0);  // ядра каждого клиента
    //сначала запрашиваем количество доступных ядер
    for (int i = 0; i < numberOfClient; i++)
    {

        cout << "\nКлиент " << (i + 1) << " из " << numberOfClient << " Жду клиента..." << endl;
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET)
        {
            cout << "Ошибка подключения!" << endl;
            logfile << "Ошибка подключения клиента!" << endl;
            continue;
        }
        clientSockets.push_back(clientSocket);
        cout << "Запрашиваю количество ядер у клиента " << (i + 1) << "..." << endl;
        logfile << "Клиент " << (i + 1) << " подключился" << endl;
        string coresRequest = "Количество_ядер";
        send(clientSocket, coresRequest.c_str(), coresRequest.size(), 0);
        char coreBuffer[1024];
        int coreBytes = recv(clientSocket, coreBuffer, sizeof(coreBuffer), 0);
        if (coreBytes > 0)
        {
            coreBuffer[coreBytes] = '\0';
            coresPerClient[i] = atoi(coreBuffer);
            cout << "Клиент " << (i + 1) << " имеет " << coresPerClient[i] << " ядер" << endl;
            logfile << "Клиент " << (i + 1) << " имеет " << coresPerClient[i] << " ядер" << endl;
        }
        else
        {
            coresPerClient[i] = 1;//если не получим данные то по умолчанию возьмем одно
            cout << "Клиент " << (i + 1) << " имеет " << coresPerClient[i] << " ядер" << endl;

        }
    }
    int theTotalAvailableCores = 0;
    for (int cores : coresPerClient)
    {
        theTotalAvailableCores += cores;
    }
    double totalLength = b - a;
    vector<double> segmentStarts(numberOfClient);
    vector<double> segmentEnds(numberOfClient);
    double currentPosition = a;
    for (int i = 0; i < numberOfClient; i++)
    {
        double clientShare = (coresPerClient[i] / (double)theTotalAvailableCores * totalLength);//так мы находим долю этого клиента от общей длины
        segmentStarts[i] = currentPosition;
        segmentEnds[i] = currentPosition + clientShare;
        if (i == numberOfClient - 1)
        {
            segmentEnds[i] = b;
        }
        currentPosition = segmentEnds[i];

    }
    //начинаем работу с клиентами
    for (int i = 0; i < numberOfClient; i++)
    {
        SOCKET clientSocket = clientSockets[i];
        // Определяем задание для клиента
        double start = segmentStarts[i];
        double end = segmentEnds[i];
        threads.push_back(thread([clientSocket, start, end, h, &results, i]()
            {
                string task = to_string(start) + " " + to_string(end) + " " + to_string(h);
                send(clientSocket, task.c_str(), task.size(), 0);
                char buffer[1024];
                int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytes > 0)
                {
                    buffer[bytes] = '\0';
                    results[i] = atof(buffer);
                }
                closesocket(clientSocket);
            }));

    }
    cout << "\nВсе задания распределены. Жду вычислений..." << endl;
    // Ждём завершения всех потоков
    for (auto& t : threads)
    {
        t.join();
    }
    // Суммируем результаты
    double theEndResult = 0;
    for (int i = 0; i < numberOfClient; i++)
    {
        theEndResult += results[i];
    }
    logfile << "Итоговый результат = " << theEndResult << endl;
    //подчищаем все
    logfile.close();
    closesocket(serverSocket);
    WSACleanup();
    // Выводим итог
    cout << "Сервер завершил работу(наконец-то)" << endl;
    cout << "подводим итоги:" << endl;
    cout << "площадь занимамая интегралом = " << theEndResult;
    cout << "\nбыло задействовано ядер:" << theTotalAvailableCores;
    cout << "\nПодключилось клиентов:" << numberOfClient;
}