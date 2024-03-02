#include "camada_topologica.h"
#include "interface_utilizador.h"
#include "camada_topologica_tcp.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h> // Para struct timeval
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>


#define MAX_CLIENTS 20

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

    int ring, id;
    char command[1024];
    int PORT = atoi(TCP_escolhido);
    int tcp_socket, udp_socket, addrlen, new_socket, client_socket[MAX_CLIENTS], activity, i, sd, max_sd; 
    struct sockaddr_in address;
    fd_set readfds, writefds;

    printf("Bem-vindo ao programa COR! Digite 'help' para ver os comandos disponíveis.\n");

    // Se o socket tcp ainda não foi criado, cria-o
    if (tcp_socket == 0) {
        // Cria um socket mestre
        if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Configura o socket mestre para permitir múltiplas conexões
        int opt = 1;
        if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // Tipo de socket criado
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        // Vincula o socket à porta
        if (bind(tcp_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        printf("Listening on port %d...\n", PORT);

        // Especifica o número máximo de conexões pendentes para o socket mestre
        if (listen(tcp_socket, 3) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // Aceita a conexão pendente
        addrlen = sizeof(address);
        puts("Waiting for connections ...");
    }

    // Cria um socket UDP
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Inicializa todos os client_socket[] para 0
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    while(1) {
        // Limpa o conjunto de sockets
        FD_ZERO(&readfds); 
        FD_ZERO(&writefds);

        // Adiciona o socket mestre ao conjunto
        FD_SET(tcp_socket, &readfds); // Adiciona o socket TCP ao conjunto
        FD_SET(udp_socket, &readfds); // Adiciona o socket UDP ao conjunto
        FD_SET(STDIN_FILENO, &readfds); // Adiciona a leitura do teclado ao conjunto
        max_sd = tcp_socket > udp_socket ? tcp_socket : udp_socket;
        max_sd = max_sd > STDIN_FILENO ? max_sd : STDIN_FILENO;

        // Adiciona sockets filhos ao conjunto
        for (i = 0; i < MAX_CLIENTS; i++) {
            // Descritor de socket
            sd = client_socket[i];

            // Se o descritor de socket é válido, adiciona à lista de leitura
            if (sd > 0)
                FD_SET(sd, &readfds);

            // O maior número de descritor de arquivo, necessário para a função select
            if (sd > max_sd)
                max_sd = sd;
        }

        // Espera por uma atividade em um dos sockets, o timeout é NULL, então espera indefinidamente
        activity = select(max_sd + 1, &readfds, &writefds, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }

        // Se algo aconteceu no teclado, então é uma entrada do utilizador
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
        // Lê a entrada do teclado
        fgets(command, sizeof(command), stdin);
        printf("Keyboard input: %s", command);

        if (strncmp(command, "l", 1) == 0) {
            sscanf(command, "l %d", &ring);
            if (node != NULL) {
                leave(node, ring);
            } else {
                printf("Nó não inicializado.\n");
            }
        } else if (strncmp(command, "help", 4) == 0) {
            print_help();
        } else if (strncmp(command, "j", 1) == 0) {
            sscanf(command, "j %03d %02d", &ring, &id);
            node = join(ring, id, IP, TCP_escolhido); 
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
            printf("Comando desconhecido. Digite 'help' para ver os comandos disponíveis.\n");}
        }

        // Se algo aconteceu no socket UDP, então é uma mensagem de entrada

        // Se algo aconteceu no socket TCP, então é uma conexão de entrada
        if (FD_ISSET(tcp_socket, &readfds)) {
            if ((new_socket = accept(tcp_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Nova conexão, socket fd é %d, ip é: %s, port: %d
            printf("Nova conexão, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Adiciona novo socket ao array de sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                // Se a posição estiver vazia
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Verifica todos os sockets para atividade de leitura
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            // Se o descritor de socket é válido e há dados disponíveis para leitura
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[1024];
                int valread;

                // Lê uma mensagem do socket
                if ((valread = recv(sd, buffer, sizeof(buffer), 0)) > 0) {
                    buffer[valread] = '\0';
                    printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida

                    // Verifica se a mensagem é "READY"
                    if (strcmp(buffer, "READY") == 0) {
                        printf("Cliente está pronto para enviar a mensagem ENTRY.\n");
                    }

                    // Verifica se é uma mensagem de entrada
                    else if (strncmp(buffer, "ENTRY", 5) == 0) {
                        int new_id;
                        char new_ip[16];
                        int new_port;

                        // Analisa a mensagem ENTRY
                        sscanf(buffer, "ENTRY %d %s %d", &new_id, new_ip, &new_port);

                        // Imprime as informações do novo nó
                        printf("Informações do novo nó: id=%02d, ip=%s, port=%d\n", new_id, new_ip, new_port);  

                        // Verifica se o nó tem um segundo sucessor
                        if (node->second_successor != NULL) {
                            // Envia uma mensagem a informar ao novo nó do seu segundo-sucessor
                            send_succ(new_socket, node->second_successor);
                            printf("Mensagem enviada ao novo nó.\n");
                        }

                        // Informar o predecessor da entrada do novo nó
                        if (node->predecessor != NULL) {
                            // Envia uma mensagem a informar ao predecessor do novo nó
                            send_entry(new_socket, node->predecessor);
                            printf("Mensagem enviada ao predecessor do nó atual.\n");
                        }


                        // Cria um novo nó para o predecessor
                        char new_port_str[6];
                        sprintf(new_port_str, "%d", new_port);
                        node->predecessor = createNode(new_id, new_ip, new_port_str);
                        printf("Informações do nó predecessor atualizadas: id=%d, ip=%s, port=%s\n", node->predecessor->id, node->predecessor->ip, node->predecessor->tcp);
                        }


                        if (valread == 0) {
                        printf("Cliente desconectado.\n");
                        client_socket[i] = 0;
                        } else if (valread == -1) {
                        perror("recv");
                        }
                    }
                }
            }
        }
    close(udp_socket);
    close(tcp_socket);
    return 0;
}
    


    
    /*while (1) {
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
}*/
