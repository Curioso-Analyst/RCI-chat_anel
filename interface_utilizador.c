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
    int status = registerNode(node, ring, IP, TCP, user_input);

    // Se o registo falhar, liberta a memória alocada para o nó e retorna NULL
    if (status == -1) {
        printf("Erro ao registar o nó\n");
        sprintf(user_input, "UNREG %03d %02d", ring, node->id);
        unregisterNode(node, user_input);
        free(node);
        return NULL;
    }
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
        int porta_tcp = cliente_tcp(node, succIP, succTCP);
        if (DEBUG) {
        printf("Olá cliente, o meu fd é: %d\n", porta_tcp);
        }

        send_entry(porta_tcp, node);

        node->sucessor=createNode(succId, succIP, succTCP);

        // Lê uma mensagem do socket
        char buffer[1024];
        int valread;
        if ((valread = recv(porta_tcp, buffer, sizeof(buffer), 0)) > 0) {
            buffer[valread] = '\0';
            if (DEBUG) {
                printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida
            }
                
            // Verifica se é uma mensagem de entrada
            if (strncmp(buffer, "SUCC", 4) == 0) {
                    int new_id;
                    char new_ip[16];
                    char new_port[6];
                    
                // Analisa a mensagem SUCC
                sscanf(buffer, "SUCC %d %s %s", &new_id, new_ip, new_port);

                node->second_successor=createNode(new_id, new_ip, new_port);

                global_variable=porta_tcp;
                                            
                // Imprime as informações do novo nó
                if (DEBUG) {
                    printf("Informações do segundo sucessor: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);
                }
                printf("\n---Nó conectado ao escolhido com sucesso!---\n");	
            }
        }
    }
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
        printf("\nCorda à qual me conectei:\n");
        printf("Identificador: %02d\n", node->corda->id);
        printf("IP: %s\n", node->corda->ip);
        printf("TCP: %s\n", node->corda->tcp);
    } else {
        printf("\nCorda à qual me conectei: NULL\n");
    }

    if (node->predecessor != NULL) {
        printf("\nPredecessor:\n");
        printf("Identificador: %02d\n", node->predecessor->id);
        printf("IP: %s\n", node->predecessor->ip);
        printf("TCP: %s\n", node->predecessor->tcp);
    } else {
        printf("\nPredecessor: NULL\n");
    }

    printf("\nLista de cordas recebidas:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            printf("\nCorda %d:\n", i);
            printf("Identificador: %02d\n", clients[i]->node->id);
            printf("IP: %s\n", clients[i]->node->ip);
            printf("TCP: %s\n", clients[i]->node->tcp);
        }
    }

}

// Implementar as outras funções aqui
