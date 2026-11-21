#include "movegen.h"
#include "attacks.h"

static void add(Move*list,int*n,int from,int to,int flags,int promo){ Move m=from|(to<<6)|(promo<<12)|flags; list[(*n)++]=m; }

int gen_pseudo_moves(const Board*b,Move*list){
  int n=0; int side=b->side;
  U64 own=side==WHITE?bb_white(b):bb_black(b);
  U64 opp=side==WHITE?bb_black(b):bb_white(b);
  U64 all=own|opp;
  int pawn=side==WHITE?W_PAWN:B_PAWN;
  int knight=side==WHITE?W_KNIGHT:B_KNIGHT;
  int king=side==WHITE?W_KING:B_KING;
  // pawns
  U64 pawns=b->bb[pawn];
  U64 tmp=pawns;
  while(tmp){
    int sq=__builtin_ctzll(tmp); tmp&=tmp-1;
    int r=sq>>3;
    if(side==WHITE){
      int to=sq+8;
      if(r<7 && !(all&(1ULL<<to))){
        if(r==6){ for(int pr=1;pr<=4;pr++) add(list,&n,sq,to,MOVE_PROMO,pr);}
        else { add(list,&n,sq,to,0,0); if(r==1){int to2=sq+16; if(!(all&(1ULL<<to2))) add(list,&n,sq,to2,0,0);} }
      }
      U64 att=pawn_attacks(WHITE,sq);
      U64 targets=att & (opp | (b->ep_square>=0?(1ULL<<b->ep_square):0ULL));
      U64 t=targets;
      while(t){ int tosq=__builtin_ctzll(t); t&=t-1;
        int ep=(b->ep_square==tosq)&&!(opp&(1ULL<<tosq));
        int flags=MOVE_CAPTURE; if(ep) flags|=MOVE_EP;
        if(r==6) for(int pr=1;pr<=4;pr++) add(list,&n,sq,tosq,MOVE_PROMO|flags,pr);
        else add(list,&n,sq,tosq,flags,0);
      }
    } else {
      int to=sq-8;
      if(r>0 && !(all&(1ULL<<to))){
        if(r==1){ for(int pr=1;pr<=4;pr++) add(list,&n,sq,to,MOVE_PROMO,pr);}
        else { add(list,&n,sq,to,0,0); if(r==6){int to2=sq-16; if(!(all&(1ULL<<to2))) add(list,&n,sq,to2,0,0);} }
      }
      U64 att=pawn_attacks(BLACK,sq);
      U64 targets=att & (opp | (b->ep_square>=0?(1ULL<<b->ep_square):0ULL));
      U64 t=targets;
      while(t){ int tosq=__builtin_ctzll(t); t&=t-1;
        int ep=(b->ep_square==tosq);
        int flags=MOVE_CAPTURE; if(ep) flags|=MOVE_EP;
        if(r==1) for(int pr=1;pr<=4;pr++) add(list,&n,sq,tosq,MOVE_PROMO|flags,pr);
        else add(list,&n,sq,tosq,flags,0);
      }
    }
  }
  // knights
  {
    U64 bb=b->bb[knight]; U64 t=bb;
    while(t){ int sq=__builtin_ctzll(t); t&=t-1;
      U64 att=knight_attacks(sq)&~own; U64 a=att;
      while(a){ int to=__builtin_ctzll(a); a&=a-1; int flags=(opp&(1ULL<<to))?MOVE_CAPTURE:0; add(list,&n,sq,to,flags,0);}
    }
  }
  // king
  {
    U64 bb=b->bb[king];
    if(bb){
      int sq=__builtin_ctzll(bb);
      U64 att=king_attacks(sq)&~own; U64 a=att;
      while(a){ int to=__builtin_ctzll(a); a&=a-1; int flags=(opp&(1ULL<<to))?MOVE_CAPTURE:0; add(list,&n,sq,to,flags,0);}
      // castling
      if(side==WHITE){
        if((b->castling&CR_WKS) && !(all&(1ULL<<5)) && !(all&(1ULL<<6))){
          if(!is_square_attacked(b,4,BLACK)&&!is_square_attacked(b,5,BLACK)&&!is_square_attacked(b,6,BLACK))
            add(list,&n,4,6,MOVE_CASTLE,0);
        }
        if((b->castling&CR_WQS) && !(all&(1ULL<<1)) && !(all&(1ULL<<2)) && !(all&(1ULL<<3))){
          if(!is_square_attacked(b,4,BLACK)&&!is_square_attacked(b,3,BLACK)&&!is_square_attacked(b,2,BLACK))
            add(list,&n,4,2,MOVE_CASTLE,0);
        }
      } else {
        if((b->castling&CR_BKS) && !(all&(1ULL<<61)) && !(all&(1ULL<<62))){
          if(!is_square_attacked(b,60,WHITE)&&!is_square_attacked(b,61,WHITE)&&!is_square_attacked(b,62,WHITE))
            add(list,&n,60,62,MOVE_CASTLE,0);
        }
        if((b->castling&CR_BQS) && !(all&(1ULL<<57)) && !(all&(1ULL<<58)) && !(all&(1ULL<<59))){
          if(!is_square_attacked(b,60,WHITE)&&!is_square_attacked(b,59,WHITE)&&!is_square_attacked(b,58,WHITE))
            add(list,&n,60,58,MOVE_CASTLE,0);
        }
      }
    }
  }
  // sliding
  int bishop_code=side==WHITE?W_BISHOP:B_BISHOP;
  int rook_code=side==WHITE?W_ROOK:B_ROOK;
  int queen_code=side==WHITE?W_QUEEN:B_QUEEN;
  for(int pc=0;pc<3;pc++){
    int piece_code= pc==0?bishop_code:(pc==1?rook_code:queen_code);
    int ptype= pc==0?BISHOP:(pc==1?ROOK:QUEEN);
    U64 bb=b->bb[piece_code]; U64 t=bb;
    while(t){ int sq=__builtin_ctzll(t); t&=t-1;
      U64 att=sliding_attacks(sq,all,ptype)&~own; U64 a=att;
      while(a){ int to=__builtin_ctzll(a); a&=a-1; int flags=(opp&(1ULL<<to))?MOVE_CAPTURE:0; add(list,&n,sq,to,flags,0);}
    }
  }
  return n;
}

int gen_legal_moves(Board *b, Move *list){
  Move pseudo[MAX_MOVES];
  int np=gen_pseudo_moves(b,pseudo);
  int n=0;
  for(int i=0;i<np;i++){
    Board b2=*b;
    if(!board_make_move(&b2,pseudo[i])) continue;
    int inCheck=board_in_check(&b2, b->side);
    board_unmake(&b2);
    if(!inCheck) list[n++]=pseudo[i];
  }
  return n;
}

int move_is_capture(const Board *b, Move m){
  int to=move_to(m);
  if(move_flags(m)&MOVE_CAPTURE) return 1;
  if(move_flags(m)&MOVE_EP) return 1;
  if(piece_on(b,to)!=NO_PIECE) return 1;
  return 0;
}

int move_to_uci(Move m, char *buf){
  int from=move_from(m), to=move_to(m);
  buf[0]='a'+(from&7); buf[1]='1'+(from>>3);
  buf[2]='a'+(to&7); buf[3]='1'+(to>>3);
  int pr=move_promo(m);
  if((move_flags(m)&MOVE_PROMO) && pr>=1 && pr<=4){
    const char *p="nbrq"; buf[4]=p[pr-1]; buf[5]=0;
  } else buf[4]=0;
  return 0;
}
