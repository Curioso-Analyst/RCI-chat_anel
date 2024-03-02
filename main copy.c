#include "camada_topologica.h"
#include "interface_utilizador.h"
#include "tcpcom.h"

// Variaveis Globais
Node* node = NULL; 
char* SERVER_IP;
char* PORT;
char* TCP_escolhido;


void print_help() {
    printf("Comandos disponíveis:\n");
    printf("  join (j) ring id - Entrada de um nó com identificador id no anel ring.\n");
    printf("  direct join (dj) id succid succIP succTCP - Entrada de um nó com identificador id diretamente num anel, sem consulta ao servidor de nós.\n");
    printf("  chord (c) - Estabelecimento de uma corda para um nó do anel que não o sucessor ou o predecessor. Cada nó estabelece no máximo uma corda.\n");
    printf("  remove chord (rc) - Eliminação da corda previamente estabelecida.\n"); 
    printf("  show topology (st) - Mostra a topologia atual.\n");
    printf("  show routing table (sr) dest - Mostra o caminho mais curto de um nó para o destino dest.\n");
    printf("  show forwarding (sf) - Mostra a tabela de expedição de um nó.\n");
    printf("  message (m) dest message - Envio da mensagem message ao nó dest.\n");
    printf("  leave (l) - Saída do nó do anel.\n");
    printf("  NODES ring - Um nó pede ao servidor de nós o identificador e respetivos contactos dos nós existentes no anel ring.\n");
    printf("  exit (x) - Fecho da aplicação.\n");
    // Adicionar mais comandos aqui
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        printf("Uso: %s IP TCP regIP regUDP\n", argv[0]);
        return 1;
    }

    char* IP = argv[1];
    TCP_escolhido = argv[2];
    SERVER_IP = argv[3];
    PORT = argv[4];

    printf("Bem-vindo ao programa COR! Digite 'help' para ver os comandos disponíveis.\n");

    
    int ring, id;
    char command[256];
    
    while (1) {
    printf("Insira um comando: ");
    fgets(command, 256, stdin);

    if (strncmp(command, "help", 4) == 0) {
        print_help();
    } else if (strncmp(command, "j", 1) == 0) {
        sscanf(command, "j %03d %02d", &ring, &id);
        node = join(ring, id, IP, TCP_escolhido); 
    } else if (strncmp(command, "l", 1) == 0) {
        sscanf(command, "l %d", &ring);
            if (node != NULL) {
                leave(node, ring);
            } else {
                printf("Nó não inicializado.\n");
            }
    } else if (strncmp(command, "dj", 2) == 0) {
        int id, succid;
        char succIP[16], succTCP[6];
        sscanf(command, "dj %d %d %s %s", &id, &succid, succIP, succTCP);
        node = direct_join(id, succid, succIP, succTCP);
    } else if (strncmp(command, "c", 1) == 0) {
        // Implementação do comando 'c'
    } else if (strncmp(command, "rc", 2) == 0) {
        // Implementação do comando 'rc'
    } else if (strncmp(command, "st", 2) == 0) {
        sscanf(command, "st");
        if (node != NULL) {
            show_topology(node);
            } else {
            printf("Nó não inicializado.\n");
            }
    } else if (strncmp(command, "sr", 2) == 0) {
        // Implementação do comando 'sr'
    } else if (strncmp(command, "sf", 2) == 0) {
        // Implementação do comando 'sf'
    } else if (strncmp(command, "m", 1) == 0) {
        // Implementação do comando 'm'
    } else if (strncmp(command, "NODES", 5) == 0) {
        sscanf(command, "NODES %03d", &ring);
        nodeslist(ring);
    } else if (strncmp(command, "x", 1) == 0) {
        printf("A fechar a aplicação...\n");
        return 0; // Termina o programa
    } else {
        printf("Comando desconhecido. Digite 'help' para ver os comandos disponíveis.\n");
    }
}


    return 0;
}
