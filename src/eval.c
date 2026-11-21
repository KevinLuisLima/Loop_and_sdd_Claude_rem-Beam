#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NW 400
static float default_w[NW];

static void init_defaults(void){
    static int done=0; if(done) return;
    for(int i=0;i<NW;i++) default_w[i]=0.0f;
    default_w[0]=100; default_w[1]=320; default_w[2]=330; default_w[3]=500; default_w[4]=900;
    // PST pawns: index 6+piece*64+sq; center bonus
    for(int pt=0;pt<6;pt++) for(int sq=0;sq<64;sq++){
        int r=sq/8,f=sq%8; float v=0;
        if(pt==0){ v = (r==3||r==4)? 10 : 0; if(r==6) v=5; if(sq%8==0||sq%8==7) v-=5;}
        if(pt==1){ if(r==3||r==4) v=10; }
        if(pt==2){ if((r==3||r==4)&&(f>1&&f<6)) v=10;}
        if(pt==5){ /* king safety corners */ if(r==0||r==7) v=5;}
        default_w[6+pt*64+sq]=v;
    }
    default_w[6+5*64+3] = -5; // arbitrary
    default_w[390]=5; // mobility
    default_w[391]=-10; default_w[392]=-15;
    done=1;
}

int eval_load(Weights*w,const char*path){
    w->n=0; w->w=NULL;
    FILE*f=fopen(path,"rb");
    if(!f) return -1;
    uint32_t ver,cnt,type;
    if(fread(&ver,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&cnt,4,1,f)!=1){fclose(f);return -1;}
    if(fread(&type,4,1,f)!=1){fclose(f);return -1;}
    if(type!=0){fclose(f);return -1;}
    if(cnt>NW+1000){ /* allow */ }
    // little-endian assumed host x86
    w->w=malloc(cnt*sizeof(float));
    if(!w->w){fclose(f);return -1;}
    if(fread(w->w,sizeof(float),cnt,f)!=cnt){free(w->w);w->w=NULL;fclose(f);return -1;}
    // NaN check
    for(uint32_t i=0;i<cnt;i++){ if(!isfinite(w->w[i])){free(w->w);w->w=NULL;fclose(f);return -1;}}
    w->n=cnt;
    fclose(f);
    return 0;
}
void eval_free(Weights*w){ if(w->w) free(w->w); w->w=NULL; w->n=0;}

static const float* W(const Weights*w){ if(w && w->w && w->n>=NW) return w->w; init_defaults(); return default_w;}

int evaluate(const Board*b,const Weights*w){
    init_defaults();
    const float*wt = W(w);
    int score=0; // white relative, centipawns integer
    // material + PST
    for(int sq=0;sq<64;sq++){
        int p=piece_at(b,sq);
        if(p<0) continue;
        int pt=p%6;
        int val=0;
        if(pt>=0 && pt<=4) val=(int)wt[pt];
        // mirror for black?
        int sqw = (p<6)? sq : (sq^56);
        int pstv = (int)wt[6+pt*64+sqw];
        if(p<6){ score += val + pstv; } else { score -= val + pstv; }
    }
    // mobility estimate by bitboard counts for side to move and opponent
    // simple: count piece attacks (not legal)
    int mob[2]={0,0};
    for(int side=0;side<2;side++){
        for(int sq=0;sq<64;sq++){
            int p=piece_at(b,sq);
            if(p<0 || (p<6?0:1)!=side) continue;
            int pt=p%6;
            (void)pt; (void)sq;
            mob[side]+= (pt==0?1:0);
        }
    }
    int mmob = mob[0]-mob[1];
    if(b->side==WHITE) score += (int)(wt[390]* (mmob/5));
    else score -= (int)(wt[390]* (mmob/5));
    // pawn structure (doubled/isolated) for both
    // penalties relative to owner
    for(int side=0;side<2;side++){
        uint64_t pawns = b->bb[side==WHITE?WP:BP];
        // doubled: two pawns same file
        int doubled=0;
        for(int f=0;f<8;f++){
            int cnt=0; for(int r=0;r<8;r++) if(pawns&(1ULL<<(r*8+f))) cnt++;
            if(cnt>1) doubled += cnt-1;
        }
        int pen = (int)wt[391]*doubled;
        if(side==WHITE) score += pen; else score -= pen;
    }
    // relative to side to move
    if(b->side==BLACK) score=-score;
    return score;
}
