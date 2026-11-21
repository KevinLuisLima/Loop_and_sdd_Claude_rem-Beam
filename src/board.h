#ifndef BOARD_H
#define BOARD_H

#include "types.h"

typedef struct {
  Move move;
  int captured;
  int castling;
  int ep_square;
  int halfmove;
  int side;
  int fullmove;
} Undo;

typedef struct {
  U64 bb[12];          /* bitboards per piece */
  int side;
  int castling;
  int ep_square;
  int halfmove;
  int fullmove;
  /* undo stack */
  Undo history[1024];
  int hist_top;
} Board;

void board_init(Board *b);
int  board_from_fen(Board *b, const char *fen); /* returns 0 success */
int  board_to_fen(const Board *b, char *buf, size_t sz);
int  board_make_move(Board *b, Move m); /* applies move, pushes undo; 1 if ok */
void board_unmake(Board *b);
int  board_in_check(const Board *b, int side);
int  board_legal_move(const Board *b, Move m);
int  board_game_over(const Board *b);
int  board_repetition(const Board *b);
int  board_insufficient_material(const Board *b);
int  board_side_to_move(const Board *b);

/* occupancy helpers */
U64 bb_white(const Board *b);
U64 bb_black(const Board *b);
U64 bb_all(const Board *b);
U64 bb_pieces(const Board *b, int piece);
int square_of_piece(const Board *b, int piece, int idx); /* not needed */
int piece_on(const Board *b, int sq);

#endif