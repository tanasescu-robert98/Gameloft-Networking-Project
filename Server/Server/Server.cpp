#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <vector>

using namespace std;

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")


int iResult = 0;

WSADATA wsaData;

SOCKET RecvSocket;
struct sockaddr_in RecvAddr;

sockaddr_in vector_sock[100];
vector<struct sockaddr_in> clients;
//char vector_adrese[100][100];
int numar_adrese = 0;
int address_found = 0;

unsigned short Port = 27015;

char RecvBuf[1024];
char SendBuf[1024];
int BufLen = 1024;

struct sockaddr_in SenderAddr;
int SenderAddrSize = sizeof(SenderAddr);

enum Message_Type
{
    HELLO = 1,
    ACK = 2,
    PING = 3,
    PONG = 4
};

int ping_pong = 0;

int Initialize()
{
    //-----------------------------------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error %d\n", iResult);
        return 1;
    }
    //-----------------------------------------------
    // Create a receiver socket to receive datagrams
    RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
if (RecvSocket == INVALID_SOCKET) {
    wprintf(L"socket failed with error %d\n", WSAGetLastError());
    return 1;
}

u_long iMode = 1;
ioctlsocket(RecvSocket, FIONBIO, &iMode);

//-----------------------------------------------
// Bind the socket to any address and the specified port.
RecvAddr.sin_family = AF_INET;
RecvAddr.sin_port = htons(Port);
RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

iResult = bind(RecvSocket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
if (iResult != 0) {
    wprintf(L"bind failed with error %d\n", WSAGetLastError());
    return 1;
}


}

int send_to(char sendbuffer[1024])
{
    //strcpy_s(SendBuf, "Salut de la server!");
    //---------------------------------------------
    // Send a datagram to the receiver
    wprintf(L"Sending a datagram to the receiver...\n");
    iResult = sendto(RecvSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, sizeof(SenderAddr));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
        closesocket(RecvSocket);
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
    iResult = recvfrom(RecvSocket,
        RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
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
    address_found = false;
    for (struct sockaddr_in connected_client : clients) {
        if (connected_client.sin_addr.S_un.S_addr == SenderAddr.sin_addr.S_un.S_addr &&
            connected_client.sin_port == SenderAddr.sin_port) 
        {
            address_found = true;
            break;
        }
    }
    if (address_found == false)
    {
        char Sender_addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &SenderAddr.sin_addr, Sender_addr, sizeof(Sender_addr));
        printf("%s with port %d connected! \n", Sender_addr, SenderAddr.sin_port);
        clients.push_back(SenderAddr);
        SendBuf[0] = ACK;
        send_to(SendBuf);
    }

    if (ping_pong == 1)
    {
        SendBuf[0] = PING;
        ping_pong = 2;
        send_to(SendBuf);
    }
    if (RecvBuf[0] == PING) // PING message
    {
        printf("From client: PING \n");
        SendBuf[0] = PONG;
        send_to(SendBuf);
    }
    else if (RecvBuf[0] == PONG) // PONG message
    {
        
        printf("From client: PONG \n");
        SendBuf[0] = PING;
        send_to(SendBuf);
    }
    if(ping_pong != 2)
    ping_pong = 1;
}



void Update()
{
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


    //-----------------------------------------------
    // Close the socket when finished receiving datagrams
    wprintf(L"Finished receiving. Closing socket.\n");
    iResult = closesocket(RecvSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket failed with error %d\n", WSAGetLastError());
        return 1;
    }

    //-----------------------------------------------
    // Clean up and exit.
    wprintf(L"Exiting.\n");
    WSACleanup();
    return 0;
}


