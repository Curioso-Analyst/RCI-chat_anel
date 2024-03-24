#ifndef CAMDA_TOPOLOGICA_TCP_H
#define CAMDA_TOPOLOGICA_TCP_H

#include "camada_topologica.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int cliente_tcp(Node* node,char* j_ip,char* j_port);
void send_entry(int fd, Node* node);
void send_succ(int fd, Node* node);
void send_pred(int fd, Node* node);
void removeNode(Node** node_to_remove_ptr);

#endif // CAMDA_TOPOLOGICA_TCP_H