#include "camada_topologica_tcp.h"

int cliente_tcp(Node* node,char* j_ip,char* j_port) {
    int fd;
    struct addrinfo hints, *res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket tcp");
        return -1;  // Retorna -1 em caso de erro
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int errcode = getaddrinfo(j_ip, j_port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        return -1;  // Retorna -1 em caso de erro
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        return -1;  // Retorna -1 em caso de erro
    }
    // Conectado ao servidor.
    printf("\n------Conectado ao anel------\n");

    freeaddrinfo(res);
    return fd;
}

void send_entry(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "ENTRY %02d %s %s\n", node->id, node->ip, node->tcp);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send entry");
        exit(EXIT_FAILURE);
    }
}

void send_succ(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "SUCC %02d %s %s\n", node->id, node->ip, node->tcp);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send succ");
        exit(EXIT_FAILURE);
    }
}

void send_pred(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "PRED %02d\n", node->id);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send pred");
        exit(EXIT_FAILURE);
    }
}

void send_chord(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "CHORD %02d\n", node->id);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send corda");
        exit(EXIT_FAILURE);
    }
}


void removeNode(Node** node_to_remove_ptr) {
    Node* node_to_remove = *node_to_remove_ptr;

    // Verifica se o nó a remover é válido
    if (node_to_remove != NULL) {
        Node* successor = node_to_remove->sucessor;
        Node* second_successor = node_to_remove->second_successor;
        Node* predecessor = node_to_remove->predecessor;

        // Atualiza os ponteiros dos nós vizinhos
        if (predecessor != NULL) {
            predecessor->sucessor = successor;
            predecessor->second_successor = second_successor;
        }

        if (successor != NULL) {
            successor->predecessor = predecessor;
        }

        // Liberta a memória do nó a remover
        free(node_to_remove);

        // Atualiza o ponteiro para o nó a remover
        *node_to_remove_ptr = NULL;
    }
}