#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "search.h"
#include "movegen.h"
#include "eval.h"

static void move_to_uci(Move m, char *buf){
    if(m.from<0){ strcpy(buf,"0000"); return; }
    buf[0]='a'+m.from%8; buf[1]='1'+m.from/8;
    buf[2]='a'+m.to%8; buf[3]='1'+m.to/8;
    if(m.promoted>=1 && m.promoted<=4){
        /* map piece type to uci char */
        char c;
        switch(m.promoted){ case 1: c='n'; break; case 2: c='b'; break; case 3: c='r'; break; case 4: c='q'; break; default: c='q'; }
        buf[4]=c; buf[5]=0;
    } else { buf[4]=0; }
}

int main(int argc,char**argv){
    const char *weights_path="models/model_0.bin";
    const char *fen=NULL;
    int time_ms=7000;
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--weights")==0 && i+1<argc){ weights_path=argv[++i]; }
        else if(strcmp(argv[i],"--time")==0 && i+1<argc){ time_ms=atoi(argv[++i]); }
        else { fen=argv[i]; }
    }
    if(!fen){
        /* read entire stdin */
        static char inbuf[4096];
        if(fgets(inbuf,sizeof(inbuf),stdin)) fen=inbuf;
    }
    Position p;
    if(!pos_from_fen(&p, fen?fen:"startpos")) { fprintf(stderr,"invalid fen\n"); return 1; }
    /* validate at least one king each? basic */
    if(!p.bb[W_K] || !p.bb[B_K]){ fprintf(stderr,"invalid position: missing king\n"); return 1; }
    Weights w;
    if(!weights_load(weights_path,&w)){ fprintf(stderr,"cannot load weights %s\n",weights_path); return 1; }
    SearchContext ctx; search_init_context(&ctx, time_ms, &w);
    /* if --time provided parse? skip */
    Move m=search_best_move(&p,&ctx,64);
    if(m.from<0){
        /* maybe only one? try first legal */
        Move moves[MAX_MOVES]; int n=gen_moves(&p,moves);
        int ln=0; Move leg[MAX_MOVES];
        for(int i=0;i<n;i++){ Position p2=p; Undo u; pos_make_move(&p2,&moves[i],&u); if(!in_check(&p2,p.side)){ leg[ln++]=moves[i]; } pos_unmake_move(&p2,&moves[i],&u);}
        if(ln>0) m=leg[0];
        else { fprintf(stderr,"no legal moves\n"); weights_free(&w); return 2; }
    }
    char buf[16]; move_to_uci(m,buf);
    printf("%s\n",buf);
    fflush(stdout);
    weights_free(&w);
    free(ctx.tt);
    return 0;
}