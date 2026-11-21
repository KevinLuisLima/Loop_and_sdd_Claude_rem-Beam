#include "search.h"
#include "board.h"
#include "move.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

typedef struct { uint64_t key; int score, depth, flag, best_from, best_to, best_prom; } TTEntry;

static uint64_t rand64(int i){ uint64_t x=123456789ULL+i*987654321ULL; x^=x>>33; x*=0xff51afd7ed558ccdULL; x^=x>>33; return x; }
static uint64_t Z[12][64];
static int zinit=0;
static void init_z(void){ if(zinit) return; for(int p=0;p<12;p++) for(int s=0;s<64;s++) Z[p][s]=rand64(p*64+s); zinit=1;}

uint64_t board_hash(const Board*b){
    init_z();
    uint64_t h=0;
    for(int sq=0;sq<64;sq++){ int p=piece_at(b,sq); if(p>=0) h^=Z[p][sq];}
    if(b->side==BLACK) h^=0x9e3779b97f4a7c15ULL;
    h ^= (uint64_t)b->castling * 0x12345;
    h ^= (uint64_t)((b->ep>=0)?b->ep+1:0) * 0xabc;
    return h;
}
static long now_ms(void){ struct timeval tv; gettimeofday(&tv,NULL); return tv.tv_sec*1000L+tv.tv_usec/1000L;}
static int time_up(SearchContext*ctx){ if(ctx->budget_ms<=0) return 0; return (now_ms()-ctx->start_ms) >= ctx->budget_ms; }

static void score_moves(Board*b, MoveList*ml){
    for(int i=0;i<ml->count;i++){
        Move*m=&ml->moves[i];
        int victim = piece_at(b,m->to);
        int attacker = piece_at(b,m->from);
        int sc=0;
        if(victim>=0) sc = (victim%6+1)*100 - (attacker%6);
        if(m->prom>0) sc += 800;
        m->score=sc;
    }
}
static int cmp_move(const void*a,const void*b){
    const Move*ma=a; const Move*mb=b; return mb->score-ma->score;
}

static int qsearch(SearchContext*ctx, Board*b, int alpha,int beta){
    if(time_up(ctx)){ ctx->stop=1; return 0;}
    ctx->nodes++;
    int stand=evaluate(b,ctx->weights);
    if(stand>=beta) return beta;
    if(stand>alpha) alpha=stand;
    MoveList ml; gen_legal_moves(b,&ml); // captures? gen all but filter mentally costly; but okay small
    // keep captures only
    MoveList cap; cap.count=0;
    for(int i=0;i<ml.count;i++){ if(piece_at(b,ml.moves[i].to)>=0 || (b->ep==ml.moves[i].to && piece_at(b,ml.moves[i].from)%6==0)) { cap.moves[cap.count++]=ml.moves[i];} }
    score_moves(b,&cap);
    qsort(cap.moves,cap.count,sizeof(Move),cmp_move);
    for(int i=0;i<cap.count;i++){
        Board copy=*b; Undo u;
        if(!board_make_move(&copy,cap.moves[i].from,cap.moves[i].to,cap.moves[i].prom,&u)) continue;
        int sc=-qsearch(ctx,&copy,-beta,-alpha);
        board_unmake(&copy,&u);
        if(ctx->stop) return 0;
        if(sc>alpha){ alpha=sc; if(alpha>=beta) break; }
    }
    return alpha;
}

