#include "board.h"
#include "attacks.h"
#include "types.h"
#include <stdio.h>
uint64_t perft(Position*p,int d){ if(d==0) return 1; Move m[256]; int n=legal_moves(p,m); uint64_t t=0; for(int i=0;i<n;i++){ make_move(p,m[i]); t+=perft(p,d-1); unmake_move(p);} return t; }
int main(){ init_attacks(); Position p; pos_init(&p); printf("d1 %llu\n",perft(&p,1)); printf("d2 %llu\n",perft(&p,2)); printf("d3 %llu\n",perft(&p,3)); printf("d4 %llu\n",perft(&p,4)); printf("d5 %llu\n",perft(&p,5)); return 0; }
