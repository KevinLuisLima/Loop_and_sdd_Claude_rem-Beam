#ifndef CHESS_H
#define CHESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_MOVES 256

enum { PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4, KING=5 };
enum { WHITE=0, BLACK=1 };

typedef struct {
    uint64_t bb[2][6];
    int side;
    int castling; /* bit 0 WK,1 WQ,2 BK,3 BQ */
    int ep; /* square or -1 */
    int halfmove;
    int fullmove;
} Board;

typedef struct { int from,to,promo; } Move;

/* board.c */
void board_startpos(Board *b);
int board_from_fen(Board *b, const char *fen);
int board_to_fen(const Board *b, char *out, size_t n);
uint64_t bb_occ(const Board *b, int color);
uint64_t bb_all(const Board *b);
int is_square_attacked(const Board *b, int sq, int side);
int in_check(const Board *b, int side);
void make_move(Board *b, Move m);
int move_is_legal(Board b, Move m);
uint64_t perft(Board b, int depth);

/* movegen.c */
int gen_pseudo(const Board *b, Move *moves);
int gen_legal_moves(const Board *b, Move *moves);

/* eval.c */
#define WEIGHT_COUNT 400
int eval_load(const char *path);
int eval_load_default(void);
int evaluate(const Board *b); /* relative to side to move */

/* search.c */
int search_bestmove(Board b, int movetime_ms, Move *out);

#endif