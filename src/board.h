#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    EMPTY = -1,
    WP=0, WN=1, WB=2, WR=3, WQ=4, WK=5,
    BP=6, BN=7, BB=8, BR=9, BQ=10, BK=11
} Piece;

#define WHITE 0
#define BLACK 1

typedef struct {
    uint64_t bb[12];
    uint64_t occ[3]; /* 0 white,1 black,2 all */
    int side;
    int castling; /* bits */
    int ep; /* -1 or square 0..63 */
    int halfmove;
    int fullmove;
    /* position history (squares+castling+ep+side compact) for repetition */
    uint64_t history[1024];
    int hist_count;
} Board;

void board_init(Board *b);
int board_load_fen(Board *b, const char *fen);
void board_to_fen(const Board *b, char *out, size_t size);
typedef struct {
    int from,to,prom;
    int moved_piece;
    int captured_piece;
    int captured_sq;
    int castled;
    int rook_from, rook_to;
    int side;
    int castling,ep,halfmove,fullmove;
    uint64_t hash;
} Undo;
bool board_make_move(Board *b, int from, int to, int prom, Undo *u);
void board_unmake(Board *b, const Undo *u);
bool square_attacked(const Board *b, int sq, int by_side);
int piece_at(const Board *b, int sq);
extern uint64_t knight_att[64], king_att[64], pawn_cap[2][64];
void tables_init(void);
uint64_t rook_attacks(int sq,uint64_t occ);
uint64_t bishop_attacks(int sq,uint64_t occ);

/* move helpers */
static inline int piece_color(Piece p){ return p >= BP ? BLACK : WHITE; }
static inline int piece_type(Piece p){ return p % 6; }

#endif