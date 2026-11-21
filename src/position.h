#ifndef POSITION_H
#define POSITION_H

#include <stdint.h>
#include "types.h"
#include "bitboard.h"

/* pieces 0..5 white, 6..11 black */
#define W_P 0
#define W_N 1
#define W_B 2
#define W_R 3
#define W_Q 4
#define W_K 5
#define B_P 6
#define B_N 7
#define B_B 8
#define B_R 9
#define B_Q 10
#define B_K 11

#define EMPTY -1

typedef struct {
    u64 bb[12];
    int side;        /* 0 white, 1 black */
    int castling;    /* bits: 1 wk,2 wq,4 bk,8 bq */
    int ep;          /* square or -1 */
    int halfmove;
    int fullmove;
    /* game history for repetitions: array of zobrist hashes */
    u64 *history;
    int hist_cap;
    int hist_len;
} Position;

typedef struct {
    Position *pos;
    int side_before;
    int castling_before;
    int ep_before;
    int halfmove_before;
    int captured;
    u64 hash_before;
    /* for unmake we just restore fields from saved? store backup */
    int moved_piece;
    int from,to;
} Undo;

void pos_init(Position *p);
int pos_from_fen(Position *p, const char *fen);
int pos_to_fen(const Position *p, char *buf, int bufsz);
u64 pos_hash(const Position *p);
int pos_side(const Position *p);

/* make/unmake */
void pos_make_move(Position *p, const Move *m, Undo *u);
void pos_unmake_move(Position *p, const Move *m, const Undo *u);

/* helpers */
int piece_at(const Position *p, int sq);
u64 attacks_to(const Position *p, int sq, int side); /* attacks by side */
int in_check(const Position *p, int side);
int has_legal_moves(Position *p);

/* endgame */
int is_insufficient_material(const Position *p);

/* history */
void pos_history_push(Position *p, u64 h);
void pos_history_reset(Position *p);
int pos_is_repetition(const Position *p);

#endif