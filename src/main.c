#include "chess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char promo_char(int p){
    switch(p){ case KNIGHT: return 'n'; case BISHOP: return 'b'; case ROOK: return 'r'; case QUEEN: return 'q'; default: return 0; }
}

int main(int argc,char**argv){
    if(argc>=2 && strcmp(argv[1],"uci")==0){
        printf("id name beam3\nid author beam3\nuciok\n"); fflush(stdout);
        char line[4096];
        Board b; board_startpos(&b);
        while(fgets(line,sizeof(line),stdin)){
            if(strncmp(line,"quit",4)==0) break;
            if(strncmp(line,"go",2)==0){
                Move m; int mt=2000;
                char *p=strstr(line,"movetime"); if(p) mt=atoi(p+9);
                eval_load_default();
                search_bestmove(b,mt,&m);
                printf("bestmove %c%d%c%d", 'a'+(m.from%8), m.from/8+1, 'a'+(m.to%8), m.to/8+1);
                char pc=promo_char(m.promo); if(pc) putchar(pc);
                printf("\n"); fflush(stdout);
            }
        }
        return 0;
    }
    if(argc>=3 && strcmp(argv[1],"fen")==0){
        Board b;
        if(board_from_fen(&b,argv[2])!=0){ board_startpos(&b); }
        if(argc>=4) eval_load(argv[3]); else eval_load_default();
        int mt = argc>=5 ? atoi(argv[4]) : 2000;
        Move m;
        search_bestmove(b, mt, &m);
        printf("%c%d%c%d", 'a'+(m.from%8), m.from/8+1, 'a'+(m.to%8), m.to/8+1);
        char pc=promo_char(m.promo);
        if(pc) putchar(pc);
        printf("\n");
        return 0;
    }
    if(argc>=4 && strcmp(argv[1],"apply")==0){
        Board b;
        if(board_from_fen(&b,argv[2])!=0){ board_startpos(&b); }
        char mv[16]; strcpy(mv,argv[3]);
        Move m; memset(&m,0,sizeof(m));
        m.from=(mv[1]-'1')*8+(mv[0]-'a');
        m.to=(mv[3]-'1')*8+(mv[2]-'a');
        if(strlen(mv)>=5){
            char p=mv[4];
            if(p=='n') m.promo=KNIGHT;
            if(p=='b') m.promo=BISHOP;
            if(p=='r') m.promo=ROOK;
            if(p=='q') m.promo=QUEEN;
        }
        Board cb=b;
        make_move(&cb,m);
        char fen[256]; board_to_fen(&cb,fen,sizeof(fen));
        printf("%s\n",fen);
        return 0;
    }
    if(argc>=3 && strcmp(argv[1],"perft")==0){
        Board b; board_startpos(&b);
        int d=atoi(argv[2]);
        printf("%llu\n",(unsigned long long)perft(b,d));
        return 0;
    }
    Board b; board_startpos(&b); eval_load_default();
    Move m; search_bestmove(b,2000,&m);
    printf("%c%d%c%d\n",'a'+(m.from%8), m.from/8+1, 'a'+(m.to%8), m.to/8+1);
    return 0;
}