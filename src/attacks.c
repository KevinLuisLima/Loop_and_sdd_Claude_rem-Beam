#include "attacks.h"
#include <assert.h>

static const int KNIGHT_OFFSETS[8] = { -17,-15,-10,-6,6,10,15,17 };
static const int KING_OFFSETS[8] = { -9,-8,-7,-1,1,7,8,9 };

/* For sliding offsets by direction */

static U64 knight_att[64];
static U64 king_att[64];
static U64 pawn_att[2][64]; /* [side][sq] */

static int on_board(int sq){ return sq>=0 && sq<64; }
/* avoid wrap: check files */

void attacks_init(void){
  for(int sq=0; sq<64; sq++){
    U64 k=0;
    for(int d=0; d<8; d++){
      int t=sq+KNIGHT_OFFSETS[d];
      if(on_board(t)){
        /* file wrap check for knight: abs file diff 1/2 */
        int df = (t&7)-(sq&7); if(df<0) df=-df;
        int dr = (t>>3)-(sq>>3); if(dr<0) dr=-dr;
        if((df==1&&dr==2)||(df==2&&dr==1)) k |= 1ULL<<t;
      }
    }
    knight_att[sq]=k;
    U64 kg=0;
    for(int d=0; d<8; d++){
      int t=sq+KING_OFFSETS[d];
      if(on_board(t)){
        int df=(t&7)-(sq&7); if(df<0)df=-df;
        int dr=(t>>3)-(sq>>3); if(dr<0)dr=-dr;
        if(df<=1 && dr<=1 && !(df==0&&dr==0)) kg |= 1ULL<<t;
      }
    }
    king_att[sq]=kg;
    /* pawn attacks */
    /* white pawns attack up-left/up-right */
    U64 w=0,b=0;
    int f=sq&7, r=sq>>3;
    if(r<7){
      if(f>0) w |= 1ULL<<(sq+7);
      if(f<7) w |= 1ULL<<(sq+9);
    }
    /* black pawns attack down-left/down-right (from black's perspective: they move down, attack down) */
    if(r>0){
      if(f>0) b |= 1ULL<<(sq-9);
      if(f<7) b |= 1ULL<<(sq-7);
    }
    pawn_att[WHITE][sq]=w;
    pawn_att[BLACK][sq]=b;
  }
}

U64 pawn_attacks(int side,int sq){
  return pawn_att[side][sq];
}
U64 knight_attacks(int sq){ return knight_att[sq]; }
U64 king_attacks(int sq){ return king_att[sq]; }

U64 sliding_attacks(int sq, U64 blockers, int piece_type){
  U64 res=0;
  /* directions grouped */
  int dirs_ok = 0;
  /* We handle by 8 possible ray directions with boundary checks */
  /* N */
  if(piece_type!=BISHOP){
    int cur=sq;
    while((cur>>3)<7){ cur+=8; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* S */
  if(piece_type!=BISHOP){
    int cur=sq;
    while((cur>>3)>0){ cur-=8; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* E */
  if(piece_type!=BISHOP){
    int cur=sq;
    while((cur&7)<7){ cur+=1; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* W */
  if(piece_type!=BISHOP){
    int cur=sq;
    while((cur&7)>0){ cur-=1; if((cur>>3)!=(sq>>3)) break; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* NE */
  if(piece_type!=ROOK){
    int cur=sq;
    while((cur>>3)<7 && (cur&7)<7){ cur+=9; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* NW */
  if(piece_type!=ROOK){
    int cur=sq;
    while((cur>>3)<7 && (cur&7)>0){ cur+=7; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* SE */
  if(piece_type!=ROOK){
    int cur=sq;
    while((cur>>3)>0 && (cur&7)<7){ cur-=7; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  /* SW */
  if(piece_type!=ROOK){
    int cur=sq;
    while((cur>>3)>0 && (cur&7)>0){ cur-=9; U64 b=1ULL<<cur; res|=b; if(blockers&b) break; }
  }
  (void)dirs_ok;
  return res;
}

int is_square_attacked(const Board *b, int sq, int by_side){
  /* check pawn attacks reaching sq */
  /* A square is attacked by opponent pawns if sq is in their pawn attack set */
  /* We'll check by finding pawns that could attack sq */
  U64 occ = bb_all(b);
  /* pawn */
  /* For by_side pawns, sq attacked if there is pawn at sq attacked-from positions */
  U64 pawns = bb_pieces(b, by_side==WHITE?W_PAWN:B_PAWN);
  /* pawn attack reverse: positions from which a pawn attacks sq */
  /* white pawn attacks from sq-7 and sq-9? Let's just check: for each pawn square, its attack set contains sq */
  /* brute small: iterate pawns */
  /* optimize not needed */
  U64 tmp=pawns;
  while(tmp){
    int ps=__builtin_ctzll(tmp); tmp&=tmp-1;
    if(pawn_attacks(by_side,ps) & (1ULL<<sq)) return 1;
  }
  /* knight */
  U64 kn = bb_pieces(b, by_side==WHITE?W_KNIGHT:B_KNIGHT);
  tmp=kn;
  while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(knight_attacks(ps)&(1ULL<<sq)) return 1; }
  /* king */
  U64 kg = bb_pieces(b, by_side==WHITE?W_KING:B_KING);
  tmp=kg;
  while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(king_attacks(ps)&(1ULL<<sq)) return 1; }
  /* bishops/queens */
  U64 bq = bb_pieces(b, by_side==WHITE?W_BISHOP:B_BISHOP) | bb_pieces(b, by_side==WHITE?W_QUEEN:B_QUEEN);
  tmp=bq;
  while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(sliding_attacks(ps, occ, BISHOP)&(1ULL<<sq)) return 1; }
  /* rooks/queens */
  U64 rq = bb_pieces(b, by_side==WHITE?W_ROOK:B_ROOK) | bb_pieces(b, by_side==WHITE?W_QUEEN:B_QUEEN);
  tmp=rq;
  while(tmp){ int ps=__builtin_ctzll(tmp); tmp&=tmp-1; if(sliding_attacks(ps, occ, ROOK)&(1ULL<<sq)) return 1; }
  return 0;
}