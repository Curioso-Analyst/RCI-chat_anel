// interface_utilizador.c
#include "interface_utilizador.h"
#include "camada_topologica.h"
#include "camada_topologica_tcp.h"


Node* join(int ring, int id, char* IP, char* TCP) {
    // Cria um novo nó
    Node* node = createNode(id, IP, TCP);

    // Regista o nó
    char user_input[1024];
    sprintf(user_input, "REG %03d %02d %s %s", ring, id, IP, TCP);
    registerNode(node, ring, IP, TCP, user_input);

    return node;
}

Node* direct_join(int id, int succId, char* succIP, char* succTCP) {
    // Cria um novo nó
    Node* node = createNode(id, succIP, TCP_escolhido);

    // Se o id for igual ao succId, então cria-se um anel com um só nó
    if (id == succId) {
        node->sucessor = node;
        node->predecessor = node;
    } else {
         // Conecta-se ao nó sucessor e informa-o sobre a entrada do novo nó no anel
         cliente_tcp(node, succIP, succTCP);

    }

    // Iniciar o servidor/cliente TCP na porta escolhida pelo usuário
    // serverclient(node, TCP_escolhido);

    return node;
}

void leave(Node* node, int ring) {
    // Desregistra o nó
    char user_input[1024];
    sprintf(user_input, "UNREG %03d %02d", ring, node->id);
    unregisterNode(node, user_input);
}

void nodeslist(int ring) {
    // Pede ao servidor de nós o identificador e respetivos contactos dos nós existentes no anel ring
    char user_input[1024];
    sprintf(user_input, "NODES %03d", ring);
    getNodes(ring, user_input);
}

void show_topology(Node* node) {
    printf("Nó atual:\n");
    printf("Identificador: %02d\n", node->id);
    printf("IP: %s\n", node->ip);
    printf("TCP: %s\n", node->tcp);

    if (node->sucessor != NULL) {
        printf("\nSucessor:\n");
        printf("Identificador: %02d\n", node->sucessor->id);
        printf("IP: %s\n", node->sucessor->ip);
        printf("TCP: %s\n", node->sucessor->tcp);
    } else {
        printf("\nSucessor: NULL\n");
    }

    if (node->second_successor != NULL) {
        printf("\nSegundo sucessor:\n");
        printf("Identificador: %02d\n", node->second_successor->id);
        printf("IP: %s\n", node->second_successor->ip);
        printf("TCP: %s\n", node->second_successor->tcp);
    } else {
        printf("\nSegundo sucessor: NULL\n");
    }

    if (node->corda != NULL) {
        printf("\nVizinho na corda:\n");
        printf("Identificador: %02d\n", node->corda->id);
        printf("IP: %s\n", node->corda->ip);
        printf("TCP: %s\n", node->corda->tcp);
    } else {
        printf("\nVizinho na corda: NULL\n");
    }

    if (node->predecessor != NULL) {
        printf("\nPredecessor:\n");
        printf("Identificador: %02d\n", node->predecessor->id);
        printf("IP: %s\n", node->predecessor->ip);
        printf("TCP: %s\n", node->predecessor->tcp);
    } else {
        printf("\nPredecessor: NULL\n");
    }
}

// Implementar as outras funções aqui
