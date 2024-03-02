#include "camada_topologica_tcp.h"


void cliente_tcp(Node* node, char* succTCP) {
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

    int errcode = getaddrinfo(node->ip, succTCP, &hints, &res);
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

    // Envia a mensagem "READY" para o servidor
    char* readyMsg = "READY";
    send(fd, readyMsg, strlen(readyMsg), 0);
    printf("Mensagem READY enviada.\n");

    // Adiciona um atraso antes de enviar a mensagem "ENTRY"
    sleep(4);

    // Em seguida, envia a mensagem "ENTRY"
    send_entry(fd, node);


    /*// Código para receber e processar a resposta do servidor
    char response[1024];
    int valread = read(fd, response, 1024);
    if (valread > 0) {
        response[valread] = '\0';
        printf("Resposta recebida: %s\n", response);

        // Código para processar a resposta
        // Se a resposta for "SUCC", atualizar o sucessor do nó
        char command[5];
        int id;
        char ip[16];
        char tcp[6];
        sscanf(response, "%s %d %s %s", command, &id, ip, tcp);
        if (strcmp(command, "SUCC") == 0) {
            // Atualiza o sucessor do nó
            node->sucessor = createNode(id, ip, tcp);
            printf("Sucessor atualizado: %d %s %s\n", node->sucessor->id, node->sucessor->ip, node->sucessor->tcp);
        }
    } else if (valread == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }*/

    freeaddrinfo(res);
    close(fd);
}

void send_entry(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "ENTRY %02d %s %s", node->id, node->ip, node->tcp);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada.\n");
}

void send_succ(int fd, Node* node){
    char buffer[1024];
    sprintf(buffer, "SUCC %02d %s %s", node->id, node->ip, node->tcp);
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = send(fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada!\n");
};


