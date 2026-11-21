#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

#define MAXPLY 512
#define MAXMOVES 256

typedef enum { WHITE = 0, BLACK = 1 } Color;
typedef enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

/* move packed into int */
typedef int Move;

#define MOVE_FROM(m) ((m) & 63)
#define MOVE_TO(m) (((m) >> 6) & 63)
#define MOVE_PROMO(m) (((m) >> 12) & 7)
#define MOVE_FLAGS(m) (((m) >> 15) & 7)

#define MAKE_MOVE(f,t,p,fl) ((f) | ((t)<<6) | ((p)<<12) | ((fl)<<15))

#define FLAG_NORMAL 0
#define FLAG_CAPTURE 1
#define FLAG_DOUBLE 2
#define FLAG_EP 3
#define FLAG_PROMO 4
#define FLAG_CASTLE 5

typedef struct {
    uint64_t bb[2][6];
    int side;
    int castling; /* bit0 wk,1 wq,2 bk,3 bq */
    int ep_square; /* -1 none */
    int halfmove;
    int fullmove;
    /* undo history */
    int ply;
    int hist_from[MAXPLY];
    int hist_to[MAXPLY];
    int hist_moved[MAXPLY];
    int hist_promoted[MAXPLY];
    int hist_cap[MAXPLY];
    int hist_capsq[MAXPLY];
    int hist_cast[MAXPLY];
    int hist_ep[MAXPLY];
    int hist_half[MAXPLY];
    uint64_t hist_hash[MAXPLY];
} Position;

typedef struct {
    uint64_t key;
    int depth;
    int score;
    int flag; /* 0 exact,1 lower,2 upper */
    int best;
} TTEntry;

typedef struct {
    TTEntry *entries;
    size_t size;
    uint64_t mask;
} TranspositionTable;

typedef struct {
    float w[512];
    int n;
} Weights;

typedef struct {
    Position *pos;
    TranspositionTable *tt;
    Weights *weights;
    uint64_t nodes;
    double start_time;
    double time_limit;
    int stop;
    int max_depth;
    int depth_reached;
} SearchContext;

#endif