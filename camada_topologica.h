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
#include <stdbool.h>

#define MAX_CORDAS 20

extern int global_variable;

extern char* SERVER_IP;
extern char* PORT;
extern char* TCP_escolhido;

typedef struct Node {
    int id;
    char ip[16];
    char tcp[6];
    int ring;
    int corda_socket_fd; // File descriptor do socket de comunicação com o nó(cliente)
    int corda_socket_recebidas_fd; // File descriptor do socket de comunicação com o nó que enviou a corda (servidor)
    int pred_socket_fd; // File descriptor do socket de comunicação com o predecessor
    int suc_socket_fd; // File descriptor do socket de comunicação com o sucessor
    struct Node* sucessor;
    struct Node* predecessor;
    struct Node* second_successor;
    struct Node* corda;
    struct Node* cordas[MAX_CORDAS]; // Lista de cordas recebidas por este nó
    int num_cordas; // Número de cordas recebidas
} Node;

Node* createNode(int id, char* ip, char* tcp);
void registerNode(Node* node, int ring, char* IP, char* TCP, char* user_input);
void regservidornos(Node* node,int fd, char* user_input, char* nodes_list, struct addrinfo* server_info, struct sockaddr_in* addr, int ring, char* IP, char* TCP);
void unregisterNode(Node* node, char* user_input);
int getUniqueIdentifier(char* nodes_list);
void getNodes(int ring, char* user_input);
void getNodescorda(Node* node, char* buffer);
void establishChord(Node* node);
void removeChord(Node* node);

#endif // CAMADA_TOPOLOGICA_H;