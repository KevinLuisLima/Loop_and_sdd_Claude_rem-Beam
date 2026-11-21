#define _GNU_SOURCE
#include "board.h"
#include "attacks.h"
#include "movegen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char* PIECE_CHARS = "pnbrqkPNBRQK";

U64 bb_white(const Board *b){ U64 r=0; for(int p=0;p<6;p++) r|=b->bb[p]; return r;}
U64 bb_black(const Board *b){ U64 r=0; for(int p=6;p<12;p++) r|=b->bb[p]; return r;}
U64 bb_all(const Board *b){ return bb_white(b)|bb_black(b);}
U64 bb_pieces(const Board *b,int piece){ return b->bb[piece]; }
int piece_on(const Board *b,int sq){
  U64 m=1ULL<<sq; for(int p=0;p<12;p++) if(b->bb[p]&m) return p; return NO_PIECE;
}

void board_init(Board *b){ board_from_fen(b,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

static int char_to_piece(char c){
  const char*s="PNBRQKpnbrqk";
  for(int i=0;i<12;i++) if(s[i]==c) return i;
  return -1;
}

int board_from_fen(Board *b,const char *fen){
  memset(b,0,sizeof(Board));
  b->ep_square=-1; b->side=WHITE; b->castling=0; b->halfmove=0; b->fullmove=1; b->hist_top=0;
  char buf[512]; strncpy(buf,fen,511); buf[511]=0;
  char *save=0; char *tok=strtok_r(buf," ",&save);
  int rank=7,file=0;
  if(!tok) return 1;
  for(char*p=tok;*p;p++){
    if(*p=='/'){rank--;file=0;}
    else if(*p>='1'&&*p<='8'){file += *p-'0';}
    else {
      int pc=char_to_piece(*p); if(pc<0) return 1;
      int sq=rank*8+file; b->bb[pc]|=1ULL<<sq; file++;
    }
  }
  tok=strtok_r(NULL," ",&save); if(!tok) return 1;
  b->side=(*tok=='b')?BLACK:WHITE;
  tok=strtok_r(NULL," ",&save); if(!tok) return 1;
  if(strcmp(tok,"-")!=0){
    for(char*p=tok;*p;p++){
      switch(*p){case 'K':b->castling|=CR_WKS;break;case 'Q':b->castling|=CR_WQS;break;case 'k':b->castling|=CR_BKS;break;case 'q':b->castling|=CR_BQS;break;}
    }
  }
  tok=strtok_r(NULL," ",&save); if(!tok) return 1;
  if(strcmp(tok,"-")!=0){
    int f=tok[0]-'a', r=tok[1]-'1'; b->ep_square=r*8+f;
  } else b->ep_square=-1;
  tok=strtok_r(NULL," ",&save); if(tok) b->halfmove=atoi(tok);
  tok=strtok_r(NULL," ",&save); if(tok) b->fullmove=atoi(tok);
  // validate basic
  return 0;
}

int board_to_fen(const Board *b, char *buf, size_t sz){
  (void)sz;
  int idx=0;
  for(int r=7;r>=0;r--){
    int empties=0;
    for(int f=0;f<8;f++){
      int sq=r*8+f;
      int p=piece_on(b,sq);
      if(p==NO_PIECE) empties++;
      else {
        if(empties){ buf[idx++]='0'+empties; empties=0;}
        buf[idx++]=PIECE_CHARS[p];
      }
    }
    if(empties) buf[idx++]='0'+empties;
    if(r>0) buf[idx++]='/';
  }
  buf[idx++]=' ';
  buf[idx++]=b->side==WHITE?'w':'b';
  buf[idx++]=' ';
  if(b->castling==0) buf[idx++]='-';
  else { if(b->castling&CR_WKS)buf[idx++]='K'; if(b->castling&CR_WQS)buf[idx++]='Q'; if(b->castling&CR_BKS)buf[idx++]='k'; if(b->castling&CR_BQS)buf[idx++]='q';}
  buf[idx++]=' ';
  if(b->ep_square<0) buf[idx++]='-';
  else {buf[idx++]='a'+(b->ep_square&7); buf[idx++]='1'+(b->ep_square>>3);}
  buf[idx++]=' ';
  idx+=sprintf(buf+idx,"%d %d",b->halfmove,b->fullmove);
  buf[idx]=0;
  return 0;
}

int board_in_check(const Board *b,int side){
  int king_sq=-1;
  U64 kbb=bb_pieces(b, side==WHITE?W_KING:B_KING);
  if(kbb) king_sq=__builtin_ctzll(kbb);
  if(king_sq<0) return 0;
  return is_square_attacked(b,king_sq,side==WHITE?BLACK:WHITE);
}

int board_side_to_move(const Board *b){ return b->side;}

static void update_castling_on_move(Board*b,Move m){
  int from=move_from(m),to=move_to(m);
  int piece=-1;
  U64 mask=1ULL<<from;
  for(int pc=0;pc<12;pc++) if(b->bb[pc]&mask){piece=pc;break;}
  if(piece==W_KING){b->castling &= ~(CR_WKS|CR_WQS);}
  if(piece==B_KING){b->castling &= ~(CR_BKS|CR_BQS);}
  if(piece==W_ROOK){
    if(from==0) b->castling&=~CR_WQS;
    if(from==7) b->castling&=~CR_WKS;
  }
  if(piece==B_ROOK){
    if(from==56) b->castling&=~CR_BQS;
    if(from==63) b->castling&=~CR_BKS;
  }
  int cap_piece=-1;
  U64 tmask=1ULL<<to;
  for(int pc=0;pc<12;pc++) if(b->bb[pc]&tmask){cap_piece=pc;break;}
  if(cap_piece==W_ROOK){if(to==0)b->castling&=~CR_WQS;if(to==7)b->castling&=~CR_WKS;}
  if(cap_piece==B_ROOK){if(to==56)b->castling&=~CR_BQS;if(to==63)b->castling&=~CR_BKS;}
}

int board_make_move(Board *b, Move m){
  Undo *u=&b->history[b->hist_top++];
  if(b->hist_top>1023) { b->hist_top--; return 0; }
  u->move=m; u->side=b->side; u->castling=b->castling; u->ep_square=b->ep_square; u->halfmove=b->halfmove; u->fullmove=b->fullmove; u->captured=NO_PIECE; u->captured_sq=-1;
  int from=move_from(m), to=move_to(m);
  int flags=move_flags(m);
  int piece=piece_on(b,from);
  if(piece==NO_PIECE){ b->hist_top--; return 0; }
  // capture
  if(flags & MOVE_EP){
    int cap_sq = (b->side==WHITE)? (to-8):(to+8);
    int cap_piece=piece_on(b,cap_sq);
    if(cap_piece!=NO_PIECE){ b->bb[cap_piece]&=~(1ULL<<cap_sq); u->captured=cap_piece; u->captured_sq=cap_sq; }
  } else {
    int cap_piece=piece_on(b,to);
    if(cap_piece!=NO_PIECE){ b->bb[cap_piece]&=~(1ULL<<to); u->captured=cap_piece; u->captured_sq=to; }
  }
  (void)0;
  update_castling_on_move(b,m);
  // move piece
  b->bb[piece]&=~(1ULL<<from);
  int promo=move_promo(m);
  if(flags & MOVE_PROMO){
    int types[5]={0,PAWN?0:0,0,0,0}; (void)types;
    int pt = (promo>=1&&promo<=4)? (promo==1?KNIGHT:(promo==2?BISHOP:(promo==3?ROOK:QUEEN))) : QUEEN;
    int code = (b->side==WHITE)? pt : pt+6;
    b->bb[code] |= 1ULL<<to;
  } else {
    b->bb[piece]|=1ULL<<to;
  }
  if(flags & MOVE_CASTLE){
    if(b->side==WHITE){
      if(to==6){ b->bb[W_ROOK]&=~(1ULL<<7); b->bb[W_ROOK]|=1ULL<<5; }
      else if(to==2){ b->bb[W_ROOK]&=~(1ULL<<0); b->bb[W_ROOK]|=1ULL<<3; }
    } else {
      if(to==62){ b->bb[B_ROOK]&=~(1ULL<<63); b->bb[B_ROOK]|=1ULL<<61; }
      else if(to==58){ b->bb[B_ROOK]&=~(1ULL<<56); b->bb[B_ROOK]|=1ULL<<59; }
    }
  }
  // ep_square
  b->ep_square=-1;
  int moved_type=PIECE_TYPE(piece);
  if(moved_type==PAWN){
    if((from>>3==1 && to>>3==3) || (from>>3==6 && to>>3==4)){
      b->ep_square=(from+to)/2;
    }
  }
  // halfmove
  if(moved_type==PAWN || (flags&(MOVE_CAPTURE|MOVE_EP))) b->halfmove=0;
  else b->halfmove++;
  if(b->side==BLACK) b->fullmove++;
  b->side ^=1;
  return 1;
}

void board_unmake(Board *b){
  if(b->hist_top<=0) return;
  Undo *u=&b->history[--b->hist_top];
  Move m=u->move;
  int from=move_from(m),to=move_to(m);
  int flags=move_flags(m);
  int moving_side=u->side;
  b->side=moving_side;
  // fix promoted piece
  if(flags & MOVE_PROMO){
    int pr=move_promo(m);
    int pt=(pr>=1&&pr<=4)? (pr==1?KNIGHT:(pr==2?BISHOP:(pr==3?ROOK:QUEEN))) : QUEEN;
    int code=moving_side==WHITE? pt : pt+6;
    b->bb[code] &= ~(1ULL<<to);
    int pawn_code=moving_side==WHITE? W_PAWN : B_PAWN;
    b->bb[pawn_code] |= 1ULL<<from;
  } else {
    // find piece at to
    for(int pc=0;pc<12;pc++){
      if(b->bb[pc]&(1ULL<<to)){ b->bb[pc]&=~(1ULL<<to); b->bb[pc]|=1ULL<<from; break; }
    }
  }
  if(u->captured!=NO_PIECE){
    if(flags & MOVE_EP){
      int cap_sq=u->captured_sq; if(cap_sq<0) cap_sq=(moving_side==WHITE)?(to-8):(to+8);
      b->bb[u->captured] |= 1ULL<<cap_sq;
    } else {
      b->bb[u->captured] |= 1ULL<<to;
    }
  }
  if(flags & MOVE_CASTLE){
    if(moving_side==WHITE){
      if(to==6){ b->bb[W_ROOK]&=~(1ULL<<5); b->bb[W_ROOK]|=1ULL<<7; }
      else if(to==2){ b->bb[W_ROOK]&=~(1ULL<<3); b->bb[W_ROOK]|=1ULL<<0; }
    } else {
      if(to==62){ b->bb[B_ROOK]&=~(1ULL<<61); b->bb[B_ROOK]|=1ULL<<63; }
      else if(to==58){ b->bb[B_ROOK]&=~(1ULL<<59); b->bb[B_ROOK]|=1ULL<<56; }
    }
  }
  b->castling=u->castling;
  b->ep_square=u->ep_square;
  b->halfmove=u->halfmove;
  b->fullmove=u->fullmove;
}

int board_legal_move(const Board *b, Move m){
  Board b2=*b;
  if(!board_make_move(&b2,m)) return 0;
  int ic=board_in_check(&b2, b->side);
  board_unmake(&b2);
  return !ic;
}

int board_game_over(const Board *b){
  Board b2=*b;
  Move ml[MAX_MOVES];
  int n=gen_legal_moves(&b2,ml);
  if(n==0) return 1;
  if(b->halfmove>=100) return 1;
  if(board_repetition(b)) return 1;
  if(board_insufficient_material(b)) return 1;
  return 0;
}

int board_repetition(const Board *b){ (void)b; return 0; }
int board_insufficient_material(const Board *b){
  int pieces=0;
  for(int pc=0;pc<12;pc++){
    if(pc==W_KING || pc==B_KING) continue;
    pieces += __builtin_popcountll(b->bb[pc]);
  }
  if(pieces==0) return 1;
  if(pieces==1){
    if(b->bb[W_BISHOP]|b->bb[B_BISHOP]|b->bb[W_KNIGHT]|b->bb[B_KNIGHT]) return 1;
  }
  return 0;
}

unsigned long long perft(Board b, int depth){
  if(depth==0) return 1ULL;
  Move ml[MAX_MOVES];
  Board b2=b;
  int n=gen_legal_moves(&b2,ml);
  unsigned long long nodes=0;
  for(int i=0;i<n;i++){
    Board cb=b;
    if(!board_make_move(&cb,ml[i])) continue;
    nodes += perft(cb, depth-1);
    board_unmake(&cb);
  }
  return nodes;
}
