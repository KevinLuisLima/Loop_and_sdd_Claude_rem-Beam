#include "attacks.h"
#include <assert.h>

static U64 knight_att[64];
static U64 king_att[64];
static U64 pawn_att[2][64];

static int file_of(int sq){return sq&7;}
static int rank_of(int sq){return sq>>3;}

void attacks_init(void){
  const int ko[8]={-17,-15,-10,-6,6,10,15,17};
  const int kingo[8]={-9,-8,-7,-1,1,7,8,9};
  for(int sq=0;sq<64;sq++){
    U64 k=0;
    for(int i=0;i<8;i++){
      int t=sq+ko[i];
      if(t<0||t>63) continue;
      int df=file_of(t)-file_of(sq); if(df<0)df=-df;
      int dr=rank_of(t)-rank_of(sq); if(dr<0)dr=-dr;
      if((df==1&&dr==2)||(df==2&&dr==1)) k|=1ULL<<t;
    }
    knight_att[sq]=k;
    U64 kg=0;
    for(int i=0;i<8;i++){
      int t=sq+kingo[i];
      if(t<0||t>63) continue;
      int df=file_of(t)-file_of(sq); if(df<0)df=-df;
      int dr=rank_of(t)-rank_of(sq); if(dr<0)dr=-dr;
      if(df<=1&&dr<=1&&(df||dr)) kg|=1ULL<<t;
    }
    king_att[sq]=kg;
    U64 w=0,b=0;
    int f=file_of(sq),r=rank_of(sq);
    if(r<7){
      if(f>0) w|=1ULL<<(sq+7);
      if(f<7) w|=1ULL<<(sq+9);
    }
    if(r>0){
      if(f>0) b|=1ULL<<(sq-9);
      if(f<7) b|=1ULL<<(sq-7);
    }
    pawn_att[WHITE][sq]=w;
    pawn_att[BLACK][sq]=b;
  }
}

U64 pawn_attacks(int side,int sq){return pawn_att[side][sq];}
U64 knight_attacks(int sq){return knight_att[sq];}
U64 king_attacks(int sq){return king_att[sq];}

U64 sliding_attacks(int sq, U64 blockers, int piece_type){
  U64 res=0; int cur;
  res=0;
  if(piece_type!=BISHOP){
    int cur=sq;
    while(rank_of(cur)<7){ cur+=8; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(rank_of(cur)>0){ cur-=8; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(file_of(cur)<7){ cur+=1; if(rank_of(cur)!=rank_of(sq)) break; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(file_of(cur)>0){ cur-=1; if(rank_of(cur)!=rank_of(sq)) break; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
  }
  if(piece_type!=ROOK){
    // NE
    cur=sq;
    while(rank_of(cur)<7&&file_of(cur)<7){ cur+=9; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(rank_of(cur)<7&&file_of(cur)>0){ cur+=7; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(rank_of(cur)>0&&file_of(cur)<7){ cur-=7; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
    cur=sq;
    while(rank_of(cur)>0&&file_of(cur)>0){ cur-=9; U64 bb=1ULL<<cur; res|=bb; if(blockers&bb) break; }
  }
  return res;
}

int is_square_attacked(const Board *b, int sq, int by_side){
  U64 occ=bb_all(b);
  U64 pawns=bb_pieces(b, by_side==WHITE?W_PAWN:B_PAWN);
  U64 tmp=pawns;
  while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(pawn_attacks(by_side,ps)&(1ULL<<sq)) return 1; }
  U64 kn=bb_pieces(b, by_side==WHITE?W_KNIGHT:B_KNIGHT);
  tmp=kn; while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(knight_attacks(ps)&(1ULL<<sq)) return 1; }
  U64 kg=bb_pieces(b, by_side==WHITE?W_KING:B_KING);
  tmp=kg; while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(king_attacks(ps)&(1ULL<<sq)) return 1; }
  U64 bq=bb_pieces(b, by_side==WHITE?W_BISHOP:B_BISHOP)|bb_pieces(b, by_side==WHITE?W_QUEEN:B_QUEEN);
  tmp=bq; while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(sliding_attacks(ps,occ,BISHOP)&(1ULL<<sq)) return 1; }
  U64 rq=bb_pieces(b, by_side==WHITE?W_ROOK:B_ROOK)|bb_pieces(b, by_side==WHITE?W_QUEEN:B_QUEEN);
  tmp=rq; while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(sliding_attacks(ps,occ,ROOK)&(1ULL<<sq)) return 1; }
  return 0;
}
