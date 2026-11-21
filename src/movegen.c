#include "movegen.h"
#include "position.h"
#include "bitboard.h"
#include <string.h>

static void add_move(Move *m,int from,int to,int prom,int captured,int castle,int ep){
    m->from=from; m->to=to; m->promoted=prom; m->captured=captured;
    m->castle=castle; m->ep=ep; m->flags=0;
}

static int is_occ(Position*p,int sq){ return piece_at(p,sq)!=EMPTY; }

int gen_moves(Position *p, Move *moves){
    int n=0;
    int side=p->side;
    u64 occ=0; for(int i=0;i<12;i++) occ|=p->bb[i];
    u64 own = side==0 ? (p->bb[W_P]|p->bb[W_N]|p->bb[W_B]|p->bb[W_R]|p->bb[W_Q]|p->bb[W_K])
                      : (p->bb[B_P]|p->bb[B_N]|p->bb[B_B]|p->bb[B_R]|p->bb[B_Q]|p->bb[B_K]);
    u64 opp = occ ^ own;

    /* pawns */
    if(side==0){
        /* pushes */
        u64 pawns=p->bb[W_P];
        /* single push */
        u64 single = (pawns<<8) & ~occ;
        u64 promo_single = single & 0xFF00000000000000ULL;
        u64 normal_single = single & ~promo_single;
        /* iterate */
        /* normal pushes */
        u64 t=normal_single;
        while(t){ int to=ctz64(t); t&=t-1; int from=to-8; add_move(&moves[n++],from,to,-1,EMPTY,0,0); }
        t=promo_single;
        while(t){ int to=ctz64(t); t&=t-1; int from=to-8; add_move(&moves[n++],from,to,4,EMPTY,0,0);
add_move(&moves[n++],from,to,3,EMPTY,0,0);
add_move(&moves[n++],from,to,2,EMPTY,0,0);
add_move(&moves[n++],from,to,1,EMPTY,0,0); } /* prom order: q,r,b,n? store index */
        /* double push */
        u64 dbl = ((pawns & 0x000000000000FF00ULL)<<16) & ~occ & ~((occ)<<8?0:0);
        /* ensure intermediate empty */
        u64 intmask = (pawns & 0x000000000000FF00ULL)<<8;
        dbl = intmask & ~occ; dbl = (dbl<<8) & ~occ;
        t=dbl;
        while(t){ int to=ctz64(t); t&=t-1; int from=to-16; add_move(&moves[n++],from,to,-1,EMPTY,0,0); }
        /* captures */
        u64 capL = (pawns<<7) & opp & ~0x8080808080808080ULL;
        u64 capR = (pawns<<9) & opp & ~0x0101010101010101ULL;
        /* promo captures */
        u64 capLp = capL & 0xFF00000000000000ULL;
        u64 capLn = capL & ~capLp;
        u64 capRp = capR & 0xFF00000000000000ULL;
        u64 capRn = capR & ~capRp;
        t=capLn; while(t){ int to=ctz64(t); t&=t-1; int from=to-7; int cap=piece_at(p,to); add_move(&moves[n++],from,to,-1,cap,0,0); }
        t=capRn; while(t){ int to=ctz64(t); t&=t-1; int from=to-9; int cap=piece_at(p,to); add_move(&moves[n++],from,to,-1,cap,0,0); }
        t=capLp; while(t){ int to=ctz64(t); t&=t-1; int from=to-7; int cap=piece_at(p,to); add_move(&moves[n++],from,to,4,cap,0,0);
add_move(&moves[n++],from,to,3,cap,0,0);
add_move(&moves[n++],from,to,2,cap,0,0);
add_move(&moves[n++],from,to,1,cap,0,0); }
        t=capRp; while(t){ int to=ctz64(t); t&=t-1; int from=to-9; int cap=piece_at(p,to); add_move(&moves[n++],from,to,4,cap,0,0);
add_move(&moves[n++],from,to,3,cap,0,0);
add_move(&moves[n++],from,to,2,cap,0,0);
add_move(&moves[n++],from,to,1,cap,0,0); }
        /* ep */
        if(p->ep>=0){
            int ep=p->ep;
            /* white pawn can capture to ep if adjacent */
            if(ep/8==5){
                if(ep%8>0){ int from=ep-9; if((p->bb[W_P]>>(from))&1ULL) add_move(&moves[n++],from,ep,-1,B_P,0,1); }
                if(ep%8<7){ int from=ep-7; if((p->bb[W_P]>>(from))&1ULL) add_move(&moves[n++],from,ep,-1,B_P,0,1); }
            }
        }
    } else {
        u64 pawns=p->bb[B_P];
        /* pushes downward */
        u64 single = (pawns>>8) & ~occ;
        u64 promo_single = single & 0x00000000000000FFULL;
        u64 normal_single = single & ~promo_single;
        u64 t=normal_single;
        while(t){ int to=ctz64(t); t&=t-1; int from=to+8; add_move(&moves[n++],from,to,-1,EMPTY,0,0); }
        t=promo_single;
        while(t){ int to=ctz64(t); t&=t-1; int from=to+8; add_move(&moves[n++],from,to,4,EMPTY,0,0);
add_move(&moves[n++],from,to,3,EMPTY,0,0);
add_move(&moves[n++],from,to,2,EMPTY,0,0);
add_move(&moves[n++],from,to,1,EMPTY,0,0); }
        u64 dblmask = (pawns & 0x00FF000000000000ULL)>>8;
        u64 dbl = dblmask & ~occ; dbl = (dbl>>8) & ~occ;
        t=dbl;
        while(t){ int to=ctz64(t); t&=t-1; int from=to+16; add_move(&moves[n++],from,to,-1,EMPTY,0,0); }
        u64 capL = (pawns>>7) & opp & ~0x0101010101010101ULL;
        u64 capR = (pawns>>9) & opp & ~0x8080808080808080ULL;
        u64 capLp = capL & 0x00000000000000FFULL;
        u64 capLn = capL & ~capLp;
        u64 capRp = capR & 0x00000000000000FFULL;
        u64 capRn = capR & ~capRp;
        t=capLn; while(t){ int to=ctz64(t); t&=t-1; int from=to+7; int cap=piece_at(p,to); add_move(&moves[n++],from,to,-1,cap,0,0); }
        t=capRn; while(t){ int to=ctz64(t); t&=t-1; int from=to+9; int cap=piece_at(p,to); add_move(&moves[n++],from,to,-1,cap,0,0); }
        t=capLp; while(t){ int to=ctz64(t); t&=t-1; int from=to+7; int cap=piece_at(p,to); add_move(&moves[n++],from,to,4,cap,0,0);
add_move(&moves[n++],from,to,3,cap,0,0);
add_move(&moves[n++],from,to,2,cap,0,0);
add_move(&moves[n++],from,to,1,cap,0,0); }
        t=capRp; while(t){ int to=ctz64(t); t&=t-1; int from=to+9; int cap=piece_at(p,to); add_move(&moves[n++],from,to,4,cap,0,0);
add_move(&moves[n++],from,to,3,cap,0,0);
add_move(&moves[n++],from,to,2,cap,0,0);
add_move(&moves[n++],from,to,1,cap,0,0); }
        if(p->ep>=0){
            int ep=p->ep;
            if(ep/8==2){
                if(ep%8>0){ int from=ep+7; if((p->bb[B_P]>>(from))&1ULL) add_move(&moves[n++],from,ep,-1,W_P,0,1); }
                if(ep%8<7){ int from=ep+9; if((p->bb[B_P]>>(from))&1ULL) add_move(&moves[n++],from,ep,-1,W_P,0,1); }
            }
        }
    }

    /* knights */
    {
        static const int ko[8][2]={{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
        u64 pcs = side==0? p->bb[W_N]:p->bb[B_N];
        u64 tmp=pcs;
        while(tmp){ int from=ctz64(tmp); tmp&=tmp-1;
            int r=from/8,c=from%8;
            for(int d=0;d<8;d++){
                int nr=r+ko[d][0], nc=c+ko[d][1];
                if(nr<0||nr>7||nc<0||nc>7) continue;
                int to=nr*8+nc;
                if((own>>to)&1ULL) continue;
                int cap = is_occ(p,to)? piece_at(p,to): EMPTY;
                add_move(&moves[n++],from,to,-1,cap,0,0);
            }
        }
    }
    /* bishops */
    {
        static const int bd[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
        u64 pcs = side==0? p->bb[W_B]:p->bb[B_B];
        u64 tmp=pcs; (void)own;
        while(tmp){ int from=ctz64(tmp); tmp&=tmp-1;
            int r=from/8,c=from%8;
            for(int d=0;d<4;d++){
                int rr=r+bd[d][0], cc=c+bd[d][1];
                while(rr>=0&&rr<8&&cc>=0&&cc<8){
                    int to=rr*8+cc;
                    if((own>>to)&1ULL) break;
                    int cap = is_occ(p,to)? piece_at(p,to):EMPTY;
                    add_move(&moves[n++],from,to,-1,cap,0,0);
                    if((opp>>to)&1ULL) break;
                    rr+=bd[d][0]; cc+=bd[d][1];
                }
            }
        }
    }
    /* rooks */
    {
        static const int rd[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
        u64 pcs = side==0? p->bb[W_R]:p->bb[B_R];
        u64 tmp=pcs;
        while(tmp){ int from=ctz64(tmp); tmp&=tmp-1;
            int r=from/8,c=from%8;
            for(int d=0;d<4;d++){
                int rr=r+rd[d][0], cc=c+rd[d][1];
                while(rr>=0&&rr<8&&cc>=0&&cc<8){
                    int to=rr*8+cc;
                    if((own>>to)&1ULL) break;
                    int cap = is_occ(p,to)? piece_at(p,to):EMPTY;
                    add_move(&moves[n++],from,to,-1,cap,0,0);
                    if((opp>>to)&1ULL) break;
                    rr+=rd[d][0]; cc+=rd[d][1];
                }
            }
        }
    }
    /* queens */
    {
        static const int qd[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        u64 pcs = side==0? p->bb[W_Q]:p->bb[B_Q];
        u64 tmp=pcs;
        while(tmp){ int from=ctz64(tmp); tmp&=tmp-1;
            int r=from/8,c=from%8;
            for(int d=0;d<8;d++){
                int rr=r+qd[d][0], cc=c+qd[d][1];
                while(rr>=0&&rr<8&&cc>=0&&cc<8){
                    int to=rr*8+cc;
                    if((own>>to)&1ULL) break;
                    int cap = is_occ(p,to)? piece_at(p,to):EMPTY;
                    add_move(&moves[n++],from,to,-1,cap,0,0);
                    if((opp>>to)&1ULL) break;
                    rr+=qd[d][0]; cc+=qd[d][1];
                }
            }
        }
    }
    /* king */
    {
        static const int kd[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        u64 pcs = side==0? p->bb[W_K]:p->bb[B_K];
        if(pcs){
            int from=ctz64(pcs);
            int r=from/8,c=from%8;
            for(int d=0;d<8;d++){
                int nr=r+kd[d][0], nc=c+kd[d][1];
                if(nr<0||nr>7||nc<0||nc>7) continue;
                int to=nr*8+nc;
                if((own>>to)&1ULL) continue;
                int cap = is_occ(p,to)? piece_at(p,to):EMPTY;
                add_move(&moves[n++],from,to,-1,cap,0,0);
            }
        }
    }
    /* castling */
    if(side==0){
        /* king at 4, rights bits 1,2 */
        if((p->bb[W_K] & (1ULL<<4))){
            if((p->castling&1) && !(occ & ((1ULL<<5)|(1ULL<<6))) ){
                /* squares 5,6 not attacked */
                if(!in_check(p,0) && !(attacks_to(p,5,1)) && !(attacks_to(p,6,1))){
                    add_move(&moves[n++],4,6,-1,EMPTY,1,0);
                }
            }
            if((p->castling&2) && !(occ & ((1ULL<<1)|(1ULL<<2)|(1ULL<<3))) ){
                if(!in_check(p,0) && !(attacks_to(p,2,1)) && !(attacks_to(p,3,1))){
                    add_move(&moves[n++],4,2,-1,EMPTY,2,0);
                }
            }
        }
    } else {
        if((p->bb[B_K] & (1ULL<<60))){
            if((p->castling&4) && !(occ & ((1ULL<<61)|(1ULL<<62))) ){
                if(!in_check(p,1) && !(attacks_to(p,61,0)) && !(attacks_to(p,62,0))){
                    add_move(&moves[n++],60,62,-1,EMPTY,1,0);
                }
            }
            if((p->castling&8) && !(occ & ((1ULL<<57)|(1ULL<<58)|(1ULL<<59))) ){
                if(!in_check(p,1) && !(attacks_to(p,58,0)) && !(attacks_to(p,59,0))){
                    add_move(&moves[n++],60,58,-1,EMPTY,2,0);
                }
            }
        }
    }

    /* filter illegal: keep moves where king not in check after make */
    /* Move from pseudo to legal array */
    Move tmp[MAX_MOVES];
    memcpy(tmp,moves,n*sizeof(Move));
    int legal=0;
    for(int i=0;i<n;i++){
        Position p2=*p;
        Undo u;
        pos_make_move(&p2,&tmp[i],&u);
        if(!in_check(&p2, side)){ /* side to move before is side; after move, side flipped; original side's king safety checked before make? Actually check side to move after? We need check original side king not attacked */
            /* in_check takes side param for side's king; after make, original side no longer to move but king position same; check original side */
            if(!in_check(&p2, side)) moves[legal++]=tmp[i];
        }
        /* also need undo not necessary as p2 copy */
    }
    return legal;
}

int gen_captures(Position *p, Move *moves){
    /* generate all moves then filter captures/promo */
    int total=gen_moves(p,moves);
    int n=0;
    Move tmp[MAX_MOVES]; memcpy(tmp,moves,total*sizeof(Move));
    for(int i=0;i<total;i++){
        if(tmp[i].captured!=EMPTY || tmp[i].ep || tmp[i].promoted>=0) moves[n++]=tmp[i];
    }
    return n;
}