#ifndef INTERFACE_UTILIZADOR_H
#define INTERFACE_UTILIZADOR_H

#include "camada_topologica.h"
#include <stdio.h>

Node* join(int ring, int id, char* IP, char* TCP);
Node* direct_join(int id, int succId, char* succIP, char* succTCP);
void chord();
void remove_chord();
void show_topology(Node* node);
void show_routing(int dest);
void show_path(int dest);
void show_forwarding();
void message(int dest, char* message);
void leave(Node* node, int ring);
void exit();
void nodeslist(int ring);

#endif // INTERFACE_UTILIZADOR_H
