#include "eval.h"
#include <math.h>
#include <string.h>

static int pst[6][64];

static void init_pst(void){
    static int done=0;
    if(done) return;
    done=1;
    /* pawns */
    int pawn[64]={
     0, 0, 0, 0, 0, 0, 0, 0,
     5, 5, 5, 5, 5, 5, 5, 5,
     1, 1, 2, 3, 3, 2, 1, 1,
     0, 0, 0, 2, 2, 0, 0, 0,
     0, 0, 0,-2,-2, 0, 0, 0,
    -2,-3,-3,-4,-4,-3,-3,-2,
     0, 0, 0, 0, 0, 0, 0, 0,
     0,0,0,0,0,0,0,0};
    for(int i=0;i<64;i++) pst[0][i]=pawn[i];
    /* other simple 0 */
    for(int p=1;p<6;p++) for(int i=0;i<64;i++) pst[p][i]=0;
}

static int material(const Position *p){
    static const int vals[6]={100,300,300,500,900,0};
    int s=0;
    for(int pc=0;pc<12;pc++){
        int v=vals[pc%6]; if(pc>=6) v=-v;
        /* count pieces */
        uint64_t b=p->bb[pc];
        /* popcount */
        int cnt=__builtin_popcountll(b);
        s += (pc<6?1:-1)*cnt*vals[pc%6];
    }
    if(p->side==BLACK) s=-s;
    return s;
}

static int pst_score(const Position *p){
    init_pst();
    int s=0;
    for(int sq=0;sq<64;sq++){
        int pc=p->board[sq];
        if(pc==12) continue;
        int r=pc<6?1:-1;
        int t=pc%6;
        int v=pst[t][pc<6?sq:(63-sq)];
        s += r*v;
    }
    if(p->side==BLACK) s=-s;
    return s;
}

void eval_features(const Position *p, float *f){
    init_pst();
    for(int i=0;i<NUM_WEIGHTS;i++) f[i]=0;
    f[0]=material(p);
    f[1]=pst_score(p);
    /* mobility approx: legal move count */
    /* expensive but okay */
    /* pawn structure doubled/isolated */
    /* compute simple: count doubled pawns */
    int doubled=0;
    /* for each file count pawns */
    int wfiles[8]={0}, bfiles[8]={0};
    for(int s=0;s<64;s++){
        if(p->board[s]==WP) wfiles[s%8]++;
        if(p->board[s]==BP) bfiles[s%8]++;
    }
    for(int f=0;f<8;f++){ if(wfiles[f]>1) doubled+= wfiles[f]-1; if(bfiles[f]>1) doubled-= bfiles[f]-1; }
    if(p->side==BLACK) doubled=-doubled;
    f[3]= -doubled*20;
    /* king safety: attacked squares around king (opp) */
    int kingSq=-1;
    for(int s=0;s<64;s++) if(p->board[s]==(p->side==WHITE?WK:BK)){kingSq=s;break;}
    int unsafe=0;
    if(kingSq>=0){
        for(int dr=-1;dr<=1;dr++) for(int df=-1;df<=1;df++){
            int r=kingSq/8+dr, f=kingSq%8+df;
            if(r<0||r>7||f<0||f>7) continue;
            // attacked?
            unsafe += 0;
        }
    }
    f[4]= -unsafe;
    /* center control */
    int center=0;
    for(int sq=0;sq<64;sq++){
        int pc=p->board[sq];
        if(pc==12) continue;
        if(pc==WP||pc==BP){
            int mid=(sq>=27 && sq<=36);
            if(mid) center += (pc<6?5:-5);
        }
    }
    if(p->side==BLACK) center=-center;
    f[5]=center;
    /* mobility approximate: count moves cheap? skip; set 0 */
    f[2]=0;
}

int eval_position(const Position *p, const float *weights){
    if(p->halfmove>=100) return 0; /* 50-move */
    float f[NUM_WEIGHTS];
    eval_features(p,f);
    float s=0;
    for(int i=0;i<NUM_WEIGHTS;i++){
        float w = weights? weights[i]:1.0f;
        s += f[i]*w;
    }
    return (int)s;
}
