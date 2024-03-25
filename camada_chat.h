#ifndef CAMADA_CHAT_H
#define CAMADA_CHAT_H

#include "camada_topologica.h"


// Função protcolo chat "CHAT i n chat<LF>", o nó origem i envia ao nó destino n a mensagem chat. Estas mensagens têm 128 carateres no máximo.
void send_chat(int new_socket_suc, Node* node, int dest, char* mensagem);


#endif // CAMADA_CHAT_H