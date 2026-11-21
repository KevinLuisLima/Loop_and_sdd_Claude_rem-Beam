#include "eval.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

int weights_load(const char *path, Weights *out, char *errbuf, size_t errsz){
  FILE *f=fopen(path,"rb");
  if(!f){ snprintf(errbuf,errsz,"cannot open weights file"); return 1; }
  char magic[7]={0};
  if(fread(magic,1,6,f)!=6){ fclose(f); snprintf(errbuf,errsz,"bad header"); return 1; }
  if(memcmp(magic,"ECHW01",6)!=0){ fclose(f); snprintf(errbuf,errsz,"bad magic"); return 1; }
  unsigned ver,nw,type;
  if(fread(&ver,4,1,f)!=1 || fread(&nw,4,1,f)!=1 || fread(&type,4,1,f)!=1){ fclose(f); snprintf(errbuf,errsz,"bad header"); return 1; }
  if(ver!=1){ fclose(f); snprintf(errbuf,errsz,"bad version"); return 1; }
  if(nw!=NUM_WEIGHTS){ fclose(f); snprintf(errbuf,errsz,"bad num weights"); return 1; }
  if(type!=0){ fclose(f); snprintf(errbuf,errsz,"bad type"); return 1; }
  if(fread(out->w,sizeof(float),NUM_WEIGHTS,f)!=(size_t)NUM_WEIGHTS){ fclose(f); snprintf(errbuf,errsz,"truncated weights"); return 1; }
  for(int i=0;i<NUM_WEIGHTS;i++){
    if(!isfinite(out->w[i])){ fclose(f); snprintf(errbuf,errsz,"nan/inf in weights"); return 1; }
  }
  fclose(f);
  return 0;
}

int eval_score(const Board *b, const Weights *w){
  if(!w) return 0;
  int white_score=0;
  for(int sq=0;sq<64;sq++){
    int p=piece_on(b,sq);
    if(p==NO_PIECE) continue;
    int type=PIECE_TYPE(p);
    if(type==KING) continue;
    int side=PIECE_COLOR(p);
    float mv=w->w[type];
    int psi=5 + type*64 + (side==WHITE?sq:(63-sq));
    float pv=(psi<NUM_WEIGHTS)? w->w[psi]:0.0f;
    int val=(int)(mv+pv);
    if(side==WHITE) white_score+=val; else white_score-=val;
  }
  int score=white_score;
  if(b->side==BLACK) score=-score;
  return score;
}

int eval_random_weights(Weights *w, unsigned seed){
  srand(seed);
  for(int i=0;i<NUM_WEIGHTS;i++){
    if(i<5) w->w[i]=(float)(100*(i+1));
    else w->w[i]=0.0f;
  }
  return 0;
}
