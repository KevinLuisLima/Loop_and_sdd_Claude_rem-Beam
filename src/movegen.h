#ifndef MOVEGEN_H
#define MOVEGEN_H
#include "types.h"
#include "board.h"

int  gen_pseudo_moves(const Board *b, Move *list); /* returns count */
int  gen_legal_moves(Board *b, Move *list);        /* fills list, returns count */
int  move_is_capture(const Board *b, Move m);
int  move_to_uci(Move m, char *buf); /* writes e.g. e2e4 or e7e8q */

#endif