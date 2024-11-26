#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>



// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_PORT "27015"
#define BUFSIZE 4096

using namespace std;

class Client
{

private:
    WSADATA wsaData;
    static SOCKET ConnectSocket;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    string sendbuf;
    char recvbuf[BUFSIZE];
    int iResult;
    int recvbuflen = BUFSIZE;

    static string ip_address;

    static DWORD WINAPI RECV_MESS_FROM_SERV() {

        char recvbuf[BUFSIZE];
        int iResult;
        

        for (;;) {
            
            ZeroMemory(recvbuf, BUFSIZE);

            iResult = recv(ConnectSocket, recvbuf, BUFSIZE, 0);

            if (iResult > 0) {

                cout << recvbuf << endl;

            }
            else {
                //cout << recvbuf << " " << iResult << endl;
                return 1;
            }
        
        
        
        }


        exit(-1);
    }

    static DWORD WINAPI SEND_MESS_FROM_SERV() {
        int iResult;
 
        CHAR chBuf[BUFSIZE];

       

        for (;;) {

            ZeroMemory(chBuf, BUFSIZE);
            cin.getline(chBuf, BUFSIZE);
            //cout << chBuf << endl;

            iResult = send(ConnectSocket,chBuf, BUFSIZE, 0);
            if (iResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }   
        }

        exit(-1);

    }



public:

    Client() {
        ip_address = "localhost";
    }   

    Client(string ip_address) {
        this->ip_address = ip_address;
    }
    int Connected() {

        ConnectSocket = INVALID_SOCKET;


        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return 1;
        }


        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        iResult = getaddrinfo(ip_address.c_str(), DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

            // Create a SOCKET for connecting to server
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (ConnectSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                return 1;
            }

            // Connect to server.
            iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            WSACleanup();
            return 1;
        }





        HANDLE hThreads[2];


        hThreads[0] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)RECV_MESS_FROM_SERV, NULL, NULL, NULL);
        hThreads[1] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SEND_MESS_FROM_SERV, NULL, NULL, NULL);



        WaitForMultipleObjects(2, hThreads, FALSE, INFINITE);

        CloseHandle(hThreads[0]);
        CloseHandle(hThreads[1]);






      

        // cleanup
        closesocket(ConnectSocket);
        WSACleanup();

        return 0;
    }

};

string Client::ip_address;
SOCKET Client::ConnectSocket;



int __cdecl main(int argc, char** argv)
{


    if (argc == 1) {
        Client client;
        client.Connected();
    }
    else if (argc == 2) {
        string ip = argv[1];
        Client client{ip};
        client.Connected();
    }
    else {
        cout << "Fail programm";
    }

    
   
}