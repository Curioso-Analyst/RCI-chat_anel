#include "camada_encaminhamento.h"


//A tabela [101][101][55] ocupa 560kB
//o 55 vem do facto de termos no max 18 nós. Cada nó pode ter 2 algarismos logo 18*2 +17 sendo os 17 hifens

void cria_tabelas(char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55], char tabela_expedicao[101][2][5]) {
    int i,j;
    for (i = 0; i < 101; i++) {
        for (j = 0; j < 101; j++) {
            strcpy(tabela_encaminhamento[i][j], "-1");
        }
    }
    strcpy(tabela_encaminhamento[0][0], " ");
    for (i = 0; i < 101; i++) {
        for (j = 0; j < 2; j++) {
            strcpy(tabela_curtos[i][j], "-1");
        }
    }
    for (i = 0; i < 101; i++) {
        for (j = 0; j < 2; j++) {
            strcpy(tabela_expedicao[i][j], "-1");
        }
    }
}

//Quando recebe um ROUTE
void update_tabelas(char mensagens_guardadas[20][512], int temos_pred,int socket_pred, int socket_suc, Node* node,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5], int origem, int destino, char caminho[64]) {
    //Quando recebo um route será sempre de um vizinho seja pred,suc ou corda
    //char input[] = "ROUTE 30 10 30-8-10\n"; // Input string
    char temporario[55]; // Variables to store parameters
    char nodeid[3];
    char buffer[1024];
    int aux=0;

    //passar de int para string
    sprintf(nodeid, "%d", node->id);

    if (strstr(caminho, nodeid) != NULL) {
        //printf("Eu sou um dos nós no caminho - inválido\n");
    } else {
        //printf("Não estou no caminho\n");

        //concatnacao do meu nó com o caminho
        strcpy(temporario, nodeid);
        strcat(temporario, "-");
        strcat(temporario, caminho);

        //printf("o temporario é igual a: %s", temporario);

        //Atualizacao da tabela de encaminhamentos
        sprintf(tabela_encaminhamento[destino + 1][0], "%d", destino);
        sprintf(tabela_encaminhamento[0][origem+1], "%d", origem);
        strcpy(tabela_encaminhamento[destino+1][origem+1],temporario);

        //Atualizacao da tabela de caminhos mais curtos
        if(strcmp(tabela_curtos[destino][1],"-1")==0){
            //printf("ainda não ha caminho mais curto\n");

            sprintf(tabela_curtos[destino][0], "%d", destino);
            strcpy(tabela_curtos[destino][1],temporario);

            //printf("o caminho mais curto para o destino %d OU %s é: %s\n", destino,tabela_curtos[destino][0],tabela_curtos[destino][1]);

            //A entrada na tabela de mais curtos é diferente da anterior logo anuncia aos vizinhos
            sprintf(buffer, "ROUTE %d %d %s\n", node->id, destino, temporario);
            if(temos_pred==1){
                send_route(socket_pred, buffer);
            }else{
                strcpy(mensagens_guardadas[aux], buffer);
                aux++;
            }
            send_route(socket_suc, buffer);

            //Atualiza a tabela de expedicao
            sprintf(tabela_expedicao[destino][0], "%d", destino);
            sprintf(tabela_expedicao[destino][1], "%d", origem);

            //printf("o caminho de espedicao para o destino %s é: %s\n",tabela_expedicao[destino][0],tabela_expedicao[destino][1]);



        }else{
            if(strlen(temporario) < strlen(tabela_curtos[destino][1])){
                strcpy(tabela_curtos[destino][1],temporario);

                //printf("o caminho mais curto para o destino %d OU %s é: %s\n", destino,tabela_curtos[destino][0],tabela_curtos[destino][1]);

                //A entrada na tabela de mais curtos é diferente da anterior logo anuncia aos vizinhos
                sprintf(buffer, "ROUTE %d %d %s\n", node->id, destino, temporario);
                if(temos_pred==1){
                    send_route(socket_pred, buffer);
                }else{
                    strcpy(mensagens_guardadas[aux], buffer);
                    aux++;
                }
                send_route(socket_suc, buffer);

                //Atualiza a tabela de expedicao
                sprintf(tabela_expedicao[destino][1], "%d", origem);

                //printf("o caminho de espedicao para o destino OU %s é: %s\n",tabela_expedicao[destino][0],tabela_expedicao[destino][1]);
            } 
        }
    }
    
}

