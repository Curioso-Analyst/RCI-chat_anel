#ifndef TCPCOM_H
#define TCPCOM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> // Para struct timeval
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>

#include "camada_topologica.h"
#include "interface_utilizador.h"
#include "camada_topologica_tcp.h"  


#define MAX_CLIENTS 30

int serverclient(Node* node, char* TCP);

#endif // TCPCOM_H;