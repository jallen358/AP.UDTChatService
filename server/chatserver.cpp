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
#include <string>
#include <cstring>
#include <fstream>
#include <list>
#include <pthread.h>
using namespace std;

#define MAX_CLIENTS 5
#define BUFF_S 10000

static unsigned int num_clients = 0;
static int cid = 10;

typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int cid;
    char name[32];
}client_t;

client_t* clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_clients(client_t* client) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            clients[i] = client;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void remove_clients(int cid) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->cid == cid) {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void send_msg(char* s, int cid) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->cid != cid) {
                if (UDT::send(clients[i]->sockfd, s, strlen(s), 0) < 0) {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
void shorten(char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { 
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}
void* clientLoop(void* arg) {
    char buffer[BUFF_S];
    char name[32];
    int leave = 0;

    num_clients++;
    client_t* cliPtr = (client_t*)arg;
    
    UDT::recv(cliPtr->sockfd, name, 32, 0);
    strcpy(cliPtr->name, name);
    sprintf(buffer, "%s joined the chatroom\n", cliPtr->name);
    printf("%s", buffer);
    send_msg(buffer, cliPtr->cid);

    memset(buffer, 0, BUFF_S);

    while (1) {
        if (leave == 1) {
            break;
        }
        int receive = UDT::recv(cliPtr->sockfd, buffer, BUFF_S, 0);
        if (receive > 0) {
            if (strlen(buffer) > 0) {
                send_msg(buffer, cliPtr->cid);
                shorten(buffer, strlen(buffer));
                printf("%s\n", buffer);
            }
        }
        else if (receive == 0 || strcmp(buffer, "Exit") == 0) {
            leave = 1;
        }
        else {
            leave = 1;
        }
        memset(buffer, 0, BUFF_S);
    }
    UDT::close(cliPtr->sockfd);
    remove_clients(cliPtr->cid);
    free(cliPtr);
    num_clients--;
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char *argv[]) {

    char* PORT = argv[2];
    int PORTnum = atoi(argv[2]);
    char pass[100] = "";
    strcpy(pass, argv[4]);
    UDT::startup();
    
    UDTSOCKET server;

    addrinfo serv;
    addrinfo* res;

    pthread_t tid;

    memset(&serv, 0, sizeof(struct addrinfo));
    serv.ai_flags = AI_PASSIVE;
    serv.ai_family = AF_INET;
    serv.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(NULL, PORT, &serv, &res)) {
        return 0; 
    }

    server = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    UDT::bind(server, res->ai_addr, res->ai_addrlen);
    
    

    freeaddrinfo(res);

    UDT::listen(server, 10);
    

    cout << "Server started on port " << PORTnum <<". Accepting connections" << endl;    

    UDTSOCKET recver; 

    sockaddr_in clientAddr;
    

    while (true)
    {
        int addrlen = sizeof(clientAddr);


        if (UDT::INVALID_SOCK == (recver = UDT::accept(server, (sockaddr*)&clientAddr, &addrlen))) {
            return 0;
        }

        client_t* cli = (client_t*)malloc(sizeof(client_t));

        cli->address = clientAddr;
        cli->sockfd = recver;
        cli->cid = cid++;

        char clienthost[NI_MAXHOST];
        char clientservice[NI_MAXSERV];
        getnameinfo((sockaddr*)&clientAddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST | NI_NUMERICSERV);

        int passlen;
        char* clientPass;
        if (UDT::ERROR == UDT::recv(recver, (char*)&passlen, sizeof(int), 0)) {
            cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
            return 0;
        }

        clientPass = new char[passlen];
        memset(clientPass, 0, passlen);

        if (UDT::ERROR == UDT::recv(recver, (char*)clientPass, passlen, 0)) {
            cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
            return 0;
        }
        
        // Validation to Client
        char valid[100];
        char temp[100];
        string data; 
        if (strcmp((const char*)clientPass, (const char*)pass) != 0) {
            sprintf(temp, "2"); 
        }
        else {
            sprintf(temp, "1");
        }
        UDT::send(recver, temp, strlen(temp), 0);


        add_clients(cli);
        pthread_create(&tid, NULL, &clientLoop, (void*)cli);

       
        sleep(1);

    }



        




    // For each client? 
    
   
        // when first connected. Send Passcode and if invalid, client should cancel connection
        // else: receives messages from the client

        // send msg to all other client

    // epoll stuff? 


    UDT::close(server);
    return 0;
}


