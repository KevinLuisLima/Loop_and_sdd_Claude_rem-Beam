#include "chess.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static float W[WEIGHT_COUNT];
static int loaded=0;

static void defaults(void){
    for(int i=0;i<WEIGHT_COUNT;i++) W[i]=0.0f;
    /* material centipawns */
    /* layout: 0..5 material, 6..6+6*64 PST, etc. We'll use simple: material weights at 0..5, PST at 100.. */
    W[PAWN]=100; W[KNIGHT]=320; W[BISHOP]=330; W[ROOK]=500; W[QUEEN]=900; W[KING]=0;
    /* PST base small */
    for(int t=0;t<6;t++) for(int s=0;s<64;s++){
        W[6 + t*64 + s] = 0.0f;
    }
    /* mobility term index 390, doubled 391, isolated 392, king safety 393 */
    W[390]=1; W[391]=-10; W[392]=-15; W[393]=-2;
    loaded=1;
}

int eval_load_default(void){ if(!loaded) defaults(); return 0; }

int eval_load(const char *path){
    FILE *f=fopen(path,"rb");
    if(!f) return -1;
    /* header: magic u32, version u32, count u32 */
    uint32_t magic,ver,count;
    if(fread(&magic,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&ver,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&count,4,1,f)!=1){fclose(f);return -1;}
    /* little-endian assumed */
    if(count!=WEIGHT_COUNT){fclose(f);return -1;}
    if(ver==0){fclose(f);return -1;}
    /* magic check loose */
    (void)magic;
    size_t rd=fread(W,sizeof(float),WEIGHT_COUNT,f);
    /* validate finite */
    for(int i=0;i<WEIGHT_COUNT;i++){
        if(!isfinite(W[i])){ fclose(f); return -1; }
    }
    if(rd!=WEIGHT_COUNT){ fclose(f); return -1; }
    loaded=1;
    fclose(f);
    return 0;
}

static int count_pieces(const Board*b,int color,int type){ return __builtin_popcountll(b->bb[color][type]); }

int evaluate(const Board *b){
    if(!loaded) defaults();
    int stm=b->side; int opp=stm^1;
    /* material */
    float score=0;
    for(int t=0;t<5;t++){ /* exclude king */
        score += W[t]*count_pieces(b,stm,t);
        score -= W[t]*count_pieces(b,opp,t);
    }
    /* PST */
    for(int color=0;color<2;color++){
        for(int t=0;t<6;t++){
            uint64_t bb=b->bb[color][t];
            while(bb){ int sq=__builtin_ctzll(bb); bb&=bb-1;
                float v=W[6+t*64+sq];
                if(color==stm) score += v; else score -= v;
            }
        }
    }
    /* mobility simple: number of legal moves */
    Board cb=*b;
    Move mv[MAX_MOVES];
    /* moves for stm */
    cb.side=stm;
    int nstm=gen_legal_moves(&cb,mv);
    cb.side=opp;
    int nopp=gen_legal_moves(&cb,mv);
    score += W[390]*(nstm-nopp);
    /* doubled/isolated pawns */
    /* simple pawn structure for stm */
    /* doubled */
    for(int color=0;color<2;color++){
        /* count per file */
        int files[8]={0};
        uint64_t p=b->bb[color][PAWN];
        uint64_t t=p;
        while(t){ int sq=__builtin_ctzll(t); t&=t-1; files[sq&7]++; }
        int doubled=0, isolated=0;
        for(int f=0;f<8;f++){ if(files[f]>1) doubled += files[f]-1; if(files[f]>0){ int nbr=(f>0?files[f-1]:0)+(f<7?files[f+1]:0); if(nbr==0) isolated += files[f]; } }
        float pen = W[391]*doubled + W[392]*isolated;
        if(color==stm) score += pen; else score -= pen;
    }
    /* relative to side to move already stm positive */
    return (int)score;
}