static int negamax(SearchContext*ctx, Board*b, int depth, int alpha,int beta){
    if(ctx->stop) return 0;
    if(time_up(ctx)){ ctx->stop=1; return 0;}
    if(depth<=0) return qsearch(ctx,b,alpha,beta);
    ctx->nodes++;
    // repetition/draw
    if(b->halfmove>=100) return 0;
    // TT
    uint64_t key=board_hash(b);
    TTEntry*tt=(TTEntry*)ctx->tt;
    TTEntry*te=&tt[key % ctx->tt_size];
    if(te->key==key && te->depth>=depth){
        if(te->flag==0) return te->score;
        if(te->flag==1 && te->score>alpha) alpha=te->score;
        if(te->flag==2 && te->score<beta) beta=te->score;
        if(alpha>=beta) return te->score;
    }
    MoveList ml; gen_legal_moves(b,&ml);
    if(ml.count==0){
        // checkmate or stalemate
        if(square_attacked(b, __builtin_ctzll((b->side==WHITE?b->bb[WK]:b->bb[BK])), 1-b->side)) return -30000-depth;
        return 0;
    }
    score_moves(b,&ml);
    // pv from tt
    if(te->key==key){ for(int i=0;i<ml.count;i++) if(ml.moves[i].from==te->best_from && ml.moves[i].to==te->best_to && ml.moves[i].prom==te->best_prom) { Move tmp=ml.moves[0]; ml.moves[0]=ml.moves[i]; ml.moves[i]=tmp; break;}}
    qsort(ml.moves+1, ml.count>1?ml.count-1:0, sizeof(Move), cmp_move);
    int best=-30000; Move bestm=ml.moves[0];
    int orig_alpha=alpha;
    for(int i=0;i<ml.count;i++){
        Board copy=*b; Undo u;
        if(!board_make_move(&copy, ml.moves[i].from, ml.moves[i].to, ml.moves[i].prom, &u)) continue;
        int sc=-negamax(ctx,&copy, depth-1, -beta, -alpha);
        board_unmake(&copy,&u);
        if(ctx->stop) return 0;
        if(sc>best){ best=sc; bestm=ml.moves[i]; }
        if(sc>alpha) alpha=sc;
        if(alpha>=beta) break;
    }
    // store TT
    te->key=key; te->score=best; te->depth=depth;
    te->best_from=bestm.from; te->best_to=bestm.to; te->best_prom=bestm.prom;
    if(best<=orig_alpha) te->flag=2; // upper
    else if(best>=beta) te->flag=1; // lower
    else te->flag=0;
    return best;
}

void search_init(SearchContext*ctx, Weights*w, int time_ms){
    memset(ctx,0,sizeof(*ctx));
    ctx->budget_ms=time_ms;
    ctx->weights=w;
    int elems = 1<<16; // 65536 entries ~ 2.5 MB?
    ctx->tt_size=elems;
    ctx->tt=calloc(elems,sizeof(TTEntry));
}
void search_free(SearchContext*ctx){ free(ctx->tt); ctx->tt=NULL;}

Move search_best_move(Board*b, SearchContext*ctx, int*score_out){
    ctx->start_ms=now_ms();
    ctx->stop=0; ctx->nodes=0;
    Move best= {0}; int score=0;
    // ensure at least depth 1 legal
    MoveList ml; gen_legal_moves(b,&ml);
    if(ml.count==1){ if(score_out) *score_out=0; return ml.moves[0];}
    if(ml.count==0){ if(score_out)*score_out=0; Move m={0}; return m;}
    for(int d=1; d<=64 && !ctx->stop; d++){
        int sc=negamax(ctx,b,d,-30000,30000);
        if(ctx->stop) break;
        score=sc;
        ctx->depth_reached=d;
        if(ctx->best_move.from==0 && ctx->best_move.to==0) ctx->best_move= ctx->best_move; // none
        // best move from TT root? Need capture within negamax? We didn't store ctx->best_move. Use te
        uint64_t key=board_hash(b);
        TTEntry*te=&((TTEntry*)ctx->tt)[key%ctx->tt_size];
        if(te->key==key){ Move m={te->best_from,te->best_to,te->best_prom,0}; best=m; }
    }
    if(score_out) *score_out=score;
    ctx->best_move=best;
    if(best.from==0 && best.to==0 && ml.count>0) best=ml.moves[0];
    return best;
}
