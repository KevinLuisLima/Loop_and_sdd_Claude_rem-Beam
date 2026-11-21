#ifndef ATTACKS_H
#define ATTACKS_H

#include "types.h"
#include "board.h"

void attacks_init(void);
U64 pawn_attacks(int side, int sq);      /* side of pawn owner */
U64 knight_attacks(int sq);
U64 king_attacks(int sq);
U64 sliding_attacks(int sq, U64 blockers, int piece_type); /* ROOK/BISHOP/QUEEN */
int is_square_attacked(const Board *b, int sq, int by_side);

#endif