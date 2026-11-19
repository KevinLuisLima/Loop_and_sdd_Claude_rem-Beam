#include "chess.h"
#include "weights.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void move_to_uci(const Move *m, char *buf){
    buf[0]='a'+(m->from%8); buf[1]='1'+(m->from/8);
    buf[2]='a'+(m->to%8); buf[3]='1'+(m->to/8);
    buf[4]=0;
    if(m->prom>0){
        /* 1 queen 2 rook 3 bishop 4 knight */
        const char promo[5]="qrbn";
        buf[4]=promo[m->prom-1>3?0:m->prom-1];
        buf[5]=0;
    }
}

int main(int argc, char **argv){
    char fenbuf[1024]; fenbuf[0]=0;
    const char *wpath=NULL;
    /* parse */
    char parts[1024]; parts[0]=0;
    int first=1;
    for(int i=1;i<argc;i++){
        if(strstr(argv[i],".bin")){
            wpath=argv[i];
        } else {
            if(!first) strcat(parts," ");
            strcat(parts,argv[i]);
            first=0;
        }
    }
    if(parts[0]==0){
        /* read stdin */
        if(fgets(parts,sizeof(parts),stdin)){}
    }
    /* applymove mode */
    if(argc>=4 && strcmp(argv[1],"--applymove")==0){
        strncpy(fenbuf, argv[2], sizeof(fenbuf)-1);
        char mstr[16]; strcpy(mstr, argv[3]);
        Position p; memset(&p,0,sizeof(p)); p.ep=-1; for(int i=0;i<64;i++) p.board[i]=12;
        pos_from_fen(&p, fenbuf);
        /* parse move */
        Move mv; mv.from=(mstr[0]-'a')+(mstr[1]-'1')*8; mv.to=(mstr[2]-'a')+(mstr[3]-'1')*8; mv.prom=0; mv.flags=0;
        if(mstr[4]=='q') { mv.prom=1; }
        if(mstr[4]=='r') { mv.prom=2; }
        if(mstr[4]=='b') { mv.prom=3; }
        if(mstr[4]=='n') { mv.prom=4; }
        make_move(&p,&mv);
        char buf[512]; pos_to_fen(&p,buf,sizeof(buf));
        printf("%s\n", buf);
        return 0;
    }
    /* fen may include leading command? use as is */
    strncpy(fenbuf, parts, sizeof(fenbuf)-1);
    fenbuf[sizeof(fenbuf)-1]=0;

    float weights[NUM_WEIGHTS];
    for(int i=0;i<NUM_WEIGHTS;i++) weights[i]=1.0f;
    const char *env=getenv("CHESS_WEIGHTS");
    const char *tryPaths[4]={wpath, env, "model.bin", "weights.bin"};
    for(int i=0;i<4;i++){
        if(tryPaths[i] && load_weights(tryPaths[i], weights, NUM_WEIGHTS)){
            break;
        }
    }

    Position p;
    memset(&p,0,sizeof(p));
    p.ep=-1;
    if(!pos_from_fen(&p, fenbuf) || !p.valid){
        pos_init_start(&p);
    }

    Move m = find_best_move(&p, weights, 7000, 12);
    if(m.from<0){
        /* contingency: first legal */
        Move moves[512];
        int n=generate_pseudo_moves(&p,moves);
        n=filter_legal(&p,moves,n);
        if(n>0) m=moves[0];
    }
    char buf[16];
    if(m.from<0) strcpy(buf,"0000");
    else move_to_uci(&m,buf);
    printf("%s\n", buf);
    fflush(stdout);
    return 0;
}