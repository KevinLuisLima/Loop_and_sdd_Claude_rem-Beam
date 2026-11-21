#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define MAX_MOVES 256

typedef struct {
    int from;
    int to;
    int promoted; /* piece type 0..5 or -1 */
    int captured; /* piece 0..11 or -1 */
    int castle;   /* 0 none, 1 K-side, 2 Q-side */
    int ep;       /* 1 if en passant */
    unsigned int flags;
} Move;

typedef struct {
    int score;
} MoveExt;

#endif