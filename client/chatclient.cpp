/**
 * Author: Jason Allen
 * GTID: 903535932
 * GT Email: jallen358@gatech.edu
 */
#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#endif
#include <iostream>
#include <udt.h>
#include <fstream>
#include <cstring>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


using namespace std;
#define LENGTH 10000
UDTSOCKET client; 
char name[32];
int flag = 0;

void shorten(char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // trim \n
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}
void exitCli(int sig) {
    flag = 1;
}
void flushOut() {
    fflush(stdout);
}
void* send_msg(void* args) {
    char msg[LENGTH] = {};
    char buffer[LENGTH + 32] = {};

    while (1) {
        flushOut();
        fgets(msg, LENGTH, stdin);
        shorten(msg, LENGTH);
        if (strcmp(msg, ":Exit") == 0) {
            flag = 1;
            return NULL;
        } else if (strcmp(msg, ":)") == 0) {
            strcpy(msg, "[feeling happy]");
            sprintf(buffer, "%s: %s\n", name, msg);
            UDT::send(client, buffer, strlen(buffer), 0);
        }
        else if (strcmp(msg, ":(") == 0) {
            strcpy(msg, "[feeling sad]");
            sprintf(buffer, "%s: %s\n", name, msg);
            UDT::send(client, buffer, strlen(buffer), 0);
            memset(msg, 0, LENGTH);
            memset(buffer, 0, LENGTH + 32);
        }
        else if (strcmp(msg, ":mytime") == 0) {
            time_t r;
            struct tm* timeinfo;
            time(&r);
            timeinfo = localtime(&r);
            strcpy(msg, asctime(timeinfo));
            sprintf(buffer, "%s: %s", name, msg);
            UDT::send(client, buffer, strlen(buffer), 0);
        }
        else if (strcmp(msg, ":+1hr") == 0) {
            time_t r;
            struct tm* timeinfo;
            time(&r);
            timeinfo = localtime(&r);
            timeinfo->tm_hour = timeinfo->tm_hour + 1;
            strcpy(msg, asctime(timeinfo));
            sprintf(buffer, "%s: %s", name, msg);
            UDT::send(client, buffer, strlen(buffer), 0);

        } else {
            sprintf(buffer, "%s: %s\n", name, msg);
            UDT::send(client, buffer, strlen(buffer), 0);
        }
        memset(msg, 0, LENGTH);
        memset(buffer, 0, LENGTH + 32);
    }
    flag = 1;
}

void* recv_msg(void* args) {
    char msg[LENGTH] = {};
    while (1) {
        int receive = UDT::recv(client, msg, LENGTH + 32, 0);
        if (receive > 0) {
            printf("%s", msg);
        }
        else if (receive == 0) {
            break;
        }
        memset(msg, 0, sizeof(msg));
    }
}
int main(int argc, char *argv[]) {

    UDT::startup();
    char *hostIP = argv[3];
    char* username = argv[5];
    char *passcode = argv[7];
    char* PORT = argv[9];
    int PORTnum = atoi(argv[9]);
    char msg[10000];
    
    struct addrinfo hints, *local, *peer; 

    memset(&hints, 0, sizeof(struct addrinfo));

    strcpy(name, argv[5]);

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(NULL, PORT, &hints, &local))
    {
        cout << "incorrect network address.\n" << endl;
        return 0;
    }

    struct sockaddr_in server_addr;

    client = UDT::socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORTnum);

    freeaddrinfo(local);

    if (0 != getaddrinfo("127.0.0.1", PORT, &hints, &peer))
    {
        cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
        return 0;
    }

    if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen))
    {
        cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    freeaddrinfo(peer);


    int size = 10000;
    // Password Check
    char* data = new char[size];
    int len = strlen(passcode);
    UDT::send(client, (char*)&len, sizeof(int), 0);

    UDT::send(client, passcode, len, 0);
    
    // Username for "Joined the chat room"

    char valid[100];
    
    UDT::recv(client, valid, 100, 0);
    if (strcmp(valid, "2") == 0)
    {
        cout << "Incorrect Passcode";
        return 0;
    }
    cout << "Connected to " << hostIP << " on port " << PORT << endl;
    
    
    UDT::send(client, username, 32, 0);
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL,  send_msg, NULL) != 0) {
        
        return EXIT_FAILURE;
    }
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, recv_msg, NULL) != 0) {
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    
    while (true)
    {

        if (flag == 1) {
            break;
        }
    }

   
    UDT::close(client);
    return 0;
}


