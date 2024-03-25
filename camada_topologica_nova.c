#include "camada_topologica.h"
#include "camada_topologica_tcp.h"
#include "camada_encaminhamento.h"

int global_variable=-1;

// Função para criar um nó
Node* createNode(int id, char* ip, char* tcp) {
    Node* node = (Node*) malloc(sizeof(Node));
    node->id = id;
    strcpy(node->ip, ip);
    strcpy(node->tcp, tcp);
    node->pred_socket_fd = -1; // Inicializa o socket do predecessor como -1
    node->suc_socket_fd = -1; // Inicializa o socket do sucessor como -1
    node->sucessor = node; // O nó é seu próprio sucessor, criando um anel com apenas um nó
    node->predecessor = node; // O nó é seu próprio predecessor, criando um anel com apenas um nó
    node->second_successor = node;
    node->corda = NULL;
    node->corda_socket_fd = -1; // Incializa o socket da corda como -1
    return node;
}

// Função para obter um identificador único
int getUniqueIdentifier(char* nodes_list) {
    int id;
    // Array para armazenar os números inteiros encontrados
    int numbers[100]={-1};

    // Ponteiro para o caractere atual 
    char *p = nodes_list;

    // Ponteiro para o próximo caractere a seguir '\n'
    char *nextline;
  
    // Variável temporária para guardar o número lido 
    int temp;

    // Processa a string de entrada
    while ((nextline = strchr(p, '\n')) != NULL) {
        // Tenta ler um number de seguinte segmento
        if( sscanf(nextline, "\n%d", &temp) == 1 ) {
            // Adicione o número ao array
            numbers[temp] = temp;
        }
        //Move o ponteiro para a próxima linha
        p = nextline + 1;
    }

    // Imprime os números obtidos
    for (int i = 0; i < 100; i++) {
        if(numbers[i]!=i){
            id=i;
            break;
        }
    }
    return id;
}

// Função para registrar o nó no servidor
int registerNode(Node* node, int ring, char* IP, char* TCP, char* user_input) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char nodes_list[1024] = {0};  // Inicializa nodes_list com 0

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if (fd==-1) {
        perror("Erro ao criar o socket");
        return -1;  // Retorna em vez de sair
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;  //UDP socket

    errcode=getaddrinfo(SERVER_IP, PORT, &hints, &res);
    if(errcode!=0) {
        fprintf(stderr, "Erro ao obter informações de endereço: %s\n", gai_strerror(errcode));
        return -1;  // Retorna em vez de sair
    }

    // Define um tempo limite para recepção
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao definir o tempo limite do socket");
        return -1;  // Retorna em vez de sair
    }

    // Pede a lista de nós
    char nodes_command[1024];
    sprintf(nodes_command, "NODES %03d", ring);

    int tries = 0;
    while (tries < MAX_TRIES) {
        // Envia a mensagem de pedido da lista de nós
        sendto(fd, nodes_command, strlen(nodes_command), 0, res->ai_addr, res->ai_addrlen);

        // Recebe a lista de nós
        n=recvfrom(fd, nodes_list, 1024, 0, (struct sockaddr*) &addr, &addrlen);
        if(n==-1) {
            // Timeout, reenvia a mensagem
            printf("Temporizador expirou! Reenviando mensagem...\n");
            tries++;
        } else {
            // Confirmação recebida, sai do loop
            if (DEBUG) {
            printf("Mensagem confirmada!\n");
            }
            break;
        }
    }

    if (tries == MAX_TRIES) {
        printf("Mensagem não confirmada após %d tentativas.\n", MAX_TRIES);
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    int j_id;
    char j_port[10];
    char j_ip[128];
    char indiferente[30];
    char indi[10];
    //Escolhe o primeiro elemento da Nodes List como futuro sucessor
    int ret = sscanf(nodes_list, "%s %s %d %s %s",indiferente, indi, &j_id, j_ip, j_port); 

    if (ret < 3) {
        // não há elementos na lista
        // Envia a mensagem de registro ao servidor por UDP
        regservidornos(node, fd, user_input,nodes_list, res, &addr, ring, IP, TCP);

    } else {
        //há elementos na lista

        // Envia a mensagem de registro ao servidor por UDP para verficar se o id já está em uso
        regservidornos(node, fd, user_input,nodes_list, res, &addr, ring, IP, TCP);

        // Conecta-se ao nó que já está no servidor e grava o socket de comunicação em porta_tcp
        int porta_tcp = cliente_tcp(node, j_ip, j_port);
        if (porta_tcp == -1) {
            printf("Falha ao conectar-se ao nó.\n");
            return -1;
        }
        send_entry(porta_tcp, node);

        node->sucessor=createNode(j_id, j_ip, j_port);

        // Lê uma mensagem do socket
        char buffer[1024];
        char buffer1[1024];
        int valread;
        if ((valread = recv(porta_tcp, buffer, sizeof(buffer), 0)) > 0) {
            buffer[valread] = '\0';
            // Verifica se é uma mensagem de entrada
            if (strncmp(buffer, "SUCC", 4) == 0) {
                    int new_id;
                    char new_ip[16];
                    char new_port[6];
                    
                // Analisa a mensagem SUCC
                sscanf(buffer, "SUCC %d %s %s", &new_id, new_ip, new_port);

                node->second_successor=createNode(new_id, new_ip, new_port);

                global_variable=porta_tcp;

                sprintf(buffer1, "ROUTE %d %d %d\n", node->id, node->id, node->id);
                send_route(porta_tcp,buffer1);
                                            
                // Imprime as informações do novo nó
                if (DEBUG) {
                    printf("Informações do segundo sucessor: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);
                }
            }
        }
    }

    freeaddrinfo(res);
    close(fd);
    return 0;
}

// Função para registrar o nó no servidor com único identificador
void regservidornos(Node* node,int fd, char* user_input, char* nodes_list, struct addrinfo* server_info, struct sockaddr_in* addr, int ring, char* IP, char* TCP) {
    ssize_t n;
    socklen_t addrlen;
    char buffer[128];

    char message[1024];
    sprintf(message, "%s", user_input);

    
    n=sendto(fd, message, strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (strncmp(buffer, "ERROR - node id not available", 29) == 0) {
        // Escolhe um novo identificador que não esteja na lista
        int new_id = getUniqueIdentifier(nodes_list);
        printf("O identificador fornecido já estava em uso. Um novo identificador único, %02d, foi escolhido.\n", new_id);
        node->id = new_id;

        // Tenta registrar novamente com o novo identificador
        sprintf(user_input, "REG %03d %02d %s %s", ring, new_id, IP, TCP); 
        regservidornos(node, fd, user_input,nodes_list, server_info, addr, ring, IP, TCP);
    } else {
        // Escreve no ecra a resposta do servidor
        write(1, "Resposta do servidor de nós: ", strlen("Resposta do servidor de nós: "));
        write(1, buffer, n);
        write(1, "\n", 1);
    }
}

// Função para retirar o nó do servidor
void unregisterNode(Node* node, char* user_input) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if (fd==-1) /*error*/ exit (1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;  //UDP socket
    errcode=getaddrinfo (SERVER_IP, PORT, &hints, &res);
    if(errcode!=0) /*error*/ exit(1);

    // Envia a mensagem de desregisto
    char message[1024];
    sprintf(message, "%s", user_input);
   
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("sendto"); // Exibe mensagem de erro se houver
        exit(1);
    }

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);
    

    // Escreve no ecra a resposta do servidor
    write(1, "Resposta do servidor: ", 22); write(1,buffer,n); write(1, "\n", 1);

    freeaddrinfo(res); //libertar a memoria alocada
    close(fd); //fechar o socket
}

