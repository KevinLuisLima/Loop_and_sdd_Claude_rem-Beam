#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

typedef unsigned long long u64;

static inline int popcount64(u64 x){ return __builtin_popcountll(x); }
static inline int ctz64(u64 x){ return __builtin_ctzll(x); }

u64 square_bb(int sq);

#endif