#include "search.h"
#include "movegen.h"
#include "position.h"
#include "bitboard.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

long now_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return (long)(ts.tv_sec*1000 + ts.tv_nsec/1000000);
}

void search_init_context(SearchContext *ctx,int time_ms, Weights *w){
    memset(ctx,0,sizeof(*ctx));
    ctx->max_time_ms=time_ms;
    ctx->start_ms=now_ms();
    ctx->tt_size=1<<18; /* 262144 entries ~ 16MB? Wait too big; reduce */
    ctx->tt_size=1<<16;
    ctx->tt=calloc(ctx->tt_size, sizeof(TTEntry));
    ctx->weights=w;
}

static int time_up(SearchContext *ctx){
    if(ctx->stop) return 1;
    long el = now_ms()-ctx->start_ms;
    if(el >= ctx->max_time_ms*95/100){ ctx->stop=1; return 1; }
    return 0;
}

static int score_move_order(Move *m){
    int s=0;
    if(m->captured!=EMPTY){
        int victim = m->captured%6;
        static const int vals[6]={100,320,330,500,900,0};
        s = 10000 + vals[victim]*10;
    }
    if(m->promoted>=0) s+=9000;
    return s;
}

static void order_moves(Move *moves,int n,int *scores){
    for(int i=0;i<n;i++) scores[i]=score_move_order(moves+i);
    /* insertion sort */
    for(int i=1;i<n;i++){
        Move mt=moves[i]; int st=scores[i]; int j=i-1;
        while(j>=0 && scores[j]<st){ moves[j+1]=moves[j]; scores[j+1]=scores[j]; j--; }
        moves[j+1]=mt; scores[j+1]=st;
    }
}

static int quiesce(Position *p, SearchContext *ctx, int alpha, int beta){
    if(time_up(ctx)) return 0;
    ctx->nodes++;
    int stand = evaluate(p,ctx->weights);
    if(stand>=beta) return beta;
    if(stand>alpha) alpha=stand;
    /* TT not in q */
    Move moves[MAX_MOVES];
    int n=gen_captures(p,moves);
    int scores[MAX_MOVES];
    order_moves(moves,n,scores);
    /* filter legal inside gen already */
    for(int i=0;i<n;i++){
        Position p2=*p;
        Undo u;
        pos_make_move(&p2,&moves[i],&u);
        int side_before=p->side;
        if(in_check(&p2, side_before)){ pos_unmake_move(&p2,&moves[i],&u); continue; }
        int sc = -quiesce(&p2,ctx,-beta,-alpha);
        pos_unmake_move(&p2,&moves[i],&u);
        if(ctx->stop) return 0;
        if(sc>alpha){ alpha=sc; }
        if(alpha>=beta) return beta;
    }
    return alpha;
}

static int alphabeta(Position *p, SearchContext *ctx, int depth, int alpha, int beta){
    if(time_up(ctx)) return 0;
    ctx->nodes++;
    /* repetition / 50-move / terminal */
    if(p->halfmove>=100) return 0;
    /* TT */
    u64 key=pos_hash(p);
    int ti=key % ctx->tt_size;
    if(ctx->tt[ti].used && ctx->tt[ti].key==key && ctx->tt[ti].depth>=depth){
        if(ctx->tt[ti].flag==0) return ctx->tt[ti].score;
        if(ctx->tt[ti].flag==1 && ctx->tt[ti].score>=beta) return ctx->tt[ti].score;
        if(ctx->tt[ti].flag==2 && ctx->tt[ti].score<=alpha) return ctx->tt[ti].score;
    }
    if(depth<=0) return quiesce(p,ctx,alpha,beta);
    Move moves[MAX_MOVES];
    int n=gen_moves(p,moves);
    /* legal filtering already in gen_moves */
    if(n==0){
        if(in_check(p,p->side)) return -100000; /* mate score approx depth? */
        return 0; /* stalemate */
    }
    /* if only one move */
    int scores[MAX_MOVES];
    order_moves(moves,n,scores);
    /* TT best move first */
    if(ctx->tt[ti].used && ctx->tt[ti].key==key){
        for(int i=0;i<n;i++){
            if(moves[i].from==ctx->tt[ti].best.from && moves[i].to==ctx->tt[ti].best.to && moves[i].promoted==ctx->tt[ti].best.promoted){
                /* move to front */
                Move t=moves[i]; for(int j=i;j>0;j--) moves[j]=moves[j-1]; moves[0]=t;
                int ts=scores[i]; for(int j=i;j>0;j--) scores[j]=scores[j-1]; scores[0]=ts;
                break;
            }
        }
    }
    int best=-1000000;
    Move bestMove=moves[0];
    int alpha0=alpha;
    for(int i=0;i<n;i++){
        Position p2=*p;
        Undo u;
        pos_make_move(&p2,&moves[i],&u);
        int side_before=p->side;
        if(in_check(&p2, side_before)){ pos_unmake_move(&p2,&moves[i],&u); continue; }
        int sc = -alphabeta(&p2,ctx,depth-1,-beta,-alpha);
        pos_unmake_move(&p2,&moves[i],&u);
        if(ctx->stop) return 0;
        if(sc>best){ best=sc; bestMove=moves[i]; }
        if(sc>alpha) alpha=sc;
        if(alpha>=beta){
            /* store tt cutoff */
            break;
        }
    }
    /* store TT */
    ctx->tt[ti].used=1;
    ctx->tt[ti].key=key;
    ctx->tt[ti].depth=depth;
    ctx->tt[ti].score=best;
    ctx->tt[ti].best=bestMove;
    if(best<=alpha0) ctx->tt[ti].flag=2;
    else if(best>=beta) ctx->tt[ti].flag=1;
    else ctx->tt[ti].flag=0;
    return best;
}

Move search_best_move(Position *p, SearchContext *ctx, int max_depth){
    Move best=p[0].bb[0]? (Move){0}: (Move){0}; /* placeholder */
    /* ensure at least one legal move */
    Move moves[MAX_MOVES];
    int n=gen_moves(p,moves);
    /* filter legal */
    Move legal[MAX_MOVES]; int ln=0;
    for(int i=0;i<n;i++){
        Position p2=*p; Undo u; pos_make_move(&p2,&moves[i],&u);
        if(!in_check(&p2,p->side)) legal[ln++]=moves[i];
        pos_unmake_move(&p2,&moves[i],&u);
    }
    if(ln==0){ /* no legal move */
        best.from=-1; return best;
    }
    best=legal[0];
    ctx->start_ms=now_ms();
    ctx->stop=0;
    for(int d=1; d<=max_depth; d++){
        if(time_up(ctx)) break;
        ctx->depth_reached=d;
        /* reset for ID? keep TT */
        int alpha=-1000000, beta=1000000;
        Position pcopy=*p;
        int sc=alphabeta(&pcopy,ctx,d,alpha,beta);
        /* find best move from TT or re-evaluate? TT root stored best */
        u64 key=pos_hash(p);
        int ti=key%ctx->tt_size;
        if(ctx->tt[ti].used && ctx->tt[ti].key==key) best=ctx->tt[ti].best;
        (void)sc;
        if(ctx->stop) break;
    }
    /* if no best? fallback first legal */
    if(best.from<0||best.from>63) best=legal[0];
    return best;
}