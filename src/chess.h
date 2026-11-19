#ifndef CHESS_H
#define CHESS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NUM_WEIGHTS 6
#define EMPTY 12

typedef enum { WHITE=0, BLACK=1 } Color;

enum Piece { WP=0,WN,WB,WR,WQ,WK, BP,BN,BB,BR,BQ,BK };

typedef struct {
    int board[64];
    uint64_t bb[12];
    uint64_t occ[3]; /* 0 white,1 black,2 all */
    int side;
    int castling; /* KQkq bits */
    int ep; /* -1 or square */
    int halfmove;
    int fullmove;
    bool valid;
} Position;

typedef struct {
    int from, to, prom, flags;
    int captured;
} Move;

/* undo not used; positions copied */

void pos_init_start(Position *p);
int pos_from_fen(Position *p, const char *fen);
int pos_to_fen(const Position *p, char *buf, size_t sz);
Color color_of(int piece);
int type_of(int piece);

void make_move(Position *p, const Move *m);
void unmake_move(Position *p, const Move *m, const Position *saved);

int generate_pseudo_moves(const Position *p, Move *moves);
int filter_legal(Position *p, Move *moves, int n);

uint64_t attack_bb(const Position *p, int sq, int color);
int is_attacked(const Position *p, int sq, int color);
int in_check(const Position *p, int color);

long long perft(Position p, int depth);

int count_legal_moves(Position *p);
Move find_best_move(Position *p, const float *weights, int time_ms, int max_depth);

#endif