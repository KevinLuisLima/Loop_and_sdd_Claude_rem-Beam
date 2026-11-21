#ifndef ATTACKS_H
#define ATTACKS_H
#include "types.h"
uint64_t knight_attacks(int sq);
uint64_t king_attacks(int sq);
uint64_t pawn_attacks(int color, int sq);
uint64_t bishop_attacks(int sq, uint64_t occ);
uint64_t rook_attacks(int sq, uint64_t occ);
uint64_t queen_attacks(int sq, uint64_t occ);
int square_attacked(Position *pos, int sq, int by_color);
void init_attacks(void);
#endif