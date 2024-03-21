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
void update_tabelas(Node* node,char tabela_encaminhamento[101][101][50], char tabela_curtos[101][2][50],char tabela_expedicao[101][2][5]) {
    //Quando recebo um route será sempre de um vizinho seja pred,suc ou corda
    //char input[] = "ROUTE 30 10 30-8-10\n"; // Input string
    char param1[10], caminho[55], temporario[55], curto_temporario[55]; // Variables to store parameters
    int origem, destino;
    char *token;
    char *delimiter = "-";

    // Parse input string
    int result = sscanf(input, "%s %d %d %s", param1, &origem, &destino, caminho);

    if (strstr(caminho, node->id) != NULL) {
        printf("Eu sou um dos nós no caminho - inválido\n");
    } else {
        printf("Não estou no caminho\n");

        //concatnacao do meu nó com o caminho
        strcpy(temporario, node->id);
        strcat(temporario, "-");
        strcat(temporario, caminho);

        //Atualizacao da tabela de encaminhamentos
        sprintf(tabela_encaminhamento[destino + 1][0], "%d", destino);
        sprintf(tabela_curtos[0][origem+1], "%d", origem);
        strcpy(tabela_encaminhamento[destino+1][origem+1],temporario);

        //Atualizacao da tabela de caminhos mais curtos
        if(strcmp(tabela_curtos[destino+1][1],"-1")==0){
            sprintf(tabela_curtos[destino + 1][0], "%d", destino);
            strcpy(tabela_curtos[destino+1][1],temporario);

            //A entrada na tabela de mais curtos é diferente da anterior logo anuncia aos vizinhos
            send_route(node,temporario,destino);

            //Atualiza a tabela de expedicao
            sprintf(tabela_expedicao[destino + 1][0], "%d", destino);
            sprintf(tabela_expedicao[destino + 1][1], "%d", origem);

        }else{
            if(strncmp(temporario, tabela_curtos[destino+1][1]) < 0){
                strcpy(tabela_curtos[destino+1][1],temporario);

                //A entrada na tabela de mais curtos é diferente da anterior logo anuncia aos vizinhos
                send_route(node,temporario,destino);

                //Atualiza a tabela de expedicao
                sprintf(tabela_expedicao[destino + 1][1], "%d", origem);
            } 
        }
    }
    
}

void send_route(Node* node, char caminho[55], int destino){
    char mensagem[100];
    sprintf(mensagem, "ROUTE %d %d %s", node->id, destino, caminho);
    
    send_mensagem(mensagem);
}

void send_all_routes(Node* node,char tabela_curtos[101][2][50]){
    char mensagem[100];
    int i;
    for(i=0; i<101; i++){
        if(strcmp(tabela_curtos[i][0],"-1")!=0){
            sprintf(mensagem, "ROUTE %d %s %s", node->id, tabela_curtos[i][0], tabela_curtos[i][1]);

            send_mensagem(mensagem);
        }
    }
}

void imprimir_encaminhamento(char tabela_encaminhamento[101][101][50]){
    int i,j;
    for(i=0; i<101; i++){
        if(strcmp(tabela_encaminhamento[i][0],"-1")!=0){
            for(j=0; j<101; j++){
                if(strcmp(tabela_encaminhamento[i][j],"-1")!=0){
                    printf(" %s |",tabela_encaminhamento[i][j]);
                }
            }
            printf("\n");
        }
    }
}

void imprimir_curtos(char tabela_curtos[101][2][50]){
    int i,j;
    for(i=0; i<101; i++){
        if(strcmp(tabela_curtos[i][0],"-1")!=0){
            for(j=0; j<2; j++){

                printf(" %s |",tabela_curtos[i][j]);
            }
            printf("\n");
        }
    }
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