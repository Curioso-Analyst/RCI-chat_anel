// camada_topologica.h
#ifndef CAMADA_TOPOLOGICA_H
#define CAMADA_TOPOLOGICA_H

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

extern char* SERVER_IP;
extern char* PORT;
extern char* TCP_escolhido;

typedef struct Node {
    int id;
    char ip[16];
    char tcp[6];
    struct Node* sucessor;
    struct Node* predecessor;
    struct Node* second_successor;
    struct Node* corda;
} Node;

Node* createNode(int id, char* ip, char* tcp);
void registerNode(Node* node, int ring, char* IP, char* TCP, char* user_input);
void unregisterNode(Node* node, char* user_input);
int getUniqueIdentifier(char* nodes_list);
void getNodes(int ring, char* user_input);

#endif // CAMADA_TOPOLOGICA_H;