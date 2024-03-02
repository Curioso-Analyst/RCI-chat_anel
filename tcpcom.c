#include "tcpcom.h"

int serverclient(Node* node, char* TCP) {
    int PORT = atoi(TCP);
    int opt = 1;
    int master_socket;
    struct sockaddr_in address;
    int addrlen, new_socket, client_socket[MAX_CLIENTS], activity, i, sd, max_sd;
    int ring;
    

    // Conjunto de descritores de socket
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;

    // Inicializa todos os client_socket[] para 0, para não verificar
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Cria um socket mestre
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configura o socket mestre para permitir múltiplas conexões
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Tipo de socket criado
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincula o socket à porta recebida no main
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d...\n", PORT);

    // Especifica o número máximo de 3 conexões pendentes para o socket mestre
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Aceita a conexão pendente
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (1) {
        // Limpa o conjunto de sockets
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        // Adiciona o socket mestre ao conjunto
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

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

        // Adiciona o descritor de arquivo de entrada padrão (teclado) ao conjunto
        FD_SET(STDIN_FILENO, &readfds);
        if (STDIN_FILENO > max_sd)
            max_sd = STDIN_FILENO;


        /* Define o timeout para 3 segundos
        struct timeval timeout;
        timeout.tv_sec = 6;
        timeout.tv_usec = 0;*/

        // Espera por uma atividade em um dos sockets, o timeout é NULL, então espera indefinidamente
        activity = select(max_sd + 1, &readfds, &writefds, &exceptfds, NULL);

        if (activity == 0) {
        printf("select timeout\n");
        } else if (activity < 0 && errno != EINTR) {
        printf("select error");
        }

        // Verifica se há entrada do teclado
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
        // Lê a entrada do teclado
        char command[1024];
        fgets(command, sizeof(command), stdin);
        printf("Keyboard input: %s", command);

        if (strncmp(command, "l", 1) == 0) {
            sscanf(command, "l %d", &ring);
            if (node != NULL) {
                leave(node, ring);
            } else {
                printf("Nó não inicializado.\n");
                }
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
    
        // Se algo aconteceu no socket mestre, então é uma conexão de entrada
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Nova conexão, socket fd é %d, ip é: %s, port: %d
            printf("New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

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
                        sleep(2);  // Adiciona um atraso antes de enviar a mensagem "ENTRY"
                    }
                    // Verifica se é uma mensagem de entrada
                    else if (strncmp(buffer, "ENTRY", 5) == 0) {
                        int new_id;
                        char new_ip[16];
                        int new_port;

                        // Analisa a mensagem ENTRY
                        sscanf(buffer, "ENTRY %d %s %d", &new_id, new_ip, &new_port);

                        printf("Informações do novo nó: id=%02d, ip=%s, port=%d\n", new_id, new_ip, new_port);  // Imprime as informações do novo nó

                        // Cria um novo nó para o predecessor
                        char new_port_str[6];
                        sprintf(new_port_str, "%d", new_port);
                        node->predecessor = createNode(new_id, new_ip, new_port_str);
                        


                        printf("Informações do nó predecessor atualizadas: id=%d, ip=%s, port=%s\n", node->predecessor->id, node->predecessor->ip, node->predecessor->tcp);  // Imprime as informações do nó predecessor atualizadas
                    }

                }

                if (valread == 0) {
                    printf("Cliente desconectado.\n");
                    client_socket[i] = 0;
                } else if (valread == -1) {
                    perror("recv");
                }
            }
        }
    } //while(1)
    
    return 0;   
}