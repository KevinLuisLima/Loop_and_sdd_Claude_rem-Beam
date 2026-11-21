#ifndef MOVEGEN_H
#define MOVEGEN_H
#include "types.h"
#include "position.h"
int gen_moves(Position *p, Move *moves);
int gen_captures(Position *p, Move *moves);
#endif