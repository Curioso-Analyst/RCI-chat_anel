#ifndef CANADA_ENCAMINHAMENTO_H
#define CANADA_ENCAMINHAMENTO_H

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "camada_topologica.h"

void cria_tabelas(char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55], char tabela_expedicao[101][2][5]);
void update_tabelas(char mensagens_guardadas[20][512], int temos_pred,int socket_pred, int socket_suc, Node* node,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5], int origem, int destino, char caminho[64]);
void send_route(int fd, char buffer[512]);
void acumula_routes(int fd, Node* node,char tabela_curtos[101][2][55]);
// void send_all_routes(Node* node,char tabela_curtos[101][2][55]);
void imprimir_encaminhamento(int destino, char tabela_encaminhamento[101][101][55]);
//void imprimir_curtos(char tabela_curtos[101][2][55]);
void imprimir_expedicao(char tabela_expedicao[101][2][5]);
void elimina_vizinho(int fd, int id, Node* node,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]);
void elimina_no(int socket_pred, int socket_suc, int meu_id, int id_saida,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]);
//void elimina_no_distante(int id,char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5]);
//void send_routes_incompletos(char tabela_encaminhamento[101][101][55], char tabela_curtos[101][2][55],char tabela_expedicao[101][2][5])
#endif // CANADA_ENCAMINHAMENTO_H
