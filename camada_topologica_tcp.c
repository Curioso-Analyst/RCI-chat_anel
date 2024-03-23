#include "camada_topologica_tcp.h"


int cliente_tcp(Node* node,char* j_ip,char* j_port) {
    int fd;
    struct addrinfo hints, *res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket criado.\n");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int errcode = getaddrinfo(j_ip, j_port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }
    printf("getaddrinfo executado com sucesso.\n");

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // Conectado ao servidor.
    printf("Conectado ao servidor.\n");

    freeaddrinfo(res);
    return fd;
}

void send_entry(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "ENTRY %02d %s %s\n", node->id, node->ip, node->tcp);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada para o socket %d: %s\n",fd, buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada.\n");
}

void send_succ(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "SUCC %02d %s %s\n", node->id, node->ip, node->tcp);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada!\n");
}

void send_succ1(int fd, Node* node, char* mensagem){
    char buffer[1024];
    sprintf(buffer, "SUCC %02d %s %s\n", node->id, node->ip, node->tcp);
    strcat(buffer, mensagem);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada!\n");
}

void send_pred(char fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "PRED %02d\n", node->id);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada.\n");
}

void send_pred1(char fd, Node* node, char* mensagem){
    char buffer[1024];
    sprintf(buffer, "PRED %02d\n", node->id);
    strcat(buffer, mensagem);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada.\n");
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