#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <conio.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 512
#define SERVER_PORT "27015"

SOCKET clientSock = INVALID_SOCKET;

const int maxWidth = 80;
const int maxHeight = 25;

int cursorX = 5;
int cursorY = 5;

void DisableCursor() {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 10;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

void DisplaySmiley() {
    system("cls");
    COORD pos = { cursorX, cursorY };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    cout << ":)";
}

DWORD WINAPI CommandSender(void* data) {
    char command[2] = " ";
    DisplaySmiley();
    while (true) {
        if (_kbhit()) {
            int input = _getch();
            if (input == 224 || input == 0) input = _getch();
            switch (input) {
                case 72:
                    command[0] = 'u';
                    if (cursorY > 0) cursorY--;
                    break;
                case 80:
                    command[0] = 'd';
                    if (cursorY < maxHeight - 1) cursorY++;
                    break;
                case 77:
                    command[0] = 'r';
                    if (cursorX < maxWidth - 3) cursorX++;
                    break;
                case 75:
                    command[0] = 'l';
                    if (cursorX > 0) cursorX--;
                    break;
            }
            if (send(clientSock, command, 2, 0) == SOCKET_ERROR) {
                cout << "Send failed with error: " << WSAGetLastError() << endl;
                closesocket(clientSock);
                WSACleanup();
                exit(1);
            }
            DisplaySmiley();
        }
    }
    return 0;
}

DWORD WINAPI ResponseReceiver(void* data) {
    while (true) {
        char resp[BUFFER_SIZE];
        int numReceived = recv(clientSock, resp, BUFFER_SIZE, 0);
        resp[numReceived] = '\0';
        if (numReceived > 0) {
            cout << resp << endl;
        } else if (numReceived == 0) {
            cout << "Server closed the connection.\n";
        } else {
            cout << "Recv failed with error: " << WSAGetLastError() << endl;
        }
    }
    return 0;
}

int main() {
    setlocale(0, "");
    system("title Emoji Control Panel");

    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        cout << "WSAStartup failed.\n";
        return 1;
    }

    struct addrinfo hints, *servinfo;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("127.0.0.1", SERVER_PORT, &hints, &servinfo) != 0) {
        cout << "Getaddrinfo failed.\n";
        WSACleanup();
        return 1;
    }

    for (auto p = servinfo; p != NULL; p = p->ai_next) {
        clientSock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientSock == INVALID_SOCKET) {
            cout << "Socket creation failed with error: " << WSAGetLastError() << endl;
            WSACleanup();
            return 1;
        }

        if (connect(clientSock, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            closesocket(clientSock);
            clientSock = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (clientSock == INVALID_SOCKET) {
        cout << "Unable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    DisableCursor();

    HANDLE hThreadSend = CreateThread(NULL, 0, CommandSender, NULL, 0, NULL);
    HANDLE hThreadReceive = CreateThread(NULL, 0, ResponseReceiver, NULL, 0, NULL);

    WaitForSingleObject(hThreadSend, INFINITE);
    WaitForSingleObject(hThreadReceive, INFINITE);

    CloseHandle(hThreadSend);
    CloseHandle(hThreadReceive);

    closesocket(clientSock);
    WSACleanup();

    return 0;
}