//envia um route especifico
void send_route(int fd, char buffer[1024]){
    // Imprime a mensagem que será enviada
    printf("Mensagem a ser enviada: %s\n", buffer);
    int n = write(fd, buffer, strlen(buffer));
    if (n == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    printf("Mensagem enviada!\n");
}

//acumula todos os routes para enviar
void acumula_routes(int fd, Node* node,char tabela_curtos[101][2][55]){
    char buffer[1024];
    int i;
    for(i=0; i<101; i++){
        if(strcmp(tabela_curtos[i][0],"-1")!=0){
            sprintf(buffer, "ROUTE %d %s %s\n", node->id, tabela_curtos[i][0], tabela_curtos[i][1]);
            send_route(fd, buffer);
        }
    }
}

void imprimir_encaminhamento(int destino, char tabela_encaminhamento[101][101][55]){
    int i;
    for(i=0; i<101; i++){
        if(strcmp(tabela_encaminhamento[destino+1][i],"-1")!=0){
            printf(" %s |",tabela_encaminhamento[0][i]);
        }
    }
    printf("\n");
    for(i=0; i<101; i++){
        if(strcmp(tabela_encaminhamento[destino+1][i],"-1")!=0){
            printf(" %s |",tabela_encaminhamento[destino+1][i]);
        }
    }
    printf("\n");
}

void imprimir_expedicao(char tabela_expedicao[101][2][5]){
    int i,j;
    for(i=0; i<101; i++){
        if(strcmp(tabela_expedicao[i][0],"-1")!=0){
            for(j=0; j<2; j++){

                printf(" %s |",tabela_expedicao[i][j]);
            }
            printf("\n");
        }
    }
}

//Quando recebemos um entry
//eliminamos apenas coluna na tabela de encaminhamento
void elimina_vizinho(int fd, int id, Node* node,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]){
    char nodeid[2], tempor[100], viz[2];
    int aux=0, vizinho=-1;
    int i,j;
    char buffer[512];

    //passar de int para string
    sprintf(nodeid, "%d", node->id);
    
    //elimina coluna na tabela de encaminhamento
    for(i=0; i<101; i++){
        strcpy(tabela_encaminhamento[i][node->id+1],"-1");
    }
    //atualiza tabela de caminho curtos e expedicao
    for(i=1; i<101; i++){
        aux=0;
        vizinho=-1;
        if(strcmp(tabela_encaminhamento[i][0],"-1")!=0){
            //se o vizinho do caminho mais curto saiu
            if(strcmp(tabela_expedicao[i-1][1],nodeid)==0){
                strcpy(tabela_curtos[i-1][1],"-1");
            }
            for(j=0; j<101; j++){
                if(strcmp(tabela_encaminhamento[i][j],"-1")!=0){
                    if(aux==0){
                        strcpy(tempor,tabela_encaminhamento[i][j]);
                        vizinho=j-1;
                        aux=1;
                    }else{
                        if(strlen(tabela_encaminhamento[i][j]) < strlen(tempor)){
                            strcpy(tempor,tabela_encaminhamento[i][j]);
                            vizinho=j-1;
                        }
                    }
                }
            }
            if(strcmp(tabela_curtos[i-1][1],"-1")==0){
                strcpy(tabela_curtos[i-1][1],tempor);

                //manda para os vizinhos exceto o que entrou agora
                sprintf(buffer, "ROUTE %d %d %s", id, i-1, tempor);
                send_route(fd, buffer);

                sprintf(viz, "%d", vizinho);
                strcpy(tabela_expedicao[i-1][1],viz);
            }else if((strcmp(tabela_curtos[i-1][1],"-1")!=0) && (strlen(tempor)<strlen(tabela_curtos[i-1][1]))){
                strcpy(tabela_curtos[i-1][1],tempor);

                //manda para os vizinhos exceto o que entrou agora
                sprintf(buffer, "ROUTE %d %d %s", id, i-1, tempor);
                send_route(fd, buffer);

                sprintf(viz, "%d", vizinho);
                strcpy(tabela_expedicao[i-1][1],viz);
            }
        }   
    }
}

