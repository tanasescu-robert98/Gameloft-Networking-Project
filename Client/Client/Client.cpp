#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <iostream>

using namespace std;

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int iResult = 0;
WSADATA wsaData;

SOCKET SendSocket = INVALID_SOCKET;
sockaddr_in RecvAddr;

unsigned short Port = 27015;

char RecvBuf[1024];
char SendBuf[1024];

int BufLen = 1024;
int RecvAddrSize = sizeof(RecvAddr);

bool isConnected = false;

auto start = chrono::steady_clock::now();

int received_a_message_flag = 0;

int previous_second = 0;

enum Message_Type
{
    HELLO = 1,
    ACK = 2,
    PING = 3,
    PONG = 4
};

int Initialize()
{
    //----------------------
   // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    //---------------------------------------------
    // Create a socket for sending data
    SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SendSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    u_long iMode = 1;
    ioctlsocket(SendSocket, FIONBIO, &iMode);

    //---------------------------------------------
    // Set up the RecvAddr structure with the IP address of
    // the receiver (in this example case "192.168.1.1")
    // and the specified port number.
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(Port);
    RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

int send_to(char sendbuffer[1024])
{
    iResult = sendto(SendSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
        closesocket(SendSocket);
        WSACleanup();
        return 1;
    }
}

void message_handler()
{
    switch (RecvBuf[0])
    {
        case ACK:
            received_a_message_flag = 1;
            isConnected = true;
            printf("From server: Welcome to the server! You are connected! \n");
            break;
        case PING:
            received_a_message_flag = 1;
            printf("From server: PING \n");
            SendBuf[0] = PONG;
            send_to(SendBuf);
            break;
    }
}

void recv()
{
    iResult = recvfrom(SendSocket,
        RecvBuf, BufLen, 0, (SOCKADDR*)&RecvAddr, &RecvAddrSize);
    if (iResult == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return;
        }
        else
        {
            wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
        }
    }

    message_handler();
}

void time_handler()
{
    auto end = chrono::steady_clock::now();
    auto current_second = chrono::duration_cast<chrono::seconds>(end - start).count();
    if (current_second != previous_second)
    {
        cout << "Elapsed time in seconds: "
            << chrono::duration_cast<chrono::seconds>(end - start).count()
            << " sec" << endl;
        if (chrono::duration_cast<chrono::seconds>(end - start).count() % 15 == 0
            && chrono::duration_cast<chrono::seconds>(end - start).count() != 0)
        {
            if (received_a_message_flag == 1)
            {
                cout << "There were some received messages from server!" << endl;
            }
            else
            {
                cout << "There were no received messages. Closing connection!" << endl;
                closesocket(SendSocket);
                WSACleanup();
                exit(0);
            }
            received_a_message_flag = 0;
        }
        previous_second = current_second;
    }
}

void Update()
{
    if (isConnected == false)
    {
        SendBuf[0] = HELLO;
        send_to(SendBuf);
    }

    recv();

    time_handler();
}

int main()
{
    auto start = chrono::steady_clock::now();
    int return_initialize = -1;
    return_initialize = Initialize();
    if (return_initialize == 1)
        return 0;

    while (true)
    {
        Update();
        Sleep(30); //sleeps 10 ms
    }
    //---------------------------------------------
    // When the application is finished sending, close the socket.
    wprintf(L"Finished sending. Closing socket.\n");
    iResult = closesocket(SendSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //---------------------------------------------
    // Clean up and quit.
    wprintf(L"Exiting.\n");
    WSACleanup();
    return 0;
}
