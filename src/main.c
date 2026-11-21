#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char**argv){
  char fen[1024];
  if(argc>1){
    strncpy(fen, argv[1], sizeof(fen)-1);
    fen[sizeof(fen)-1]=0;
  } else {
    if(!fgets(fen,sizeof(fen),stdin)) return 1;
    /* trim newline */
    fen[strcspn(fen,"\n")]=0;
  }
  char wpath[512];
  if(argc>2) strcpy(wpath,argv[2]);
  else {
    const char* e=getenv("WEIGHTS");
    if(e) strcpy(wpath,e); else strcpy(wpath,"weights.bin");
  }
  int time_ms = 7000;
  if(argc>3) time_ms=atoi(argv[3]);

  Board b;
  if(board_from_fen(&b,fen)!=0){ fprintf(stderr,"invalid FEN\n"); return 1; }
  /* validate king counts? basic: exactly one king each */
  if(__builtin_popcountll(b.bb[W_KING])!=1 || __builtin_popcountll(b.bb[B_KING])!=1){
    fprintf(stderr,"invalid position: king count\n"); return 2;
  }
  Move ml[MAX_MOVES];
  Board bcp=b;
  int n=gen_legal_moves(&bcp,ml);
  if(n==0){
    /* mate/stalemate */
    printf("none\n");
    return 0;
  }
  if(n==1){
    char ub[8]; move_to_uci(ml[0],ub); printf("%s\n",ub); return 0;
  }
  Weights w;
  char err[256];
  if(weights_load(wpath,&w,err,sizeof(err))!=0){
    fprintf(stderr,"weights error: %s\n",err);
    return 3;
  }
  SearchCtx c; search_init(&c);
  int depth=0, nodes=0;
  Move m=search_best_move(&b,&w,time_ms,&c,&depth,&nodes);
  if(m==0) m=ml[0];
  char ub[8]; move_to_uci(m,ub);
  fprintf(stderr,"depth=%d nodes=%d\n",depth,nodes);
  printf("%s\n",ub);
  return 0;
}