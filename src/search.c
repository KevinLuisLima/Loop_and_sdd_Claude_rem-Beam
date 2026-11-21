#include "search.h"
#include "movegen.h"
#include <sys/time.h>
#include <stdlib.h>

static long long now_ms(void){ struct timeval tv; gettimeofday(&tv,NULL); return tv.tv_sec*1000LL+tv.tv_usec/1000LL; }
void search_init(SearchCtx*c){ c->nodes=0; c->stopped=0; c->start_ms=0; c->limit_ms=0; }
static int time_up(SearchCtx*c){
  if(c->stopped) return 1;
  if(now_ms() >= c->limit_ms){ c->stopped=1; return 1; }
  return 0;
}

static int alphabeta(Board*b,const Weights*w,SearchCtx*c,int depth,int alpha,int beta){
  if(time_up(c)) return 0;
  c->nodes++;
  if(depth<=0) return quiescence(b,w,c,alpha,beta,0);
  Move ml[MAX_MOVES];
  Board b2=*b;
  int n=gen_legal_moves(&b2,ml);
  if(n==0){
    if(board_in_check(b,b->side)) return -100000+ (MAX_PLY-depth);
    return 0;
  }
  int best=-100000000;
  for(int i=0;i<n;i++){
    if(time_up(c)) break;
    Board cb=*b;
    if(!board_make_move(&cb,ml[i])) continue;
    int score=-alphabeta(&cb,w,c,depth-1,-beta,-alpha);
    board_unmake(&cb);
    if(score>best) best=score;
    if(score>alpha) alpha=score;
    if(alpha>=beta) break;
  }
  if(best==-100000000) return 0;
  return best;
}

int quiescence(Board*b,const Weights*w,SearchCtx*c,int alpha,int beta,int side_root){
  (void)side_root;
  if(time_up(c)) return 0;
  c->nodes++;
  int standpat=eval_score(b,w);
  if(standpat>=beta) return beta;
  if(standpat>alpha) alpha=standpat;
  Move ml[MAX_MOVES];
  Board b2=*b;
  int n=gen_legal_moves(&b2,ml);
  for(int i=0;i<n;i++){
    if(time_up(c)) break;
    Move m=ml[i];
    if(!move_is_capture(b,m)) continue;
    Board cb=*b;
    if(!board_make_move(&cb,m)) continue;
    int score=-quiescence(&cb,w,c,-beta,-alpha,0);
    board_unmake(&cb);
    if(score>=beta) return beta;
    if(score>alpha) alpha=score;
  }
  return alpha;
}

Move search_best_move(Board*b,const Weights*w,int time_ms,SearchCtx*c,int*depth_out,int*nodes_out){
  search_init(c);
  c->start_ms=now_ms();
  c->limit_ms=c->start_ms + (time_ms>0?time_ms:1000);
  Move ml[MAX_MOVES];
  Board b2=*b;
  int n=gen_legal_moves(&b2,ml);
  if(n==0) return 0;
  if(n==1){ if(depth_out)*depth_out=0; if(nodes_out)*nodes_out=0; return ml[0]; }
  Move best_move=ml[0];
  for(int depth=1; depth<=32; depth++){
    if(time_up(c)) break;
    int alpha=-100000000,beta=100000000;
    Move cur_best=best_move;
    int cur_best_score=-100000000;
    int stopped_before=c->stopped;
    for(int i=0;i<n;i++){
      if(time_up(c)) break;
      Board cb=*b;
      if(!board_make_move(&cb,ml[i])) continue;
      int score=-alphabeta(&cb,w,c,depth-1,-beta,-alpha);
      board_unmake(&cb);
      if(score>cur_best_score){ cur_best_score=score; cur_best=ml[i]; }
      if(score>alpha) alpha=score;
    }
    if(!c->stopped && !stopped_before){
      best_move=cur_best;
      if(depth_out) *depth_out=depth;
    } else break;
  }
  if(nodes_out) *nodes_out=c->nodes;
  if(depth_out && *depth_out==0) *depth_out=1;
  if(best_move==0) best_move=ml[0];
  return best_move;
}
