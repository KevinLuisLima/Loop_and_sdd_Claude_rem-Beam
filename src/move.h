#ifndef MOVE_H
#define MOVE_H
#include "board.h"

typedef struct {
    int from, to;
    int prom; /* 0 none, piece type 0..3 for Q R B N from q? store 0=Q? We'll store 1=Q? */
    int score;
} Move;

typedef struct {
    Move moves[256];
    int count;
} MoveList;

void gen_moves(const Board *b, MoveList *ml, bool captures_only);
void gen_legal_moves(const Board *b, MoveList *ml);
bool is_legal(const Board *b, Move m);
int move_to_uci(Move m, char *out);
int parse_move_uci(const Board *b, const char *s, Move *m);

#endif