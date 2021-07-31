#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <time.h>
#include <list>
using namespace std;

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")


int iResult = 0;

WSADATA wsaData;

SOCKET RecvSocket;
struct sockaddr_in RecvAddr;

vector<struct sockaddr_in> clients;
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

int initiate_ping_pong = 0;

auto start = chrono::steady_clock::now();

int number_of_pongs = 0;

int received_a_message_flag = 0;

double sum_of_pong_delay = 0;

time_t start_time;
time_t end_time;

list<string> received_messages;

string recv_message;

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

void message_handler()
{
    recv_message = received_messages.front();
    received_messages.pop_front();
    if (recv_message[0] == HELLO) // if hello message received
    {
        address_found = false;
        // search for the address and port in the vector
        for (struct sockaddr_in connected_client : clients) {
            if (connected_client.sin_addr.S_un.S_addr == SenderAddr.sin_addr.S_un.S_addr &&
                connected_client.sin_port == SenderAddr.sin_port)
            {
                address_found = true;
                break;
            }
        }
        if (address_found == false) // if the address and port are new add them to the vector
        {
            received_a_message_flag = 1;
            char Sender_addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &SenderAddr.sin_addr, Sender_addr, sizeof(Sender_addr));
            printf("%s with port %d connected! \n", Sender_addr, SenderAddr.sin_port);
            clients.push_back(SenderAddr);
            SendBuf[0] = ACK;
            send_to(SendBuf);
        }
    }
    if (initiate_ping_pong == 1) // initiate ping-pong mecanism
    {
        start_time = time(NULL);
        SendBuf[0] = PING;
        initiate_ping_pong = 2;
        send_to(SendBuf);
    }
    if (recv_message[0] == PONG) // if PONG message received
    {
        end_time = time(NULL);
        received_a_message_flag = 1;
        number_of_pongs++;
        sum_of_pong_delay += (double)(end_time - start_time);
        /*std::cout << "Average Time: " << (double)(end_time - start_time) << " Seconds" << std::endl;*/
        printf("From client: PONG \n");
        SendBuf[0] = PING;
        start_time = time(NULL);
        send_to(SendBuf);
    }
    // if ping pong mecanism wasn't initiated yet, initiate it now
    if (initiate_ping_pong == 0)
        initiate_ping_pong = 1;
    recv_message.clear();
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
    received_messages.push_back(RecvBuf);
    if(received_messages.size() > 0)
        message_handler();
}

void Update()
{
    recv();

    auto end = chrono::steady_clock::now();

    cout << "Elapsed time in seconds: "
        << chrono::duration_cast<chrono::seconds>(end - start).count()
        << " sec" << endl;
    if (chrono::duration_cast<chrono::seconds>(end - start).count() % 15 == 0
        && chrono::duration_cast<chrono::seconds>(end - start).count() != 0)
    {
        if (received_a_message_flag == 1)
        { 
            cout << "There were some received messages!" << endl;
        }
        else
        {
            cout << "There were no received messages. Closing connection!" << endl;
            if(clients.size() > 0)
                clients.pop_back();
        }
        received_a_message_flag = 0;
    }
    if (chrono::duration_cast<chrono::seconds>(end - start).count() % 10 == 0)
    {
        cout << "Number of pongs received in 10 seconds : " << number_of_pongs << endl;
        if(number_of_pongs > 0)
            cout << "With the average delay : " << sum_of_pong_delay / number_of_pongs << " seconds" << endl;
        number_of_pongs = 0;
        sum_of_pong_delay = 0;
    }
    memset(RecvBuf, 0, strlen(RecvBuf));
}

int main()
{
    auto start = chrono::steady_clock::now();
    int return_initialize;
    return_initialize = Initialize();

    if (return_initialize == 1)
        return 0;

    while (true)
    {
        Update();
        Sleep(5000); //sleeps 10 ms
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