// Função para obter a lista de nós
void getNodes(int ring, char* user_input) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[1024];

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if (fd==-1) /*error*/ exit (1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;  //UDP socket
    errcode=getaddrinfo (SERVER_IP, PORT, &hints, &res);
    if(errcode!=0) /*error*/ exit(1);

    // Envia a mensagem de pedido da lista de nós
    char message[1024];
    sprintf(message, "%s", user_input);

    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,1024,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    // Escreve no ecra a resposta do servidor
    write(1, "Resposta do servidor: ", 22); write(1,buffer,n); write(1, "\n", 1);

    freeaddrinfo(res); //libertar a memoria alocada
    close(fd); //fechar o socket
}

// Função para obter a lista para os nós
void getNodescorda(Node* node, char* buffer) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if (fd==-1) /*error*/ exit (1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;  //UDP socket
    errcode=getaddrinfo (SERVER_IP, PORT, &hints, &res);
    if(errcode!=0) /*error*/ exit(1);

    // Envia a mensagem de pedido da lista de nós
    char message[1024];
    sprintf(message, "NODES %03d", node->ring);
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,1024,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);
    freeaddrinfo(res); //libertar a memoria alocada
    close(fd); //fechar o socket
}

// Função para criar uma corda
void establishChord(Node* node) {
    char buffer[1024];
    getNodescorda(node, buffer);

    char* saveptr;
    char* line = strtok_r(buffer, "\n", &saveptr);
    Node* other_node = NULL;
    while (line != NULL) {
        char* id_str = strtok(line, " ");
        char* ip = strtok(NULL, " ");
        char* tcp = strtok(NULL, " ");

        if (id_str == NULL || ip == NULL || tcp == NULL) {
            // Se qualquer um dos tokens for NULL, pula para a próxima linha
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        int id = atoi(id_str);  // Converte a string do ID para um inteiro

        // Verifica se o nó já está na lista de clientes antes de tentar estabelecer uma conexão(evita conexões duplicadas)
        bool already_connected = false;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i]->node->id == id) {
                already_connected = true;
                break;
            }
        }

        // Se o nó já está na lista de clientes, pula para o próximo nó
        if (!already_connected && id != node->sucessor->id && id != node->predecessor->id && id != node->id) {
            other_node = createNode(id, ip, tcp);
            int porta_tcp = cliente_tcp(other_node, ip, tcp);
            if (porta_tcp != -1) {
                if (DEBUG) {
                printf("Olá cliente, o meu fd é: %d\n", porta_tcp);
                }
                send_chord(porta_tcp, node);
                printf("Corda estabelecida com sucesso com o nó %02d!\n", id);

                node->corda = other_node;
                node->corda->corda_socket_fd = porta_tcp;
                break;
            } else {
                printf("Falha ao conectar ao servidor.\n");
                free(other_node);
            }
        }

        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (other_node == NULL) {
        printf("Não foram encontrados nós adequados para estabelecer uma corda.\n");
    }
}

// Função para remover uma corda (cliente)
void removeChord(Node* node) {
    if (node->corda != NULL) {
        // Fecha o socket
        close(node->corda->corda_socket_fd);
        
        // Libera a memória do nó da corda
        free(node->corda);
        node->corda = NULL;
        printf("Corda removida com sucesso.\n");
    } else {
        printf("Nenhuma corda para remover.\n");
    }
}

// Função para adicionar um cliente à lista(servidor cordas)
void add_client(int socket_fd, Node* node) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = (ClientInfo*) malloc(sizeof(ClientInfo));
            clients[i]->socket_fd = socket_fd;
            clients[i]->node = node;
            return;
        }
    }
}

// Função para remover um cliente da lista(servidor cordas)
void remove_client(int socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket_fd == socket_fd) {
            free(clients[i]);
            clients[i] = NULL;
            return;
        }
    }
}