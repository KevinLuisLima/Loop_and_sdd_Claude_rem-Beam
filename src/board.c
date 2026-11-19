#include "chess.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const int8_t knight_d[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{-1,2},{1,-2},{-1,-2}};
static const int8_t king_d[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

static inline int sq(int f,int r){ return r*8+f; }
static inline int file_of(int s){ return s&7; }
static inline int rank_of(int s){ return s>>3; }

void board_startpos(Board *b){
    memset(b,0,sizeof(*b));
    /* white pieces */
    b->bb[WHITE][ROOK]   = (1ULL<<0)|(1ULL<<7);
    b->bb[WHITE][KNIGHT] = (1ULL<<1)|(1ULL<<6);
    b->bb[WHITE][BISHOP] = (1ULL<<2)|(1ULL<<5);
    b->bb[WHITE][QUEEN]  = (1ULL<<3);
    b->bb[WHITE][KING]   = (1ULL<<4);
    b->bb[WHITE][PAWN]   = 0x000000000000FF00ULL;
    /* black */
    b->bb[BLACK][ROOK]   = (1ULL<<56)|(1ULL<<63);
    b->bb[BLACK][KNIGHT] = (1ULL<<57)|(1ULL<<62);
    b->bb[BLACK][BISHOP] = (1ULL<<58)|(1ULL<<61);
    b->bb[BLACK][QUEEN]  = (1ULL<<59);
    b->bb[BLACK][KING]   = (1ULL<<60);
    b->bb[BLACK][PAWN]   = 0x00FF000000000000ULL;
    b->side = WHITE;
    b->castling = 0x0F;
    b->ep = -1;
    b->halfmove = 0;
    b->fullmove = 1;
}

int board_from_fen(Board *b, const char *fen){
    if(!b||!fen) return -1;
    memset(b,0,sizeof(*b));
    b->ep=-1; b->castling=0; b->side=WHITE; b->halfmove=0; b->fullmove=1;
    char buf[512];
    size_t flen=strlen(fen);
    if(flen>=sizeof(buf)) return -1;
    strcpy(buf,fen);
    char *save=NULL;
    char *tok=strtok_r(buf," ",&save);
    if(!tok) return -1;
    /* pieces */
    int rank=7, file=0;
    for(char *p=tok;*p;p++){
        if(*p=='/'){ rank--; file=0; if(rank<0) return -1; continue; }
        if(*p>='1' && *p<='8'){ file += *p-'0'; if(file>8) return -1; continue; }
        int color=-1, type=-1;
        switch(*p){
            case 'P': color=WHITE; type=PAWN;break;
            case 'N': color=WHITE; type=KNIGHT;break;
            case 'B': color=WHITE; type=BISHOP;break;
            case 'R': color=WHITE; type=ROOK;break;
            case 'Q': color=WHITE; type=QUEEN;break;
            case 'K': color=WHITE; type=KING;break;
            case 'p': color=BLACK; type=PAWN;break;
            case 'n': color=BLACK; type=KNIGHT;break;
            case 'b': color=BLACK; type=BISHOP;break;
            case 'r': color=BLACK; type=ROOK;break;
            case 'q': color=BLACK; type=QUEEN;break;
            case 'k': color=BLACK; type=KING;break;
            default: return -1;
        }
        if(file>=8||rank<0) return -1;
        b->bb[color][type] |= 1ULL << (rank*8+file);
        file++;
    }
    /* ensure one king each */
    if(__builtin_popcountll(b->bb[WHITE][KING])!=1 || __builtin_popcountll(b->bb[BLACK][KING])!=1) return -1;

    tok=strtok_r(NULL," ",&save);
    if(!tok) return -1;
    if(tok[0]=='w') b->side=WHITE; else if(tok[0]=='b') b->side=BLACK; else return -1;

    tok=strtok_r(NULL," ",&save);
    if(!tok) return -1;
    for(char *p=tok;*p;p++){
        switch(*p){
            case 'K': b->castling|=1; break;
            case 'Q': b->castling|=2; break;
            case 'k': b->castling|=4; break;
            case 'q': b->castling|=8; break;
            case '-': break;
            default: return -1;
        }
    }

    tok=strtok_r(NULL," ",&save);
    if(tok && tok[0]!='-'){
        int f=tok[0]-'a', r=tok[1]-'1';
        if(f<0||f>7||r<0||r>7) return -1;
        b->ep = r*8+f;
    }

    tok=strtok_r(NULL," ",&save);
    if(tok){ b->halfmove = atoi(tok); if(b->halfmove<0) return -1; }
    tok=strtok_r(NULL," ",&save);
    if(tok){ b->fullmove = atoi(tok); if(b->fullmove<1) return -1; }
    return 0;
}

int board_to_fen(const Board *b, char *out, size_t n){
    if(!b||!out) return -1;
    char tmp[128]; size_t ti=0;
    for(int r=7;r>=0;r--){
        int empty=0;
        for(int f=0;f<8;f++){
            int sq=r*8+f;
            char ch=0;
            for(int color=0;color<2;color++) for(int type=0;type<6;type++){
                if((b->bb[color][type]>>sq)&1ULL){
                    const char *P="PNBRQK"; const char *p="pnbrqk";
                    ch = color==WHITE? P[type]:p[type];
                }
            }
            if(ch){ if(empty){ if(ti>=sizeof(tmp)-1) return -1; tmp[ti++]= '0'+empty; empty=0;} if(ti>=sizeof(tmp)-1) return -1; tmp[ti++]=ch;}
            else empty++;
        }
        if(empty){ if(ti>=sizeof(tmp)-1) return -1; tmp[ti++]= '0'+empty; }
        if(r>0){ if(ti>=sizeof(tmp)-1) return -1; tmp[ti++]='/'; }
    }
    tmp[ti]=0;
    char cast[5]; int ci=0;
    if(b->castling&1) cast[ci++]='K';
    if(b->castling&2) cast[ci++]='Q';
    if(b->castling&4) cast[ci++]='k';
    if(b->castling&8) cast[ci++]='q';
    if(ci==0) cast[ci++]='-';
    cast[ci]=0;
    char ep[3];
    if(b->ep>=0){ ep[0]='a'+(b->ep&7); ep[1]='1'+(b->ep>>3); ep[2]=0; }
    else { ep[0]='-'; ep[1]=0; }
    snprintf(out,n,"%s %c %s %s %d %d",tmp,b->side==WHITE?'w':'b',cast,ep,b->halfmove,b->fullmove);
    return 0;
}

uint64_t bb_occ(const Board *b,int color){ uint64_t x=0; for(int t=0;t<6;t++) x|=b->bb[color][t]; return x; }
uint64_t bb_all(const Board *b){ return bb_occ(b,WHITE)|bb_occ(b,BLACK); }

/* attack detection */
static uint64_t pawn_attacks(int side,int sq){
    uint64_t b=1ULL<<sq; uint64_t r=0;
    if(side==WHITE){ if((b & ~0x8080808080808080ULL)) r |= b<<7; if((b & ~0x0101010101010101ULL)) r |= b<<9; }
    else { if((b & ~0x0101010101010101ULL)) r |= b>>7; if((b & ~0x8080808080808080ULL)) r |= b>>9; }
    return r;
}
static uint64_t slider_attacks(const Board *b,int sq,int dx,int dy){
    uint64_t r=0; int f=sq&7, rk=sq>>3;
    int nf=f+dx, nr=rk+dy;
    while(nf>=0&&nf<8&&nr>=0&&nr<8){
        int s=nr*8+nf;
        r|=1ULL<<s;
        if((bb_all(b)>>s)&1) break;
        nf+=dx; nr+=dy;
    }
    return r;
}

int is_square_attacked(const Board *b, int sq, int side){
    /* pawns */
    uint64_t occp = side==WHITE? b->bb[BLACK][PAWN] : 0; (void)occp;
    uint64_t pawns = b->bb[side][PAWN];
    if(pawn_attacks(side^1, sq) & pawns) {} /* no */
    /* attackers of sq are side; compute if sq attacked by side */
    /* pawns from opponent? let's do direct: piece (side) attacks sq */
    /* pawn attacks: a pawn of side attacks sq if sq is in its attack set */
    /* reverse: for color side, pawn attacks sq => original pawn position is reverse diagonal */
    {
        uint64_t mask=0;
        if(side==WHITE){
            /* white pawn attacks sq from down-left/down-right */
            if(sq>=9) mask |= 1ULL<<(sq-9); // not exact due files; use masks below guard files
        }
    }
    /* simpler: iterate pawns? up to 8; do bit reverse properly */
    {
        /* for side, pawn capture origins that attack sq */
        uint64_t orig=0;
        int f=sq&7, r=sq>>3;
        if(side==WHITE){
            if(r>0){
                if(f>0) orig |= 1ULL<<((r-1)*8+(f-1));
                if(f<7) orig |= 1ULL<<((r-1)*8+(f+1));
            }
        } else {
            if(r<7){
                if(f>0) orig |= 1ULL<<((r+1)*8+(f-1));
                if(f<7) orig |= 1ULL<<((r+1)*8+(f+1));
            }
        }
        if(orig & b->bb[side][PAWN]) return 1;
    }
    /* knights */
    /* reverse knight positions */
    {
        int f=sq&7, r=sq>>3; uint64_t orig=0;
        for(int i=0;i<8;i++){
            int nf=f - knight_d[i][0], nr=r - knight_d[i][1]; // reverse
            if(nf>=0&&nf<8&&nr>=0&&nr<8) orig |= 1ULL<<(nr*8+nf);
        }
        if(orig & b->bb[side][KNIGHT]) return 1;
    }
    /* king */
    {
        int f=sq&7, r=sq>>3; uint64_t orig=0;
        for(int i=0;i<8;i++){
            int nf=f - king_d[i][0], nr=r - king_d[i][1];
            if(nf>=0&&nf<8&&nr>=0&&nr<8) orig |= 1ULL<<(nr*8+nf);
        }
        if(orig & b->bb[side][KING]) return 1;
    }
    /* bishops/queens diagonals */
    for(int d=0; d<4; d++){
        static const int dx[4]={1,1,-1,-1}; static const int dy[4]={1,-1,1,-1};
        uint64_t r=slider_attacks(b,sq,dx[d],dy[d]);
        if(r & (b->bb[side][BISHOP]|b->bb[side][QUEEN])) return 1;
    }
    /* rooks/queens orthogonals */
    {
        int dxo[4]={1,-1,0,0}, dyo[4]={0,0,1,-1};
        for(int d=0;d<4;d++){
            uint64_t r=slider_attacks(b,sq,dxo[d],dyo[d]);
            if(r & (b->bb[side][ROOK]|b->bb[side][QUEEN])) return 1;
        }
    }
    return 0;
}

int in_check(const Board *b, int side){
    /* find king */
    uint64_t k=b->bb[side][KING];
    if(!k) return 1;
    int sq=__builtin_ctzll(k);
    return is_square_attacked(b,sq,side^1);
}

void make_move(Board *b, Move m){
    int from=m.from, to=m.to;
    int color=b->side;
    int opp=color^1;
    /* find piece */
    int ptype=-1;
    for(int t=0;t<6;t++) if((b->bb[color][t]>>from)&1ULL){ ptype=t; break; }
    assert(ptype>=0);
    int capture=0;
    for(int t=0;t<6;t++){
        if((b->bb[opp][t]>>to)&1ULL){ capture=1; b->bb[opp][t] &= ~(1ULL<<to);}
    }
    if(ptype==PAWN && to==b->ep && ((from&7)!=(to&7))){ capture=1; }
    /* en passant capture */
    if(ptype==PAWN && to==b->ep && ((from&7)!=(to&7))){
        int capsq = color==WHITE? to-8 : to+8;
        b->bb[opp][PAWN] &= ~(1ULL<<capsq);
    }
    /* move piece */
    b->bb[color][ptype] &= ~(1ULL<<from);
    /* promotion */
    int final=ptype;
    if(m.promo){
        final = m.promo; /* 1..4? we'll use piece types */
        /* remove pawn */
    } else if(ptype==PAWN && (to>>3==7 || to>>3==0)){
        final=QUEEN; /* default queen if promo 0? handle */
    }
    /* if promotion requested */
    if(m.promo>=KNIGHT && m.promo<=QUEEN){
        final=m.promo;
    } else if (ptype==PAWN && (to>>3==0 || to>>3==7)){
        final=QUEEN;
    }
    b->bb[color][final] |= 1ULL<<to;
    /* castling rook move */
    if(ptype==KING){
        if(from==4 && to==6){ /* white short */
            b->bb[WHITE][ROOK] &= ~(1ULL<<7);
            b->bb[WHITE][ROOK] |= 1ULL<<5;
        } else if(from==4 && to==2){
            b->bb[WHITE][ROOK] &= ~(1ULL<<0);
            b->bb[WHITE][ROOK] |= 1ULL<<3;
        } else if(from==60 && to==62){
            b->bb[BLACK][ROOK] &= ~(1ULL<<63);
            b->bb[BLACK][ROOK] |= 1ULL<<61;
        } else if(from==60 && to==58){
            b->bb[BLACK][ROOK] &= ~(1ULL<<56);
            b->bb[BLACK][ROOK] |= 1ULL<<59;
        }
    }
    /* castling rights update & ep */
    int newep=-1;
    if(ptype==PAWN && abs(to-from)==16){
        newep = (from+to)/2;
    }
    /* remove pawn if moved? done */
    /* rights */
    if(ptype==KING){
        if(color==WHITE){ b->castling &= ~3; }
        else { b->castling &= ~12; }
    }
    if(ptype==ROOK){
        if(color==WHITE){
            if(from==0) b->castling &= ~2;
            if(from==7) b->castling &= ~1;
        } else {
            if(from==56) b->castling &= ~8;
            if(from==63) b->castling &= ~4;
        }
    }
    /* if rook captured on corner, rights update */
    if(to==0) b->castling &= ~2; /* white queen side rook captured */
    if(to==7) b->castling &= ~1;
    if(to==56) b->castling &= ~8;
    if(to==63) b->castling &= ~4;

    b->ep=newep;
    if(ptype==PAWN || /* capture? */
       (0)){}
    /* halfmove clock */
    if(ptype==PAWN || capture) b->halfmove=0;
    else b->halfmove++;
    /* Actually reset on capture: detect if to originally occupied by enemy (we removed) impossible now. We'll set in caller? Use simple: increment, reset handled below approx. */
    if(color==BLACK) b->fullmove++;
    b->side = opp;
}

int move_is_legal(Board b, Move m){
    Board c=b;
    make_move(&c,m);
    /* king of mover (now opp side's opponent) not in check */
    int mover = b.side;
    /* after move side switched; check mover king */
    if(in_check(&c, mover)) return 0;
    /* also ensure move from has right piece */
    return 1;
}

uint64_t perft(Board b,int depth){
    if(depth==0) return 1;
    Move moves[MAX_MOVES];
    int n=gen_legal_moves(&b,moves);
    if(depth==1) return (uint64_t)n;
    uint64_t total=0;
    for(int i=0;i<n;i++){
        Board cb=b;
        make_move(&cb,moves[i]);
        total += perft(cb,depth-1);
    }
    return total;
}