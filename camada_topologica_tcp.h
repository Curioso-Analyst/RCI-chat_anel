#ifndef CAMDA_TOPOLOGICA_TCP_H
#define CAMDA_TOPOLOGICA_TCP_H

#include "camada_topologica.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void cliente_tcp(Node* node, char* succTCP);
void send_entry(int fd, Node* node);
void send_succ(int fd, Node* node);

#endif // CAMDA_TOPOLOGICA_TCP_H