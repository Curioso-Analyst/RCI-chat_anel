#include "camada_chat.h"

// Função que envia uma mensagem de chat
void send_chat(int new_socket_suc, Node* node, int dest, char* mensagem){
    // Verifica se a mensagem é muito longa
    if (strlen(mensagem) > 128) {
        printf("Erro: A mensagem é muito longa.\n");
        return;
    }

    char buffer[1024];
    sprintf(buffer, "CHAT %02d %02d %s\n", node->id, dest, mensagem);
    int n = send(new_socket_suc, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada ao nó: %02d!\n", dest);
}

