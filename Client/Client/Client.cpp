#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <string.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int iResult;
WSADATA wsaData;

SOCKET SendSocket = INVALID_SOCKET;
sockaddr_in RecvAddr;

unsigned short Port = 27015;

char RecvBuf[1024];
char SendBuf[1024];

int BufLen = 1024;
int RecvAddrSize = sizeof(RecvAddr);

bool isConnected = false;

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
    //strcpy_s(SendBuf, "0Salut de la client!");
    //---------------------------------------------
    // Send a datagram to the receiver
    wprintf(L"Sending a datagram to the receiver...\n");
    iResult = sendto(SendSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
        closesocket(SendSocket);
        WSACleanup();
        return 1;
    }
}

void recv()
{
    //-----------------------------------------------
    // Call the recvfrom function to receive datagrams
    // on the bound socket.
    wprintf(L"Receiving datagrams...\n");
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
    if (RecvBuf[0] == ACK)
    {
        isConnected = true;
        printf("From server: Welcome to the server! You are connected! \n");        
    }
    else if (RecvBuf[0] == PING) // PING message
    {
        printf("From server: PING \n");
        SendBuf[0] = PONG;
        send_to(SendBuf);
    }
    else if (RecvBuf[0] == PONG) // PONG message
    {
        printf("From server: PONG \n");
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
}

int main()
{
    int return_initialize;
    return_initialize = Initialize();
    if (return_initialize == 1)
        return 0;

    while (true)
    {
        Update();
        Sleep(2000); //sleeps 10 ms
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
