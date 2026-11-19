#include "chess.h"
#include <time.h>
#include <stdio.h>

static int negamax(Board b,int depth,int alpha,int beta){
    if(depth==0) return evaluate(&b);
    Move moves[MAX_MOVES];
    int n=gen_legal_moves(&b,moves);
    if(n==0){
        if(in_check(&b,b.side)) return -100000+ (10-depth);
        return 0;
    }
    int best=-1000000;
    for(int i=0;i<n;i++){
        Board cb=b; make_move(&cb,moves[i]);
        int score=-negamax(cb,depth-1,-beta,-alpha);
        if(score>best) best=score;
        if(score>alpha) alpha=score;
        if(alpha>=beta) break;
    }
    return best;
}

int search_bestmove(Board b,int movetime_ms,Move *out){
    Move moves[MAX_MOVES];
    int n=gen_legal_moves(&b,moves);
    if(n==0) return -1;
    if(n==1){ if(out) *out=moves[0]; return 0; }
    int bestIdx=0;
    clock_t start=clock();
    int d=1;
    while(1){
        double elapsed=(double)(clock()-start)*1000.0/CLOCKS_PER_SEC;
        if(d>1 && elapsed > (double)movetime_ms*0.7) break;
        if(d>8) break;
        int bestScore=-1000000;
        int curBest=bestIdx;
        for(int i=0;i<n;i++){
            Board cb=b; make_move(&cb,moves[i]);
            int score=-negamax(cb,d-1,-1000000,1000000);
            if(score>bestScore){ bestScore=score; curBest=i; }
        }
        bestIdx=curBest;
        d++;
    }
    if(out) *out=moves[bestIdx];
    fprintf(stderr, "profundidade_maxima=%d tempo_ms=%.1f\n", d-1, (double)(clock()-start)*1000.0/CLOCKS_PER_SEC);
    return 0;
}