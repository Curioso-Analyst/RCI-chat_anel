#include "camada_topologica.h"
#include "camada_topologica_tcp.h"
#include "camada_encaminhamento.h"

int global_variable=-1;

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
    return node;
}


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

void registerNode(Node* node, int ring, char* IP, char* TCP, char* user_input) {
    int fd,errcode;
    //ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    //char buffer[128];

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if (fd==-1) /*error*/ exit (1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;  //UDP socket

    errcode=getaddrinfo(SERVER_IP, PORT, &hints, &res);
    if(errcode!=0) /*error*/ exit(1);

    // Pede a lista de nós
    char nodes_command[1024];
    sprintf(nodes_command, "NODES %03d", ring);
    sendto(fd, nodes_command, strlen(nodes_command), 0, res->ai_addr, res->ai_addrlen);

    // Recebe a lista de nós
    char nodes_list[1024] = {0};  // Inicializa nodes_list
    recvfrom(fd, nodes_list, 1024, 0, (struct sockaddr*) &addr, &addrlen);

    int j_id;
    char j_port[10];
    char j_ip[128];
    char indiferente[30];
    char indi[10];
    printf("%s\n",nodes_list);

    //Escolhe o primeiro elemento da Nodes List como futuro sucessor
    int ret = sscanf(nodes_list, "%s %s %d %s %s",indiferente, indi, &j_id, j_ip, j_port); 

    if (ret < 3) {
        // não há elementos na lista
        // Envia a mensagem de registro

        // Call the new function to send and receive messages
        regservidornos(node, fd, user_input,nodes_list, res, &addr, ring, IP, TCP);

        
    } else {
        //há elementos na lista

        // Conecta-se ao nó que já está no servidor e grava o socket de comunicação em porta_tcp
        int porta_tcp = cliente_tcp(node, j_ip, j_port);
        printf("Olá cliente, o meu fd é: %d\n", porta_tcp);
        send_entry(porta_tcp, node);

        node->sucessor=createNode(j_id, j_ip, j_port);

        // Lê uma mensagem do socket
        char buffer[1024];
        char buffer1[1024];
        int valread;
        if ((valread = recv(porta_tcp, buffer, sizeof(buffer), 0)) > 0) {
            buffer[valread] = '\0';
            printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida
                
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
                printf("Informações do segundo sucessor: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);

                sprintf(buffer1, "ROUTE %d %d %d\n", node->id, node->id, node->id);
                send_route(porta_tcp,buffer1);

                //Depois passa para o main, parte servidor, para aceitar a nova conexão e receber o PRED.
            }
        }

        // Envia a mensagem de registro ao servidor por UDP
        regservidornos(node, fd, user_input,nodes_list, res, &addr, ring, IP, TCP);
    }

    freeaddrinfo(res);
    close(fd);
}

void regservidornos(Node* node,int fd, char* user_input, char* nodes_list, struct addrinfo* server_info, struct sockaddr_in* addr, int ring, char* IP, char* TCP) {
    ssize_t n;
    socklen_t addrlen;
    char buffer[128];

    char message[1024];
    sprintf(message, "%s", user_input);

    // Imprime a mensagem que será enviada
    printf("Sending message: %s\n", message);
    fflush(stdout); // Força a liberação do fluxo de saída padrão

    n=sendto(fd, message, strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    //printf("Resposta do servidor: %s\n", buffer);

    if (strncmp(buffer, "ERROR - node id not available", 29) == 0) {
        // Escolhe um novo identificador que não esteja na lista
        int new_id = getUniqueIdentifier(nodes_list);
        printf("O identificador fornecido já estava em uso. Um novo identificador único, %d, foi escolhido.\n", new_id);
        node->id = new_id;

        // Tenta registrar novamente com o novo identificador
        sprintf(user_input, "REG %03d %02d %s %s", ring, new_id, IP, TCP);
        printf("Tentando registrar novamente com id: %d\n", new_id); 
        regservidornos(node, fd, user_input,nodes_list, server_info, addr, ring, IP, TCP);
    } else {
        // Escreve no ecra a resposta do servidor
        write(1, "Resposta do servidor: ", 22); write(1,buffer,n); write(1, "\n", 1);
    }
}

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

    // Imprime a mensagem que será enviada
    printf("Sending message: %s\n", message);
    fflush(stdout); // Força a liberação do fluxo de saída padrão

    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    // Recebe a resposta
    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*) &addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    // Escreve no ecra a resposta do servidor
    write(1, "Resposta do servidor: ", 22); write(1,buffer,n); write(1, "\n", 1);

    freeaddrinfo(res); //libertar a memoria alocada
    close(fd); //fechar o socket
}

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

    // Imprime a mensagem que será enviada
    printf("Sending message: %s\n", message);
    fflush(stdout); // Força a liberação do fluxo de saída padrão

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