//Quando o vizinho sai do anel
//elimina coluna e linha na tabela de encaminhamento
void elimina_no(int socket_pred, int socket_suc, int meu_id, int id_saida,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]){
    char tempor[100], viz[2];
    int aux=0, vizinho=-1;
    int i,j;
    char buffer[512];
    
    //elimina coluna e linha na tabela de encaminhamento
    for(i=0; i<101; i++){
        strcpy(tabela_encaminhamento[i][id_saida+1],"-1");
        strcpy(tabela_encaminhamento[id_saida+1][i],"-1");
    }
    //elimina linha na tabela curtos e expedicao
    for(i=0; i<2; i++){
        strcpy(tabela_curtos[id_saida][i],"-1");
        strcpy(tabela_expedicao[id_saida][i],"-1");
    }
    //atualiza tabela de caminho curtos e expedicao
    for(i=1; i<101; i++){
        aux=0;
        vizinho=-1;
        if(strcmp(tabela_encaminhamento[i][0],"-1")!=0){
            
            for(j=0; j<101; j++){
                if(strcmp(tabela_encaminhamento[i][j],"-1")!=0){
                    if(aux==0){
                        strcpy(tempor,tabela_encaminhamento[i][j]);
                        vizinho=j-1;
                        aux=1;
                    }else{
                        if(strlen(tabela_encaminhamento[i][j]) < strlen(tempor)){
                            strcpy(tempor,tabela_encaminhamento[i][j]);
                            vizinho=j-1;
                        }
                    }
                }
            }

            //acho que este if é redundante, a tabela nunca vai estar a -1
            if(strcmp(tabela_curtos[i-1][1],"-1")==0){
                strcpy(tabela_curtos[i-1][1],tempor);

                //manda para os vizinhos exceto o que entrou agora
                sprintf(buffer, "ROUTE %d %d %s", meu_id, i-1, tempor);
                if(socket_pred!=-1){
                    send_route(socket_pred, buffer);
                }
                if(socket_suc!=-1){
                    send_route(socket_suc, buffer);
                }

                sprintf(viz, "%d", vizinho);
                strcpy(tabela_expedicao[i-1][1],viz);
            }else if((strcmp(tabela_curtos[i-1][1],"-1")!=0) && (strlen(tempor)<strlen(tabela_curtos[i-1][1]))){
                strcpy(tabela_curtos[i-1][1],tempor);

                //manda para os vizinhos exceto o que entrou agora
                sprintf(buffer, "ROUTE %d %d %s", meu_id, i-1, tempor);
                if(socket_pred!=-1){
                    send_route(socket_pred, buffer);
                }
                if(socket_suc!=-1){
                    send_route(socket_suc, buffer);
                }

                sprintf(viz, "%d", vizinho);
                strcpy(tabela_expedicao[i-1][1],viz);
            }
        }   
    }
}


//quando um no distante sai do anel
//recebemos um ROUTE X Y 
//mais tarde vamos receber os ROUTE com os caminhos mais curtos dos outros aneis logo basta-nos eliminar as linhas
// void elimina_no_distante(int id,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]){
//     int i;

//     //elimina linha na tabela de encaminhamento
//     for(i=0; i<101; i++){
//         //penso que esta primeria linha seja redundante pois como não era vizinho não existe coluna
//         strcpy(tabela_encaminhamento[i][id+1],"-1");
//         strcpy(tabela_encaminhamento[id+1][i],"-1");
//     }
//     //elimina linha na tabela curtos e expedicao
//     for(i=0; i<2; i++){
//         strcpy(tabela_curtos[id][i],"-1");
//         strcpy(tabela_expedicao[id][i],"-1");
//     }
// }

// void elimina_vizinho(Node* node,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]){
//     //elimina coluna na tabela de encaminhamento
//     for(i=0; i<101; i++){
//         strcpy(tabela_encaminhamento[i][node->id+1],"-1");
//     }
//     //atualiza tabela de caminho curtos
//     for(i=1; i<101; i++){
//         strcpy(tabela_curtos[i-1][1],"-1");
//         if(strcmp(tabela_encaminhamento[i][0],"-1")!=){
//             for(j=0; j<101; j++){
//                 if(strcmp(tabela_encaminhamento[i][j],"-1")!=){
//                     if(strcmp(tabela_curtos[i-1][1],"-1")==0){
//                         strcpy(tabela_curtos[i-1][1],tabela_encaminhamento[i][j]);
//                         strcpy(tabela_expedicao[i-1][1])
//                     }
//                     else{
//                         if(strlen(tabela_encaminhamento[i][j]) < strlen(tabela_curtos[i-1][1])){
//                             strcpy(tabela_curtos[i-1][1],tabela_encaminhamento[i][j]);
//                         }
//                     }
//                 }
//             }       
//         }   
//     }
// }

// void imprimir_encaminhamento(char tabela_encaminhamento[101][101][55]){
//     int i,j;
//     for(i=0; i<101; i++){
//         if(strcmp(tabela_encaminhamento[i][0],"-1")!=0){
//             for(j=0; j<101; j++){
//                 if(strcmp(tabela_encaminhamento[i][j],"-1")!=0){
//                     printf(" %s |",tabela_encaminhamento[i][j]);
//                 }
//             }
//             printf("\n");
//         }
//     }
// }

// void imprimir_curtos(char tabela_curtos[101][2][55]){
//     int i,j;
//     for(i=0; i<101; i++){
//         if(strcmp(tabela_curtos[i][0],"-1")!=0){
//             for(j=0; j<2; j++){

//                 printf(" %s |",tabela_curtos[i][j]);
//             }
//             printf("\n");
//         }
//     }
// }

