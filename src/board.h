#ifndef BOARD_H
#define BOARD_H
#include "types.h"
void pos_init(Position *pos);
int pos_from_fen(Position *pos, const char *fen);
void pos_to_fen(Position *pos, char *buf, size_t sz);
uint64_t fen_hash(Position *pos);
int make_move(Position *pos, Move m);
void unmake_move(Position *pos);
int in_check(Position *pos, int color);
int legal_moves(Position *pos, Move *moves); /* returns count */
int is_game_over(Position *pos);
#endif