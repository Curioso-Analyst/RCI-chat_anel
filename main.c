#include "camada_topologica.h"
#include "interface_utilizador.h"
#include "camada_topologica_tcp.h"  
#include "camada_encaminhamento.h"
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

void setup_master_socket(int *tcp_socket, int PORT) {
    struct sockaddr_in address;
    int opt = 1;

    // Cria um socket mestre
    if ((*tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configura o socket mestre para permitir múltiplas conexões
    if (setsockopt(*tcp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Tipo de socket criado
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincula o socket à porta
    if (bind(*tcp_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d...\n", PORT);

    // Especifica o número máximo de conexões pendentes para o socket mestre
    if (listen(*tcp_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    puts("Waiting for connections ...");
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        printf("Uso: %s IP TCP regIP regUDP\n", argv[0]);
        return 1;
    }

    // Variáveis para armazenar os argumentos
    char* IP = argv[1];
    TCP_escolhido = argv[2];
    SERVER_IP = argv[3];
    PORT = argv[4];

    // Variáveis uteis para o programa
    int ring, id;
    char command[1024];
    int PORT = atoi(TCP_escolhido);
    int tcp_socket, addrlen, activity, max_sd, new_socket_pred, new_socket_suc; 
    int temos_pred = -1, temos_suc =-1, pred_saiu =-1;
    int new_socket = -1;
    struct sockaddr_in address;
    fd_set readfds, writefds;
    int numero;

    char tabela_encaminhamento[101][101][55], tabela_curtos[101][2][55], tabela_expedicao[101][2][5];

    char mensagens_guardadas[20][512];
    int k;
    for (k =0; k<20;k++){
        strcpy(mensagens_guardadas[k],"-1");
    }

    printf("Bem-vindo ao programa COR! Digite 'help' para ver os comandos disponíveis.\n");

    // Configura o socket mestre e inicia o servidor
    setup_master_socket(&tcp_socket, PORT);

    cria_tabelas(tabela_encaminhamento,tabela_curtos,tabela_expedicao);

    // Começa o ciclo principal onde tem o select
    while(1) {
        if(global_variable!=-1){
            new_socket_suc=global_variable;
            global_variable=-1;
            temos_suc=1;
        }
        // Limpa o conjunto de sockets
        FD_ZERO(&readfds); 
        FD_ZERO(&writefds);

        // Adiciona o socket mestre ao conjunto
        FD_SET(tcp_socket, &readfds); // Adiciona o socket TCP ao conjunto
        FD_SET(STDIN_FILENO, &readfds); // Adiciona a leitura do teclado ao conjunto


        if (temos_pred==1){
            FD_SET(new_socket_pred, &readfds);  // Adiciona o socket do predecessor ao conjunto
        }

        if (temos_suc==1){
            FD_SET(new_socket_suc, &readfds);  // Adiciona o socket do sucessor ao conjunto
        }

        max_sd = tcp_socket;

        // Compare STDIN_FILENO with max_sd and assigns larger value to max_sd
        if(STDIN_FILENO > max_sd)
            max_sd = STDIN_FILENO; // STDIN_FILENO is greater

        // Compare new_socket_predecessor with max_sd and assigns larger value to max_sd
        if(new_socket_pred > max_sd)
            max_sd = new_socket_pred;

        if(new_socket_suc > max_sd)
            max_sd = new_socket_suc;

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
                close(new_socket_pred);
                close(new_socket_suc);
                new_socket_pred=-1;
                new_socket_suc=-1;
                temos_pred=-1;
                temos_suc=-1;
            } else {
                printf("Nó não inicializado.\n");
            }
        } else if (strncmp(command, "help", 4) == 0) {
            print_help();
        } else if (strncmp(command, "j", 1) == 0) {
            sscanf(command, "j %03d %02d", &ring, &id);
            node = join(ring, id, IP, TCP_escolhido);
            char nodeid[3];
            sprintf(nodeid, "%d", node->id);
            strcpy(tabela_curtos[node->id][0],nodeid);
            strcpy(tabela_curtos[node->id][1],nodeid);
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
            sscanf(command, "sr %d", &id);
            imprimir_encaminhamento(id,tabela_encaminhamento);

        } else if (strncmp(command, "sp", 2) == 0) {
            sscanf(command, "sp %d", &id);
            printf("O caminho para %d é: %s\n",id,tabela_curtos[id][1]);
        } else if (strncmp(command, "sf", 2) == 0) {
            // Mostra a tabela de expedicao
            imprimir_expedicao(tabela_expedicao);
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

        int new_socket_temp = -1; // Variável temporária para armazenar o novo socket aceite
        
        // Verifica se há atividade no socket de escuta
        if (FD_ISSET(tcp_socket, &readfds)) {
        addrlen = sizeof(address);

            if ((new_socket_temp = accept(tcp_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Nova conexão, socket fd é %d, ip é: %s, port: %d
            printf("Olá servidor, o meu socket fd é %d, ip is : %s, port : %d\n", new_socket_temp, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Atualiza new_socket após todas as verificações do loop for
            if (new_socket_temp != -1) {
            new_socket = new_socket_temp;
            printf("Valor de new_socket atualizado para: %d\n", new_socket);
            }

            // Conversão de inteiro para char
            char socket_char[20]; // Tamanho suficiente para armazenar um número inteiro
            sprintf(socket_char, "%d", new_socket_temp);
            char port_char[20]; // Tamanho suficiente para armazenar um número inteiro
            sprintf(port_char, "%d", ntohs(address.sin_port));

            // Lê uma mensagem do socket
            char buffer[1024];
            int valread;
            if ((valread = recv(new_socket, buffer, sizeof(buffer), 0)) > 0) {
                buffer[valread] = '\0';
                printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida

                // Verifica se é uma mensagem de entrada
                if (strncmp(buffer, "ENTRY", 5) == 0) {
                    int new_id;
                    char new_ip[16];
                    char new_port[6];
                    
                    // Analisa a mensagem ENTRY
                    sscanf(buffer, "ENTRY %d %s %s", &new_id, new_ip, new_port);
                                            
                    // Imprime as informações do novo nó
                    printf("Informações de um novo cliente: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);

                    //Se está a entrar o segundo nó
                    if(node->sucessor->id==node->id){

                        printf("----ESTAVA SOZINHO---");

                        node->predecessor = createNode(new_id, new_ip, new_port);
                        node->sucessor = createNode(new_id, new_ip, new_port);

                        // Envia uma mensagem a informar ao novo nó do seu segundo-sucessor
                        send_succ(new_socket, node->sucessor);
                        // Define como a socket com o predecessor a new_socket_tcp
                        new_socket_pred = new_socket;

                        // Conexão TCP com o novo nó de entrada na porta dele
                        int new_socket_tcp = cliente_tcp(node, new_ip, new_port);

                        // Envia uma mensagem PRED para a nova porta tcp estabelecida
                        send_pred(new_socket_tcp, node);

                        // Atualizar o show topology
                        //node->sucessor = createNode(new_id, new_ip, new_port);

                        // Define como a socket com o predecessor a new_socket_tcp
                        new_socket_suc = new_socket_tcp;

                        temos_pred=1;
                        temos_suc=1;

                    }else if(node->id==node->second_successor->id){

                        //Atualiza as tabelas
                        //elimina_vizinho(new_socket_suc, node->id,node->predecessor, tabela_encaminhamento,tabela_curtos,tabela_expedicao);

                        // Envia uma mensagem a informar ao novo nó do seu segundo-sucesso

                        send_succ(new_socket, node->sucessor);

                        //Atualiza o st
                        node->predecessor = createNode(new_id, new_ip, new_port);

                        //Envia para o antigo predecessor o Entry
                        send_entry(new_socket_pred, node->predecessor);

                        acumula_routes(new_socket, node, tabela_curtos);

                        //Atualiza a nova socket com o predecessor
                        new_socket_pred=new_socket;
                        temos_pred=1;
                        temos_suc=1;

                    }
                    else{

                        //Atualiza as tabelas
                        elimina_vizinho(new_socket_suc, node->id,node->predecessor, tabela_encaminhamento,tabela_curtos,tabela_expedicao);

                        // Envia uma mensagem a informar ao novo nó do seu segundo-sucessor
                        //char* mensagem = acumula_routes(node, tabela_curtos);

                        send_succ(new_socket, node->sucessor);

                        //Atualiza o st
                        node->predecessor = createNode(new_id, new_ip, new_port);

                        //Envia para o antigo predecessor o Entry
                        send_entry(new_socket_pred, node->predecessor);

                        acumula_routes(new_socket, node, tabela_curtos);

                        //Atualiza a nova socket com o predecessor
                        new_socket_pred=new_socket;
                        temos_pred=1;
                        temos_suc=1;

                    }

                }
                
                // Verifica se é uma mensagem PRED
                if (strncmp(buffer, "PRED", 4) == 0) {
                    //  printf("\nENTREI NO PRED\n");
                    int new_id,i;
                    
                    // Analisa a mensagem PRED
                    sscanf(buffer, "PRED %d", &new_id);
                                            
                    // Informa o seu sucessor sobre a sua identidade.
                    printf("Informações do : id=%02d\n", new_id);

                    //Criar um novo nó
                    node->predecessor = createNode(new_id, inet_ntoa(address.sin_addr), port_char);

                    //atualiza o socket do predecessor
                    new_socket_pred = new_socket;
                    for(i=0;i<20;i++){
                        if(strcmp(mensagens_guardadas[i],"-1")!=0){
                            send_route(new_socket_pred,mensagens_guardadas[i]);
                            strcpy(mensagens_guardadas[i],"-1");
                        }
                    }
                    temos_pred=1;

                    //Analisa mensagens ROUTE
                    char route_type[6];
                    int source, destination;
                    char path[64]; // Adjust size as needed

                    // Start reading from the buffer
                    char* line = strtok(buffer, "\n");

                    while (line != NULL) {
                        if (strncmp(line, "ROUTE", 5) == 0) {
                            numero = sscanf(line, "%s %d %d %s", route_type, &source, &destination, path);

                            if (numero==3){
                                elimina_no(new_socket_pred,new_socket_suc ,node->id, destination, tabela_encaminhamento,tabela_curtos,tabela_expedicao);
                            }else if (numero ==4){
                                update_tabelas(mensagens_guardadas,temos_pred,new_socket_pred, new_socket_suc, node, tabela_encaminhamento, tabela_curtos, tabela_expedicao, source, destination, path);
                            }
                        }

                        // Get the next line from the buffer
                        line = strtok(NULL, "\n");
                    }
                    /////////////////////////////////////////////////////////////////
                    if(pred_saiu==1){
                        send_succ(new_socket_pred, node->sucessor);
                        pred_saiu=-1;
                    }

                }
            }

        }

        if(temos_pred==1){
            if (FD_ISSET(new_socket_pred, &readfds)){
                char buffer[1024];
                int valread;
                if ((valread = read(new_socket_pred, buffer,1024 - 1)) > 0) {
                    printf("\nVOU LER A MENSAGEM DO MEU PRED\n");
                    buffer[valread] = '\0';
                    printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida

                    if (strncmp(buffer, "ROUTE", 5) == 0){
                        //Analisa mensagens ROUTE
                        char route_type[6];
                        int source, destination;
                        char path[64]; // Adjust size as needed

                        // Start reading from the buffer
                        char* line = strtok(buffer, "\n");

                        while (line != NULL) {
                            if (strncmp(line, "ROUTE", 5) == 0) {
                                numero=sscanf(line, "%s %d %d %s", route_type, &source, &destination, path);

                                if (numero==3){
                                    elimina_no(new_socket_pred,new_socket_suc ,node->id, destination, tabela_encaminhamento,tabela_curtos,tabela_expedicao);
                                }else if (numero ==4){
                                    update_tabelas(mensagens_guardadas,temos_pred,new_socket_pred, new_socket_suc, node, tabela_encaminhamento, tabela_curtos, tabela_expedicao, source, destination, path);
                                }
                            }
                            // Get the next line from the buffer
                            line = strtok(NULL, "\n");
                        }
                        
                    }

                }else{
                    printf("\nO meu predecessor saiu\n");
                    pred_saiu=1;
                    close(new_socket_pred);
                    temos_pred=-1;
                    new_socket_pred=-1;

                    sprintf(buffer, "ROUTE %d %d\n", node->id, node->predecessor->id);
                    send_route(new_socket_suc,buffer);

                    elimina_no(-1,new_socket_suc ,node->id, node->sucessor->id, tabela_encaminhamento,tabela_curtos,tabela_expedicao);

                    //ele aqui pode sempre definir o predecessor como ele propria porque caso havia 2 nós, fica certo
                    //caso havia mais que 2 nós ele há de receber um pred e atualizar
                    node->predecessor=node;

                }
            }
        }

        if(temos_suc==1){

            if (FD_ISSET(new_socket_suc, &readfds)){
                printf("\n----------------ENTROU AQUI NO FD ISSET DO SUCESSOR---------------------\n");
                char buffer[1024];
                int valread;
                if ((valread = read(new_socket_suc, buffer,1024 - 1)) > 0) { // subtract 1 for the null terminator at the end
                              
                    buffer[valread] = '\0';
                    printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida

                    // Verifica se é uma mensagem de entrada
                    if (strncmp(buffer, "ENTRY", 5) == 0) {

                        close(new_socket_suc);

                        node->second_successor=node->sucessor;

                        int new_id;
                        char new_ip[16];
                        char new_port[6];
                        
                        // Analisa a mensagem ENTRY
                        sscanf(buffer, "ENTRY %d %s %s", &new_id, new_ip, new_port);
                                                
                        // Imprime as informações do novo nó
                        printf("Informações de um novo cliente: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);

                        elimina_vizinho(new_socket_pred, node->id, node->sucessor, tabela_encaminhamento,tabela_curtos,tabela_expedicao);

                        //Criar um novo nó
                        node->sucessor = createNode(new_id, new_ip, new_port);

                        // Envia uma mensagem a informar ao predecessor do seu segundo sucessor
                        //char mensagem[500] = "";
                        send_succ(new_socket_pred, node->sucessor);

                        // Conexão TCP com o novo nó de entrada na porta dele
                        int new_socket_tcp = cliente_tcp(node, new_ip, new_port);

                        // Envia uma mensagem PRED para a nova porta tcp estabelecida
                        send_pred(new_socket_tcp, node);

                        acumula_routes(new_socket_tcp, node, tabela_curtos);

                        // Define como a socket com o sucessor a new_socket_tcp
                        new_socket_suc = new_socket_tcp;
                        temos_suc=1;


                    }

                    if (strncmp(buffer, "SUCC", 4) == 0) {
                        int new_id;
                        char new_ip[16];
                        char new_port[6];
                        
                        // Analisa a mensagem SUCC
                        sscanf(buffer, "SUCC %d %s %s", &new_id, new_ip, new_port);
                                                
                        // Informa o seu sucessor sobre a sua identidade.
                        printf("Informações do segundo sucessor: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);

                        //Criar um novo nó
                        node->second_successor = createNode(new_id, new_ip, new_port);

                    }
                    if (strncmp(buffer, "ROUTE", 5) == 0){
                        //Analisa mensagens ROUTE
                        char route_type[6];
                        int source, destination;
                        char path[64]; // Adjust size as needed

                        // Start reading from the buffer
                        char* line = strtok(buffer, "\n");

                        while (line != NULL) {
                            if (strncmp(line, "ROUTE", 5) == 0) {
                                numero=sscanf(line, "%s %d %d %s", route_type, &source, &destination, path);

                                if (numero==3){
                                    elimina_no(new_socket_pred,new_socket_suc ,node->id, destination, tabela_encaminhamento,tabela_curtos,tabela_expedicao);
                                }else if (numero ==4){
                                    update_tabelas(mensagens_guardadas,temos_pred,new_socket_pred, new_socket_suc, node, tabela_encaminhamento, tabela_curtos, tabela_expedicao, source, destination, path);
                                }
                            }

                            // Get the next line from the buffer
                            line = strtok(NULL, "\n");
                        }
                        
                    }
                }else{
                    printf("\nO meu sucessor saiu\n");
                    //fecha a adjacencia
                    close(new_socket_suc);
                    temos_suc=-1;
                    new_socket_suc=-1;

                    sprintf(buffer, "ROUTE %d %d\n", node->id, node->sucessor->id);
                    send_route(new_socket_pred,buffer);

                    elimina_no(new_socket_pred,-1 ,node->id, node->sucessor->id, tabela_encaminhamento,tabela_curtos,tabela_expedicao);

                    //define o novo sucessor como o antigo segundo sucessor
                    node->sucessor=node->second_successor;

                    if((node->id)!=(node->sucessor->id)){
                        //envia SUCC para o predecessor
                        send_succ(new_socket_pred, node->sucessor);

                        int new_socket_tcp = cliente_tcp(node, node->sucessor->ip, node->sucessor->tcp);

                        // Envia uma mensagem PRED para a nova porta tcp estabelecida
                        send_pred(new_socket_tcp, node);

                        // Define como a socket com o sucessor a new_socket_tcp
                        new_socket_suc = new_socket_tcp;
                        temos_suc=1;

                    }

                }     
            }
        }
    }
    
    close(tcp_socket);
    return 0;
}