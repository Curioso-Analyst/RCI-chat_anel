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
    int temos_pred = -1, temos_suc =-1, pred_saiu = -1;
    int new_socket = -1;
    struct sockaddr_in address;
    fd_set readfds, writefds;

    printf("Bem-vindo ao programa COR! Digite 'help' para ver os comandos disponíveis.\n");

    // Configura o socket mestre e inicia o servidor
    setup_master_socket(&tcp_socket, PORT);

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

        // Verifica se algo aconteceu no select
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error: %s\n", strerror(errno));
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
                //close(new_socket_suc);
                new_socket_pred=-1;
                new_socket_suc=-1;
                temos_pred=-1;
                temos_suc=-1;
                free(node);
                node = NULL;
            } else {
                printf("Nó não inicializado.\n");
            }
        } else if (strncmp(command, "help", 4) == 0) {
            print_help();
        } else if (strncmp(command, "j", 1) == 0) {
            int num_args = sscanf(command, "j %03d %02d", &ring, &id);

            // Verifica se ambos os parâmetros foram fornecidos
            if (num_args < 2) {
                printf("Erro: Não foram fornecidos argumentos suficientes para o comando 'j'.\n");
                printf("Uso: j <ring> <id>\n");
            } else {
                node = join(ring, id, IP, TCP_escolhido);
                node->ring = ring; // Para depois poder usar a corda 
            }
        } else if (strncmp(command, "dj", 2) == 0) {
            int id, succid;
            char succIP[16], succTCP[6];
            int num_args = sscanf(command, "dj %d %d %s %s", &id, &succid, succIP, succTCP);

            // Verifica se todos os 4 parâmetros foram fornecidos
            if (num_args < 4) {
                printf("Erro: Não foram fornecidos argumentos suficientes para o comando 'dj'.\n");
                printf("Uso: dj <id> <succid> <succIP> <succTCP>\n");
            } else {
                node = direct_join(id, succid, succIP, succTCP);
            }
        } else if (strncmp(command, "c", 1) == 0) {
            // Verifica se o nó foi inicializado
            if (node == NULL) {
                printf("Nó não inicializado. Por favor, inicialize o nó antes de tentar estabelecer uma corda.\n");
            } else if (node->corda != NULL) {
                printf("O nó já tem uma corda ativa. Por favor, remova a corda existente antes de tentar estabelecer uma nova.\n");
            } else {
                // Implementação do comando 'c'
                establishChord(node);
            }
        } else if (strncmp(command, "rc", 2) == 0) {
            // Implementação do comando 'rc'
            if (node != NULL) {
                removeChord(node);
            } else {
                printf("Nenhum nó para remover a corda.\n");
            }
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

                  // Verifica se é uma mensagem de corda
                    if (strncmp(buffer, "CHORD", 5) == 0) {
                        int new_id;
                        
                        // Analisa a mensagem CHORD
                        sscanf(buffer, "CHORD %d", &new_id);

                        // Verifica se já existe uma corda com o mesmo nó
                        for (int i = 0; i < node->num_cordas; i++) {
                            if (node->cordas[i]->id == new_id) {
                                printf("Já existe uma corda com o nó %d. Ignorando a nova corda.\n", new_id);
                                continue;
                            }
                        }
                                                                
                        // Imprime as informações do novo nó
                        printf("Informações de uma nova corda: id=%02d, ip=%s, port=%s\n", new_id, inet_ntoa(address.sin_addr), port_char);

                        // Cria um novo nó
                        Node* new_node = createNode(new_id, inet_ntoa(address.sin_addr), port_char);

                        // Adiciona a nova corda à lista de cordas
                        if (node->num_cordas < MAX_CORDAS) {
                            node->cordas[node->num_cordas] = new_node;
                            node->num_cordas++;
                        } else {
                            printf("Número máximo de cordas atingido. Não é possível adicionar mais cordas.\n");
                        }
                    }
                


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
                    if(node->sucessor==node){

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

                    }else if((node->sucessor!=node) && (node->second_successor==node)){

                        // Envia uma mensagem a informar ao novo nó do seu segundo-sucessor
                        send_succ(new_socket, node->sucessor);

                        //Atualiza o st
                        node->predecessor = createNode(new_id, new_ip, new_port);

                        //Envia para o predecessor o Entry
                        send_entry(new_socket_pred, node->predecessor);

                        //Atualiza a nova socket com o predecessor
                        //new_socket_suc=new_socket_pred;
                        new_socket_pred=new_socket;
                        temos_pred=1;
                        temos_suc=1;
                    }
                    // 
                    else{

                        // Envia uma mensagem a informar ao novo nó do seu segundo-sucessor
                        send_succ(new_socket, node->sucessor);

                        //Atualiza o st
                        node->predecessor = createNode(new_id, new_ip, new_port);

                        //Envia para o antigo predecessor o Entry
                        send_entry(new_socket_pred, node->predecessor);

                        //Atualiza a nova socket com o predecessor
                        new_socket_pred=new_socket;
                        temos_pred=1;
                        temos_suc=1;

                    }

                }
                
                // Verifica se é uma mensagem PRED
                if (strncmp(buffer, "PRED", 4) == 0) {
                    int new_id;
                    
                    // Analisa a mensagem PRED
                    sscanf(buffer, "PRED %d", &new_id);
                                            
                    // Informa o seu sucessor sobre a sua identidade.
                    printf("Informações do : id=%02d\n", new_id);

                    //Criar um novo nó
                    node->predecessor = createNode(new_id, inet_ntoa(address.sin_addr), port_char);

                    //atualiza o socket do predecessor
                    new_socket_pred = new_socket;
                    temos_pred=1;
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
                    buffer[valread] = '\0';
                    printf("Mensagem recebida: %s\n", buffer);  // Imprime a mensagem recebida

                    // Verifica se é uma mensagem de entrada
                    //Só recebe entry do predecessor quando um 3 nó se está a tentar se juntar ao outro que la está
                    /*if (strncmp(buffer, "ENTRY", 5) == 0) {

                        node->second_successor=node->predecessor;

                        int new_id;
                        char new_ip[16];
                        char new_port[6];
                        
                        // Analisa a mensagem ENTRY
                        sscanf(buffer, "ENTRY %d %s %s", &new_id, new_ip, new_port);
                                                
                        // Imprime as informações do novo nó
                        printf("Informações de um novo cliente: id=%02d, ip=%s, port=%s\n", new_id, new_ip, new_port);

                        //Criar um novo nó
                        node->sucessor = createNode(new_id, new_ip, new_port);

                        // Envia uma mensagem a informar ao predecessor do seu segundo sucessor
                        send_succ(new_socket_pred, node->sucessor);

                        // Conexão TCP com o novo nó de entrada na porta dele
                        int new_socket_tcp = cliente_tcp(node, new_ip, new_port);

                        // Envia uma mensagem PRED para a nova porta tcp estabelecida
                        send_pred(new_socket_tcp, node->predecessor);

                        // Define como a socket com o predecessor a new_socket_tcp
                        new_socket_suc = new_socket_tcp;
                        temos_suc=1;


                    }*/
                }else{
                    printf("\nO meu predecessor saiu\n");
                    pred_saiu=1;
                    close(new_socket_pred);
                    temos_pred=-1;
                    new_socket_pred=-1;

                    //ele aqui pode sempre definir o predecessor como ele propria porque caso havia 2 nós, fica certo
                    //caso havia mais que 2 nós ele há de receber um pred e atualizar
                    node->predecessor=node;

                }
            }
        }

        if(temos_suc==1){

            if (FD_ISSET(new_socket_suc, &readfds)){
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

                        //Criar um novo nó
                        node->sucessor = createNode(new_id, new_ip, new_port);

                        // Envia uma mensagem a informar ao predecessor do seu segundo sucessor
                        send_succ(new_socket_pred, node->sucessor);

                        // Conexão TCP com o novo nó de entrada na porta dele
                        int new_socket_tcp = cliente_tcp(node, new_ip, new_port);

                        // Envia uma mensagem PRED para a nova porta tcp estabelecida
                        send_pred(new_socket_tcp, node);

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
                }else{
                    printf("\nO meu sucessor saiu\n");
                    //fecha a adjacencia
                    close(new_socket_suc);
                    temos_suc=-1;
                    new_socket_suc=-1;

                    //define o novo sucessor como o antigo segundo sucessor
                    node->sucessor=node->second_successor;

                    if(node!=node->sucessor){
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