#include "board.h"
#include "attacks.h"
#include "movegen.h"
#include "eval.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int T=0, P=0;
#define CHECK(cond) do{ T++; if(cond){ P++; } else { printf("FAIL at %s:%d: %s\n",__FILE__,__LINE__,#cond);} }while(0)

int main(void){
  attacks_init();
  // perft initial d5
  {
    Board b; board_init(&b);
    unsigned long long n=perft(b,5);
    CHECK(n==4865609ULL);
  }
  // Kiwipete
  {
    Board b; board_from_fen(&b,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    unsigned long long n=perft(b,4);
    CHECK(n==4085659ULL);
  }
  // position 3
  {
    Board b; board_from_fen(&b,"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    unsigned long long n=perft(b,5);
    CHECK(n==674624ULL);
  }
  // attacks basic
  {
    Board b; board_from_fen(&b,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // rook from a1 attacks a-file/rank no wrap
    CHECK(is_square_attacked(&b, 0+8*7?7:0, WHITE)==0);
  }
  // legal moves count initial
  {
    Board b; board_init(&b);
    Move ml[MAX_MOVES]; Board b2=b;
    int n=gen_legal_moves(&b2,ml);
    CHECK(n==20);
  }
  printf("%d/%d testes passaram\n", P, T);
  return (P==T)?0:1;
}
