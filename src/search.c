#include "search.h"
#include "board.h"
#include "eval.h"
#include "attacks.h"
#include "tt.h"
#include "types.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}

static uint64_t pos_hash(Position *pos){
    uint64_t h=0;
    for(int c=0;c<2;c++) for(int p=0;p<6;p++) h ^= pos->bb[c][p] * (0x9e3779b97f4a7c15ULL*(c*6+p+1));
    h ^= (uint64_t)pos->side*0x12345;
    h ^= (uint64_t)pos->castling<<32;
    h ^= (uint64_t)(pos->ep_square+1)<<40;
    return h;
}

static int quiesce(SearchContext *ctx, Position *pos, int alpha, int beta){
    if(ctx->stop) return 0;
    double t=now_sec();
    if(t-ctx->start_time > ctx->time_limit){ ctx->stop=1; return 0;}
    int stand=eval_pos(pos, ctx->weights);
    if(stand >= beta) return beta;
    if(stand > alpha) alpha=stand;
    Move moves[MAXMOVES];
    int n=legal_moves(pos,moves);
    /* only captures */
    for(int i=0;i<n;i++){
        int fl=MOVE_FLAGS(moves[i]);
        if(!(fl==FLAG_CAPTURE || fl==FLAG_EP || fl==FLAG_PROMO)) continue;
        make_move(pos,moves[i]);
        int sc=-quiesce(ctx,pos,-beta,-alpha);
        unmake_move(pos);
        if(ctx->stop) return 0;
        if(sc > alpha){ alpha=sc; if(alpha>=beta) break;}
    }
    return alpha;
}

static int alphabeta(SearchContext *ctx, Position *pos, int depth, int alpha, int beta){
    if(ctx->stop) return 0;
    double t=now_sec();
    if(t-ctx->start_time > ctx->time_limit){ ctx->stop=1; return 0;}
    ctx->nodes++;
    if(depth<=0){
        return quiesce(ctx,pos,alpha,beta);
    }
    /* game over check */
    Move moves[MAXMOVES];
    int n=legal_moves(pos,moves);
    if(n==0){
        if(in_check(pos,pos->side)) return -100000 + (MAXPLY - pos->ply); /* mate */
        return 0; /* stalemate */
    }
    /* TT */
    uint64_t key=pos_hash(pos);
    TTEntry *te=tt_probe(ctx->tt,key);
    if(te && te->depth>=depth){
        if(te->flag==0) return te->score;
        if(te->flag==1 && te->score>=beta) return te->score;
        if(te->flag==2 && te->score<=alpha) return te->score;
    }
    /* move ordering: captures first */
    /* simple insertion */
    for(int i=0;i<n;i++){
        /* score */
    }
    int bestscore=-1000000;
    int bestmove=-1;
    int alpha0=alpha;
    for(int i=0;i<n;i++){
        make_move(pos,moves[i]);
        int sc=-alphabeta(ctx,pos,depth-1,-beta,-alpha);
        unmake_move(pos);
        if(ctx->stop) return 0;
        if(sc>bestscore){ bestscore=sc; bestmove=moves[i];}
        if(sc>alpha) alpha=sc;
        if(alpha>=beta) break;
    }
    /* store */
    int flag=0;
    if(bestscore<=alpha0) flag=2;
    else if(bestscore>=beta) flag=1;
    tt_store(ctx->tt,key,depth,bestscore,flag,bestmove);
    return bestscore;
}

void search_init_context(SearchContext *ctx, Position *pos, TranspositionTable *tt, Weights *w, double time_limit){
    ctx->pos=pos; ctx->tt=tt; ctx->weights=w;
    ctx->nodes=0; ctx->start_time=now_sec(); ctx->time_limit=time_limit;
    ctx->stop=0; ctx->max_depth=64; ctx->depth_reached=0;
}

Move search_bestmove(SearchContext *ctx, int depth_limit, double time_limit){
    ctx->start_time=now_sec();
    ctx->time_limit=time_limit;
    ctx->stop=0;
    ctx->nodes=0;
    Position *pos=ctx->pos;
    Move best=0;
    Move moves[MAXMOVES];
    int n=legal_moves(pos,moves);
    if(n==0) return 0;
    if(n==1) return moves[0];
    for(int d=1; d<=depth_limit; d++){
        int alpha=-1000000, beta=1000000;
        int bestscore=-1000000; Move bmove=moves[0];
        for(int i=0;i<n;i++){
            make_move(pos,moves[i]);
            int sc=-alphabeta(ctx,pos,d-1,-beta,-alpha);
            unmake_move(pos);
            if(ctx->stop) break;
            if(sc>bestscore){ bestscore=sc; bmove=moves[i];}
            if(sc>alpha) alpha=sc;
        }
        if(ctx->stop) break;
        best=bmove;
        ctx->depth_reached=d;
    }
    if(best==0) best=moves[0];
    return best;
}