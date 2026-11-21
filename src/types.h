#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned long long U64;

#define MAX_MOVES 256
#define MAX_PLY 64

enum { WHITE = 0, BLACK = 1 };
enum { PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING };

#define W_PAWN 0
#define W_KNIGHT 1
#define W_BISHOP 2
#define W_ROOK 3
#define W_QUEEN 4
#define W_KING 5
#define B_PAWN 6
#define B_KNIGHT 7
#define B_BISHOP 8
#define B_ROOK 9
#define B_QUEEN 10
#define B_KING 11
#define NO_PIECE 12

#define IS_PIECE(p) ((p) >= 0 && (p) < 12)
#define PIECE_COLOR(p) (((p) < 6) ? WHITE : BLACK)
#define PIECE_TYPE(p) (((p) < 6) ? (p) : ((p)-6))

#define CR_WKS 1
#define CR_WQS 2
#define CR_BKS 4
#define CR_BQS 8

#define MOVE_PROMO  (1<<14)
#define MOVE_CAPTURE (1<<15)
#define MOVE_EP     (1<<16)
#define MOVE_CASTLE (1<<17)

typedef int Move;

static inline int move_from(Move m){ return m & 0x3F; }
static inline int move_to(Move m){ return (m>>6)&0x3F; }
static inline int move_promo(Move m){ return (m>>12)&7; }
static inline int move_flags(Move m){ return m & 0xF0000; }

#endif
