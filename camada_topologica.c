#include "camada_topologica.h"

Node* createNode(int id, char* ip, char* tcp) {
    Node* node = (Node*) malloc(sizeof(Node));
    node->id = id;
    strcpy(node->ip, ip);
    strcpy(node->tcp, tcp);
    node->sucessor = node; // O nó é seu próprio sucessor, criando um anel com apenas um nó
    node->predecessor = node; // O nó é seu próprio predecessor, criando um anel com apenas um nó
    node->second_successor = NULL;
    node->corda = NULL;
    return node;
}


int getUniqueIdentifier(char* nodes_list) {
    int id;

    // Cria um array para rastrear quais identificadores estão em uso
    int ids_in_use[100] = {0};

    // Cria uma cópia de nodes_list para evitar modificar a string original
    char* nodes_list_copy = strdup(nodes_list);
    if (nodes_list_copy == NULL) {
        // Não foi possível alocar memória para a cópia, retorna erro
        return -1;
    }

    // Marca os identificadores na nodes_list como em uso
    char* token = strtok(nodes_list_copy, " ");
    while (token != NULL) {
        char* endptr;
        long id_in_use = strtol(token, &endptr, 10);
        if (*endptr == '\0' && id_in_use >= 0 && id_in_use < 100) {
            // A conversão foi bem-sucedida e o valor está dentro do intervalo esperado
            ids_in_use[id_in_use] = 1;
        }
        token = strtok(NULL, " ");
    }

    free(nodes_list_copy);

    // Inicializa o gerador de números aleatórios
    srand(time(NULL));

    // Procura um identificador aleatório que não esteja em uso
    do {
        id = rand() % 100;
    } while (ids_in_use[id] == 1);

    // Retorna o identificador aleatório não utilizado
    return id;
}

void registerNode(Node* node, int ring, char* IP, char* TCP, char* user_input) {
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

    // Envia a mensagem de registro
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

    // printf("Resposta do servidor: %s\n", buffer);

    // Verifica se o identificador não está disponível
    if (strncmp(buffer, "ERROR - node id not available", 29) == 0) {
        // Se o identificador não estiver disponível, pede a lista de nós
        char nodes_command[1024];
        sprintf(nodes_command, "NODES %03d", ring);
        sendto(fd, nodes_command, strlen(nodes_command), 0, res->ai_addr, res->ai_addrlen);

        // Recebe a lista de nós
        char nodes_list[1024] = {0};  // Inicializa nodes_list
        recvfrom(fd, nodes_list, 1024, 0, (struct sockaddr*) &addr, &addrlen);

        // Escolhe um novo identificador que não esteja na lista
        int new_id = getUniqueIdentifier(nodes_list);
        printf("O identificador fornecido já estava em uso. Um novo identificador único, %02d, foi escolhido.\n", new_id);
        node->id = new_id;

        // Tenta registrar novamente com o novo identificador
        sprintf(user_input, "REG %03d %02d %s %s", ring, new_id, IP, TCP);
        printf("Tentando registrar novamente com id: %02d\n", new_id); 
        registerNode(node, ring, IP, TCP, user_input);
    } else {
        // Escreve no ecra a resposta do servidor
        write(1, "Resposta do servidor: ", 22); write(1,buffer,n); write(1, "\n", 1);
    }


    freeaddrinfo(res);
    close(fd);
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