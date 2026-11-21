#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "move.h"
#include "search.h"
#include "eval.h"


static uint64_t perft(Board*b,int depth){
    if(depth==0) return 1;
    MoveList ml; gen_legal_moves(b,&ml);
    if(depth==1) return ml.count;
    uint64_t n=0;
    for(int i=0;i<ml.count;i++){
        Board c=*b; Undo u;
        if(board_make_move(&c,ml.moves[i].from,ml.moves[i].to,ml.moves[i].prom,&u)){
            n+=perft(&c,depth-1);
            board_unmake(&c,&u);
        }
    }
    return n;
}

int main(int argc,char**argv){
    const char*fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int time_ms=1000;
    const char*model=NULL;
    int perft_depth=-1;
    int stats=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--fen")&&i+1<argc) fen=argv[++i];
        else if(!strcmp(argv[i],"--time")&&i+1<argc) time_ms=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--model")&&i+1<argc) model=argv[++i];
        else if(!strcmp(argv[i],"--perft")&&i+1<argc) perft_depth=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--stats")) stats=1;
    }
    Board b;
    if(board_load_fen(&b,fen)!=0){ fprintf(stderr,"Invalid FEN\n"); return 1; }
    if(perft_depth>0){ printf("%llu\n",(unsigned long long)perft(&b,perft_depth)); return 0; }
    Weights w={0};
    if(model){ if(eval_load(&w,model)!=0){ fprintf(stderr,"Weight file error\n"); return 2; } }
    else {
        // try default
    }
    SearchContext ctx; search_init(&ctx,&w,time_ms*7/10);
    Move m=search_best_move(&b,&ctx,NULL);
    search_free(&ctx); eval_free(&w);
    if(m.from==0 && m.to==0){
        // contingency: first legal
        MoveList ml; gen_legal_moves(&b,&ml);
        if(ml.count==0){ fprintf(stderr,"No legal moves\n"); return 3; }
        m=ml.moves[0];
    }
    char uci[8]; move_to_uci(m,uci);
    printf("%s\n",uci);
    if(stats) fprintf(stderr,"depth=%d\n", ctx.depth_reached);
    return 0;
}
