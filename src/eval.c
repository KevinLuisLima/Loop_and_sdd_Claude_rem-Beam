#include "eval.h"
#include "attacks.h"
#include "types.h"
static uint64_t occ_color(Position *p,int c){ uint64_t r=0; for(int pt=0;pt<6;pt++) r|=p->bb[c][pt]; return r;}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* weight indices */
#define W_PAWN 0
#define W_KNIGHT 1
#define W_BISHOP 2
#define W_ROOK 3
#define W_QUEEN 4
#define W_MOB 5
#define W_PST 6 /* 6 + piece*64 + square */

static int pst_index(int piece,int sq){ return W_PST + piece*64 + sq; }

static int material_value(Weights*w,int piece){
    int idx[] = {W_PAWN,W_KNIGHT,W_BISHOP,W_ROOK,W_QUEEN};
    return (int)w->w[idx[piece]];
}
int eval_pos(Position *pos, Weights *w){
    if(pos->ply<0) {}
    int side=pos->side;
    /* material + pst */
    int score=0;
    for(int c=0;c<2;c++){
        int sign = (c==side)?1:-1;
        for(int pt=0;pt<6;pt++){
            if(pt==KING) continue;
            /* material */
            int cnt=__builtin_popcountll(pos->bb[c][pt]);
            score += sign*cnt*material_value(w,pt);
            /* pst */
            uint64_t b=pos->bb[c][pt];
            while(b){
                int s=__builtin_ctzll(b); b&=b-1;
                int sq = (c==WHITE)? s : (63-s); /* mirror? simple */
                score += sign * (int)w->w[pst_index(pt,sq)];
            }
        }
    }
    /* mobility: count attacks? simple: number of legal moves for side minus opponent approximated by knight/bishop/rook/queen mobility bitboards */
    /* count mobility for side */
    uint64_t all=0; for(int c=0;c<2;c++) for(int pt=0;pt<6;pt++) all|=pos->bb[c][pt];
    int mob=0;
    for(int pt=1;pt<=4;pt++){
        uint64_t b=pos->bb[side][pt];
        while(b){
            int s=__builtin_ctzll(b); b&=b-1;
            uint64_t a=0;
            if(pt==KNIGHT) a=knight_attacks(s);
            else if(pt==BISHOP) a=bishop_attacks(s,all);
            else if(pt==ROOK) a=rook_attacks(s,all);
            else if(pt==QUEEN) a=queen_attacks(s,all);
            mob += __builtin_popcountll(a & ~occ_color(pos,side));
        }
    }
    /* opponent mobility */
    int mobo=0;
    uint64_t all2=all;
    for(int pt=1;pt<=4;pt++){
        uint64_t b=pos->bb[side^1][pt];
        while(b){
            int s=__builtin_ctzll(b); b&=b-1;
            uint64_t a=0;
            if(pt==KNIGHT) a=knight_attacks(s);
            else if(pt==BISHOP) a=bishop_attacks(s,all2);
            else if(pt==ROOK) a=rook_attacks(s,all2);
            else if(pt==QUEEN) a=queen_attacks(s,all2);
            mobo += __builtin_popcountll(a & ~occ_color(pos,side^1));
        }
    }
    score += (int)w->w[W_MOB]*(mob-mobo);
    /* pawn structure simple: doubled/isolated penalties folded into 0 (omit) */
    return (pos->side==side)? score : -score;
}

static void fill_default(Weights*w){
    w->n=512;
    for(int i=0;i<512;i++) w->w[i]=0.0f;
    w->w[W_PAWN]=100; w->w[W_KNIGHT]=320; w->w[W_BISHOP]=330; w->w[W_ROOK]=500; w->w[W_QUEEN]=900;
    w->w[W_MOB]=5;
    /* pst defaults small */
    for(int pt=0;pt<5;pt++) for(int s=0;s<64;s++){
        /* center bonus example: knights central */
        int x=s%8,y=s/8;
        float c = 0;
        if(pt==KNIGHT){ c = (4.0f-fabsf(x-3.5f))* (4.0f-fabsf(y-3.5f))*2.0f; }
        if(pt==PAWN){ c = y*2; } /* advance for white; will mirror at eval? */
        w->w[pst_index(pt,s)] = c;
    }
}

int weights_load(Weights *w, const char *path){
    FILE *f=fopen(path,"rb");
    if(!f) return -1;
    /* header: version uint32, num uint32, type uint32 */
    unsigned v,n,t;
    if(fread(&v,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&n,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&t,4,1,f)!=1){fclose(f);return -1;}
    if(v!=1){fclose(f);return -1;}
    if(n>512){fclose(f);return -1;}
    size_t bytes;
    if(t==0) bytes=sizeof(float);
    else return -1;
    w->n=n;
    for(unsigned i=0;i<n;i++){
        float val;
        if(fread(&val,bytes,1,f)!=1){fclose(f);return -1;}
        w->w[i]=val;
    }
    fclose(f);
    return 0;
}

int weights_save(Weights *w, const char *path){
    FILE *f=fopen(path,"wb");
    if(!f) return -1;
    unsigned v=1,n=w->n,t=0;
    fwrite(&v,4,1,f); fwrite(&n,4,1,f); fwrite(&t,4,1,f);
    for(int i=0;i<w->n;i++){ float val=w->w[i]; fwrite(&val,4,1,f);}
    fclose(f);
    return 0;
}

void weights_default(Weights *w){
    w->n=512;
    fill_default(w);
}