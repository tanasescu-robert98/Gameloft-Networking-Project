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
#include <map>
using namespace std;

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")


int iResult = 0;

WSADATA wsaData;

SOCKET RecvSocket;
struct sockaddr_in RecvAddr;

//vector<struct sockaddr_in> clients;
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

//int received_a_message_flag = 0;

float sum_of_pong_delay = 0;

time_t start_time;
time_t end_time;

time_t start_time_for_ping_send;
time_t end_time_for_ping_send;

//string recv_message;

struct Received_Messages_Struct
{
    string Sender_Message;
    sockaddr_in Sender_Address;
};

list<Received_Messages_Struct> received_messages_list;

struct Connected_Client
{
    sockaddr_in Address;
    int is_active_flag = 0;
    chrono::high_resolution_clock::time_point ping_sent_timestamp;
    chrono::high_resolution_clock::time_point pong_received_timestamp;
    float sum_of_pong_delay_client = 0;
    float number_of_pongs_client = 0;
};

vector<Connected_Client> vector_connected_clients;

int client_found = 0;

int initialize_ping_sending = 0;

int previous_second = 0;

int recv_finished = -1;

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

int send_to(char sendbuffer[1024], sockaddr_in Sender)
{
    iResult = sendto(RecvSocket,
        SendBuf, BufLen, 0, (SOCKADDR*)&Sender, sizeof(Sender));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
        closesocket(RecvSocket);
        WSACleanup();
        return 1;
    }
}

void message_handler()
{
    while (received_messages_list.size() > 0)
    {
        Received_Messages_Struct recv_message = received_messages_list.front();
        received_messages_list.pop_front();
        switch (recv_message.Sender_Message[0])
        {
            case HELLO:
                address_found = false;
                // search for the address and port in the vector
                for (Connected_Client& iterator_clients : vector_connected_clients)
                {
                    if (iterator_clients.Address.sin_addr.S_un.S_addr == recv_message.Sender_Address.sin_addr.S_un.S_addr &&
                        iterator_clients.Address.sin_port == recv_message.Sender_Address.sin_port)
                    {
                        address_found = true;
                        break;
                    }
                }
                if (address_found == false) // if the address and port are new add them to the vector
                {
                    char Sender_addr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &recv_message.Sender_Address.sin_addr, Sender_addr, sizeof(Sender_addr));
                    printf("%s with port %d connected! \n", Sender_addr, recv_message.Sender_Address.sin_port);

                    Connected_Client new_client;
                    new_client.Address = recv_message.Sender_Address;
                    new_client.is_active_flag = 0;
                    new_client.sum_of_pong_delay_client = 0;
                    new_client.number_of_pongs_client = 0;
                    vector_connected_clients.push_back(new_client);

                    SendBuf[0] = ACK;
                    send_to(SendBuf, recv_message.Sender_Address);
                }
                break;

            case PONG:
                for (Connected_Client& iterator : vector_connected_clients)
                {
                    if (iterator.Address.sin_addr.S_un.S_addr == recv_message.Sender_Address.sin_addr.S_un.S_addr
                        && iterator.Address.sin_port == recv_message.Sender_Address.sin_port)
                    {
                        iterator.is_active_flag = 1;
                        auto pong_time_stamp = chrono::steady_clock::now();
                        iterator.pong_received_timestamp = pong_time_stamp;
                        iterator.number_of_pongs_client++;
                        number_of_pongs++;
                        iterator.sum_of_pong_delay_client += chrono::duration_cast<chrono::milliseconds>(iterator.pong_received_timestamp - iterator.ping_sent_timestamp).count();
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &iterator.Address.sin_addr, ip, sizeof(ip));
                        cout << "From " << ip << " with port " << iterator.Address.sin_port << ": PONG" << endl;
                        break;
                        //printf("From client: PONG \n");
                    }
                }
                break;
        }
    }
}

void recv()
{
    iResult = recvfrom(RecvSocket,
        RecvBuf, BufLen, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);
    if (iResult == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            recv_finished = 0;
            return;
        }
        else
        {
            wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
        }
    }

    Received_Messages_Struct new_message_received;
    new_message_received.Sender_Address = SenderAddr;
    new_message_received.Sender_Message = RecvBuf;
    received_messages_list.push_back(new_message_received);
}

void send_ping_to_clients()
{
    for (Connected_Client& iterator : vector_connected_clients)
    {
        SendBuf[0] = PING;
        auto ping_time_stamp = chrono::steady_clock::now();
        iterator.ping_sent_timestamp = ping_time_stamp;
        send_to(SendBuf, iterator.Address);
    }
}

void time_handler()
{
    auto end = chrono::steady_clock::now();
    auto current_second = chrono::duration_cast<chrono::seconds>(end - start).count();
    if (current_second - previous_second >= 1)
    {
        send_ping_to_clients();

        cout << "Elapsed time in seconds: "
            << chrono::duration_cast<chrono::seconds>(end - start).count()
            << " sec" << endl;
        if (chrono::duration_cast<chrono::seconds>(end - start).count() % 15 == 0
            && chrono::duration_cast<chrono::seconds>(end - start).count() != 0)
        {
            vector<int> clients_to_erase;
            for (int i = 0; i < vector_connected_clients.size(); i++)
            {
                char ip_of_client[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &vector_connected_clients[i].Address.sin_addr, ip_of_client, sizeof(ip_of_client));
                if (vector_connected_clients[i].is_active_flag == 0)
                {
                    cout << "\nThere were no received messages from " << ip_of_client << " with port " << vector_connected_clients[i].Address.sin_port << ".Closing connection!" << endl;
                    clients_to_erase.push_back(i);
                }
                else
                {
                    cout << "\nThere were some received messages from " << ip_of_client << " with port " << vector_connected_clients[i].Address.sin_port << "!" << endl;
                }
                vector_connected_clients[i].is_active_flag = 0;
            }
            for (int i : clients_to_erase)
            {
                vector_connected_clients.erase(vector_connected_clients.begin() + i);
            }
            cout << "Number of connected clients : " << vector_connected_clients.size() << endl;
            cout << endl;
        }
        if (chrono::duration_cast<chrono::seconds>(end - start).count() % 10 == 0)
        {
            for (Connected_Client& iterator : vector_connected_clients)
            {
                cout << endl;
                char client_addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &iterator.Address.sin_addr, client_addr, sizeof(client_addr));
                cout << "Number of pongs received in 10 seconds from " << client_addr << " with port " << iterator.Address.sin_port << " : " << iterator.number_of_pongs_client << endl;
                if (iterator.number_of_pongs_client > 0)
                {
                    cout << "With the average delay : " << iterator.sum_of_pong_delay_client / iterator.number_of_pongs_client << " ms" << endl;
                }
                iterator.sum_of_pong_delay_client = 0;
                iterator.number_of_pongs_client = 0;
                sum_of_pong_delay = 0;
                cout << endl;
            }
        }
        previous_second = current_second;
    }
}

void Update()
{
    while (recv_finished != 0)
    {
        recv();
    }

    recv_finished = -1;

    message_handler();
    
    time_handler();

    memset(RecvBuf, 0, strlen(RecvBuf));
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


