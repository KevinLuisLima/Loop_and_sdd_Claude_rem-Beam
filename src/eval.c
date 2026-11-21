#include "eval.h"
#include "position.h"
#include "bitboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NW 394
#define IDX_MAT(p) (p) /* 0..4 for pawn..queen (piece type index) */
#define IDX_PST(piece) (5 + (piece)*64)
#define IDX_MOB 389
#define IDX_DBL 390
#define IDX_ISO 391
#define IDX_PAS 392
#define IDX_KSAF 393

int weights_load(const char *path, Weights *out){
    FILE *f=fopen(path,"rb");
    if(!f) return 0;
    char magic[4]; if(fread(magic,1,4,f)!=4){ fclose(f); return 0; }
    if(memcmp(magic,"CHW1",4)!=0){ fclose(f); return 0; }
    unsigned int version,n,type;
    /* little-endian read */
    unsigned char b[4];
    if(fread(b,1,4,f)!=4){ fclose(f); return 0; } version=b[0]|(b[1]<<8)|(b[2]<<16)|((unsigned)b[3]<<24);
    if(fread(b,1,4,f)!=4){ fclose(f); return 0; } n=b[0]|(b[1]<<8)|(b[2]<<16)|((unsigned)b[3]<<24);
    if(fread(b,1,1,f)!=1){ fclose(f); return 0; } type=b[0];
    (void)version;
    if(n!=NW){ fclose(f); return 0; }
    if(type!=0){ fclose(f); return 0; } /* float32 */
    out->n=n;
    out->w=malloc(n*sizeof(float));
    if(!out->w){ fclose(f); return 0; }
    size_t rd=fread(out->w,sizeof(float),n,f);
    if(rd!=n){ free(out->w); out->w=NULL; fclose(f); return 0; }
    /* validate no nan/inf */
    for(int i=0;i<(int)n;i++){ if(!isfinite(out->w[i])){ free(out->w); out->w=NULL; fclose(f); return 0;} }
    fclose(f);
    return 1;
}

void weights_free(Weights *w){ if(w){ free(w->w); w->w=NULL; w->n=0; } }

/* We'll implement evaluate without extra includes */
int evaluate(Position *p, Weights *w){
    if(!w||!w->w) return 0;
    float sc=0.0f; /* white score */
    /* material + PST */
    for(int pc=0;pc<12;pc++){
        int ptype=pc%6;
        if(ptype==5) continue; /* king no material */
        u64 bb=p->bb[pc];
        int isWhite = pc<6;
        float mat = (ptype<5)? w->w[IDX_MAT(ptype)] : 0.0f;
        /* PST for piece */
        while(bb){
            int sq=ctz64(bb); bb&=bb-1;
            int idxsq = isWhite ? sq : (sq^56);
            float v = mat + w->w[IDX_PST(ptype)+idxsq];
            if(isWhite) sc += v; else sc -= v;
        }
    }
    /* mobility: count attacks to empty squares for knights/bishops/rooks/queens approximated by rays to empty */
    /* white */
    /* simple: count legal-ish moves for each side using ray counts (pseudo) */
    /* We'll compute mobility as number of pseudo moves / 10 to keep cheap-ish? Movegen is heavy; do small */
    /* Instead: mobility = (popcount of attacked empty squares) */
    /* skip detailed; add small term using piece counts? Keep 0 to avoid overhead */
    (void)IDX_MOB;
    /* pawn structure penalties */
    /* doubled/isolated for white and black */
    for(int side=0;side<2;side++){
        u64 pawns = side==0 ? p->bb[W_P] : p->bb[B_P];
        /* doubled: more than one per file */
        unsigned files[8]={0};
        u64 t=pawns;
        while(t){ int sq=ctz64(t); t&=t-1; files[sq%8]++; }
        int dbl=0, iso=0;
        for(int f=0;f<8;f++){
            if(files[f]>1) dbl += files[f]-1;
            int hasNeighbor = (f>0 && files[f-1]>0) || (f<7 && files[f+1]>0);
            if(files[f]>0 && !hasNeighbor) iso += files[f];
        }
        float penalty = w->w[IDX_DBL]*dbl + w->w[IDX_ISO]*iso;
        if(side==0) sc += penalty; else sc -= penalty;
    }
    /* side to move */
    int score = (int)(sc);
    if(p->side==1) score = -score;
    return score;
}