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
  int captured_sq;
} Undo;

typedef struct {
  U64 bb[12];
  int side;
  int castling;
  int ep_square;
  int halfmove;
  int fullmove;
  Undo history[1024];
  int hist_top;
} Board;

void board_init(Board *b);
int  board_from_fen(Board *b, const char *fen);
int  board_to_fen(const Board *b, char *buf, size_t sz);
int  board_make_move(Board *b, Move m);
void board_unmake(Board *b);
int  board_in_check(const Board *b, int side);
int  board_legal_move(const Board *b, Move m);
int  board_game_over(const Board *b);
int  board_repetition(const Board *b);
int  board_insufficient_material(const Board *b);
int  board_side_to_move(const Board *b);

U64 bb_white(const Board *b);
U64 bb_black(const Board *b);
U64 bb_all(const Board *b);
U64 bb_pieces(const Board *b, int piece);
int piece_on(const Board *b, int sq);
unsigned long long perft(Board b, int depth);
#endif
