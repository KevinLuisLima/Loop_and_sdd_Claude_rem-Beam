#include "board.h"
#include "attacks.h"
#include "types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void pos_init(Position *pos){
    pos_from_fen(pos,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static int ptype_from_char(char c){
    switch(c){case 'P':case 'p':return PAWN;case 'N':case 'n':return KNIGHT;case 'B':case 'b':return BISHOP;case 'R':case 'r':return ROOK;case 'Q':case 'q':return QUEEN;case 'K':case 'k':return KING;}
    return -1;
}
static int color_from_char(char c){ return isupper(c)?WHITE:BLACK; }
static char char_from_piece(int side,int pt){ return side==WHITE?"PNBRQK"[pt]:"pnbrqk"[pt]; }

int pos_from_fen(Position *pos, const char *fen){
    memset(pos,0,sizeof(Position));
    pos->ep_square=-1; pos->ply=0;
    char buf[512]; strncpy(buf,fen,511); buf[511]=0;
    char *tok=strtok(buf," ");
    if(!tok) return -1;
    int sq=56;
    for(char *p=tok;*p;p++){
        if(*p=='/'){ sq-=16; continue; }
        if(isdigit(*p)){ sq += *p-'0'; continue; }
        int pt=ptype_from_char(*p);
        if(pt<0) return -1;
        int c=color_from_char(*p);
        pos->bb[c][pt] |= 1ULL << sq;
        sq++;
    }
    tok=strtok(NULL," "); if(!tok) return -1;
    pos->side = (tok[0]=='w')?WHITE:BLACK;
    tok=strtok(NULL," "); if(!tok) return -1;
    pos->castling=0;
    for(char *p=tok;*p;p++){
        if(*p=='K') pos->castling|=1;
        if(*p=='Q') pos->castling|=2;
        if(*p=='k') pos->castling|=4;
        if(*p=='q') pos->castling|=8;
    }
    tok=strtok(NULL," ");
    if(tok && tok[0]!='-'){
        int file=tok[0]-'a'; int rank=tok[1]-'1';
        pos->ep_square=rank*8+file;
    } else pos->ep_square=-1;
    tok=strtok(NULL," ");
    if(tok) pos->halfmove=atoi(tok); else pos->halfmove=0;
    tok=strtok(NULL," ");
    if(tok) pos->fullmove=atoi(tok); else pos->fullmove=1;
    return 0;
}

void pos_to_fen(Position *pos,char *buf,size_t sz){
    size_t i=0;
    for(int r=7;r>=0;r--){
        int empt=0;
        for(int f=0;f<8;f++){
            int s=r*8+f;
            int found=-1,side=-1;
            for(int c=0;c<2;c++) for(int p=0;p<6;p++) if(pos->bb[c][p]&(1ULL<<s)){found=p;side=c;break;}
            if(found>=0){ if(empt){ buf[i++]= '0'+empt; empt=0;} buf[i++]= char_from_piece(side,found);}
            else empt++;
        }
        if(empt) buf[i++]= '0'+empt;
        if(r>0) buf[i++]='/';
    }
    buf[i++]= ' '; buf[i++]= pos->side==WHITE?'w':'b'; buf[i++]=' ';
    if(pos->castling==0) buf[i++]='-';
    else { if(pos->castling&1) buf[i++]='K'; if(pos->castling&2) buf[i++]='Q'; if(pos->castling&4) buf[i++]='k'; if(pos->castling&8) buf[i++]='q'; }
    buf[i++]=' ';
    if(pos->ep_square<0){ buf[i++]='-';}
    else { buf[i++]= 'a'+ (pos->ep_square%8); buf[i++]= '1'+ (pos->ep_square/8);}
    snprintf(buf+i,sz-i," %d %d",pos->halfmove,pos->fullmove);
}

static uint64_t occ_all(Position *p){ uint64_t r=0; for(int c=0;c<2;c++) for(int pt=0;pt<6;pt++) r|=p->bb[c][pt]; return r; }
static uint64_t occ_color(Position *p,int c){ uint64_t r=0; for(int pt=0;pt<6;pt++) r|=p->bb[c][pt]; return r; }

int in_check(Position *pos,int color){
    uint64_t kb=pos->bb[color][KING];
    if(!kb) return 1;
    int sq=__builtin_ctzll(kb);
    int opp=color^1;
    return square_attacked(pos,sq,opp);
}

/* fix square_attacked pawn handling patch */
int square_attacked_fixed(Position *pos,int sq,int by_color);
/* override via macro? We'll implement in attacks.c separately; call external */

static int pseudo_moves(Position *pos, Move *moves){
    int side=pos->side, opp=side^1;
    int n=0;
    uint64_t us=occ_color(pos,side);
    uint64_t all=occ_all(pos);
    uint64_t th=occ_color(pos,opp);
    /* pawns */
    uint64_t pawns=pos->bb[side][PAWN];
    while(pawns){
        int s=__builtin_ctzll(pawns); pawns&=pawns-1;
        /* pushes */
        int dir = (side==WHITE)?8:-8;
        int one = s+dir;
        if(one>=0 && one<64 && !(all&(1ULL<<one))){
            /* push */
            if((side==WHITE && one>=56) || (side==BLACK && one<8)){
                /* promotion 4 options */
                for(int pr=0;pr<4;pr++) moves[n++]=MAKE_MOVE(s,one,pr,FLAG_PROMO);
            } else moves[n++]=MAKE_MOVE(s,one,0,FLAG_NORMAL);
            /* double */
            int start = (side==WHITE)? (s>=8 && s<=15) : (s>=48 && s<=55);
            if(start){
                int two=s+2*dir;
                if(!(all&(1ULL<<two))) moves[n++]=MAKE_MOVE(s,two,0,FLAG_DOUBLE);
            }
        }
        /* captures */
        uint64_t caps = pawn_attacks(side,s) & th;
        if(pos->ep_square>=0) caps |= (1ULL<<pos->ep_square) & pawn_attacks(side,s);
        uint64_t ccaps=caps;
        while(ccaps){
            int t=__builtin_ctzll(ccaps); ccaps&=ccaps-1;
            int ep = (t==pos->ep_square && !(th&(1ULL<<t)));
            if(ep) {
                moves[n++]=MAKE_MOVE(s,t,0,FLAG_EP);
            } else {
                if((side==WHITE && t>=56)||(side==BLACK && t<8)){
                    for(int pr=0;pr<4;pr++) moves[n++]=MAKE_MOVE(s,t,pr,FLAG_PROMO);
                } else moves[n++]=MAKE_MOVE(s,t,0,FLAG_CAPTURE);
            }
        }
    }
    /* knights */
    uint64_t knights=pos->bb[side][KNIGHT];
    while(knights){
        int s=__builtin_ctzll(knights); knights&=knights-1;
        uint64_t att=knight_attacks(s)&~us;
        uint64_t a=att;
        while(a){ int t=__builtin_ctzll(a); a&=a-1;
            int fl=(th&(1ULL<<t))?FLAG_CAPTURE:FLAG_NORMAL;
            moves[n++]=MAKE_MOVE(s,t,0,fl);
        }
    }
    /* bishops */
    uint64_t bishops=pos->bb[side][BISHOP];
    while(bishops){
        int s=__builtin_ctzll(bishops); bishops&=bishops-1;
        uint64_t att=bishop_attacks(s,all)&~us;
        uint64_t a=att;
        while(a){ int t=__builtin_ctzll(a); a&=a-1;
            int fl=(th&(1ULL<<t))?FLAG_CAPTURE:FLAG_NORMAL;
            moves[n++]=MAKE_MOVE(s,t,0,fl);
        }
    }
    /* rooks */
    uint64_t rooks=pos->bb[side][ROOK];
    while(rooks){
        int s=__builtin_ctzll(rooks); rooks&=rooks-1;
        uint64_t att=rook_attacks(s,all)&~us;
        uint64_t a=att;
        while(a){ int t=__builtin_ctzll(a); a&=a-1;
            int fl=(th&(1ULL<<t))?FLAG_CAPTURE:FLAG_NORMAL;
            moves[n++]=MAKE_MOVE(s,t,0,fl);
        }
    }
    /* queens */
    uint64_t queens=pos->bb[side][QUEEN];
    while(queens){
        int s=__builtin_ctzll(queens); queens&=queens-1;
        uint64_t att=queen_attacks(s,all)&~us;
        uint64_t a=att;
        while(a){ int t=__builtin_ctzll(a); a&=a-1;
            int fl=(th&(1ULL<<t))?FLAG_CAPTURE:FLAG_NORMAL;
            moves[n++]=MAKE_MOVE(s,t,0,fl);
        }
    }
    /* king */
    uint64_t kbb=pos->bb[side][KING];
    if(kbb){
        int s=__builtin_ctzll(kbb);
        uint64_t att=king_attacks(s)&~us;
        uint64_t a=att;
        while(a){ int t=__builtin_ctzll(a); a&=a-1;
            int fl=(th&(1ULL<<t))?FLAG_CAPTURE:FLAG_NORMAL;
            moves[n++]=MAKE_MOVE(s,t,0,fl);
        }
        /* castling */
        if(side==WHITE){
            if((pos->castling&1) && !(all&(1ULL<<5)) && !(all&(1ULL<<6)) && !in_check(pos,WHITE) && !square_attacked(pos,5,BLACK) && !square_attacked(pos,6,BLACK)){
                /* king not in check already; squares f/g not attacked */
                moves[n++]=MAKE_MOVE(4,6,0,FLAG_CASTLE);
            }
            if((pos->castling&2) && !(all&(1ULL<<1)) && !(all&(1ULL<<2)) && !(all&(1ULL<<3)) && !in_check(pos,WHITE) && !square_attacked(pos,2,BLACK) && !square_attacked(pos,3,BLACK)){
                moves[n++]=MAKE_MOVE(4,2,0,FLAG_CASTLE);
            }
        } else {
            if((pos->castling&4) && !(all&(1ULL<<61)) && !(all&(1ULL<<62)) && !in_check(pos,BLACK) && !square_attacked(pos,61,WHITE) && !square_attacked(pos,62,WHITE)){
                moves[n++]=MAKE_MOVE(60,62,0,FLAG_CASTLE);
            }
            if((pos->castling&8) && !(all&(1ULL<<57)) && !(all&(1ULL<<58)) && !(all&(1ULL<<59)) && !in_check(pos,BLACK) && !square_attacked(pos,58,WHITE) && !square_attacked(pos,59,WHITE)){
                moves[n++]=MAKE_MOVE(60,58,0,FLAG_CASTLE);
            }
        }
    }
    return n;
}

int make_move(Position *pos, Move m){
    int from=MOVE_FROM(m), to=MOVE_TO(m), promo=MOVE_PROMO(m), fl=MOVE_FLAGS(m);
    int moved=-1, side=pos->side, opp=side^1;
    for(int pt=0;pt<6;pt++) if(pos->bb[side][pt]&(1ULL<<from)){ moved=pt; break;}
    if(moved<0) return 0;
    int idx=pos->ply;
    if(idx>=MAXPLY) return 0;
    /* cap */
    int cap=-1, capsq=to;
    if(fl==FLAG_EP){
        cap=PAWN;
        capsq = (side==WHITE)? to-8 : to+8;
    } else {
        for(int pt=0;pt<6;pt++) if(pos->bb[opp][pt]&(1ULL<<to)){ cap=pt;break;}
    }
    /* save */
    pos->hist_from[idx]=from;
    pos->hist_to[idx]=to;
    pos->hist_moved[idx]=moved;
    pos->hist_promoted[idx]=-1;
    if(fl==FLAG_PROMO){
        pos->hist_promoted[idx] = (promo==0)?KNIGHT : (promo==1)?BISHOP : (promo==2)?ROOK : QUEEN;
    }
    pos->hist_cap[idx]=cap;
    pos->hist_capsq[idx]=capsq;
    pos->hist_cast[idx]=pos->castling;
    pos->hist_ep[idx]=pos->ep_square;
    pos->hist_half[idx]=pos->halfmove;
    /* apply */
    pos->bb[side][moved] &= ~(1ULL<<from);
    if(cap>=0) pos->bb[opp][cap] &= ~(1ULL<<capsq);
    int placed=moved;
    if(fl==FLAG_PROMO) placed=pos->hist_promoted[idx];
    pos->bb[side][placed] |= (1ULL<<to);
    /* double pawn */
    if(fl==FLAG_DOUBLE){
        pos->ep_square = (side==WHITE)? from+8 : from-8;
    } else pos->ep_square=-1;
    /* castle move rook */
    if(fl==FLAG_CASTLE){
        if(to==6){ pos->bb[side][ROOK] &= ~(1ULL<<7); pos->bb[side][ROOK] |= (1ULL<<5);}
        if(to==2){ pos->bb[side][ROOK] &= ~(1ULL<<0); pos->bb[side][ROOK] |= (1ULL<<3);}
        if(to==62){ pos->bb[side][ROOK] &= ~(1ULL<<63); pos->bb[side][ROOK] |= (1ULL<<61);}
        if(to==58){ pos->bb[side][ROOK] &= ~(1ULL<<56); pos->bb[side][ROOK] |= (1ULL<<59);}
    }
    /* castling rights */
    if(moved==KING){
        if(side==WHITE) pos->castling &= ~3;
        else pos->castling &= ~12;
    }
    if(moved==ROOK){
        if(from==0) pos->castling&=~2;
        if(from==7) pos->castling&=~1;
        if(from==56) pos->castling&=~8;
        if(from==63) pos->castling&=~4;
    }
    if(cap==ROOK){
        if(capsq==0) pos->castling&=~2;
        if(capsq==7) pos->castling&=~1;
        if(capsq==56) pos->castling&=~8;
        if(capsq==63) pos->castling&=~4;
    }
    /* halfmove */
    if(moved==PAWN || cap>=0) pos->halfmove=0; else pos->halfmove++;
    /* fullmove/side */
    int oldside=pos->side;
    if(oldside==BLACK) pos->fullmove++;
    pos->side=opp;
    pos->ply++;
    /* legality: ensure not in check for side that moved */
    (void)0;
    return 1;
}

void unmake_move(Position *pos){
    if(pos->ply==0) return;
    pos->ply--;
    int idx=pos->ply;
    int from=pos->hist_from[idx];
    int to=pos->hist_to[idx];
    int moved=pos->hist_moved[idx];
    int promoted=pos->hist_promoted[idx];
    int cap=pos->hist_cap[idx];
    int capsq=pos->hist_capsq[idx];
    int side=pos->side^1; /* side that moved */
    /* remove placed piece */
    int placed = (promoted>=0)? promoted : moved;
    pos->bb[side][placed] &= ~(1ULL<<to);
    /* restore moved (pawn if promotion) */
    pos->bb[side][moved] |= (1ULL<<from);
    /* restore captured */
    if(cap>=0) pos->bb[side^1][cap] |= (1ULL<<capsq);
    /* restore castle rook */
    /* determine if castle by moved king two-square */
    if(moved==KING && ((from==4 && (to==6||to==2)) || (from==60 && (to==62||to==58)))){
        if(to==6){ pos->bb[side][ROOK] &= ~(1ULL<<5); pos->bb[side][ROOK] |= (1ULL<<7);}
        if(to==2){ pos->bb[side][ROOK] &= ~(1ULL<<3); pos->bb[side][ROOK] |= (1ULL<<0);}
        if(to==62){ pos->bb[side][ROOK] &= ~(1ULL<<61); pos->bb[side][ROOK] |= (1ULL<<63);}
        if(to==58){ pos->bb[side][ROOK] &= ~(1ULL<<59); pos->bb[side][ROOK] |= (1ULL<<56);}
    }
    /* restore state */
    pos->castling=pos->hist_cast[idx];
    pos->ep_square=pos->hist_ep[idx];
    pos->halfmove=pos->hist_half[idx];
    /* side/fullmove */
    if(pos->side==WHITE) {} /* after unmake side becomes mover = opposite of current */
    /* current side is opp of mover; set to mover */
    /* pos->side currently opp; mover = pos->side^1, already computed as side */
    int oldfull=pos->fullmove;
    pos->side=side;
    if(side==BLACK) /* moving side black, fullmove was incremented when move made; decrement */
        pos->fullmove = (pos->fullmove>1)? pos->fullmove-1 : 1;
    else
        pos->fullmove=oldfull;
    (void)oldfull;
}

int legal_moves(Position *pos, Move *moves){
    /* generate pseudo then filter */
    Move tmp[MAXMOVES];
    /* pseudo_moves uses pos->side */
    int n=pseudo_moves(pos,tmp);
    int cnt=0;
    for(int i=0;i<n;i++){
        make_move(pos,tmp[i]);
        int mover = pos->side^1;
        int illegal = in_check(pos, mover);
        unmake_move(pos);
        if(!illegal){
            moves[cnt++]=tmp[i];
        }
    }
    return cnt;
}

int is_game_over(Position *pos){
    Move m[MAXMOVES];
    int n=legal_moves(pos,m);
    if(n==0) return 1;
    if(pos->halfmove>=100) return 1;
    /* repetition not implemented fully */
    return 0;
}