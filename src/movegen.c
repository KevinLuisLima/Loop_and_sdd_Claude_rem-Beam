#include "chess.h"
#include <string.h>

static const int8_t knight_d[8][2]={{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{-1,2},{1,-2},{-1,-2}};
static const int8_t king_d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

static void add_move(Move*moves,int* n,int from,int to,int promo){
    if(*n<MAX_MOVES){ moves[*n].from=from; moves[*n].to=to; moves[*n].promo=promo; (*n)++; }
}

int gen_pseudo(const Board *b, Move *moves){
    int n=0; int color=b->side; int opp=color^1;
    uint64_t occ_all=bb_all(b); uint64_t occ_own=bb_occ(b,color); uint64_t occ_opp=bb_occ(b,opp);
    /* pawns */
    uint64_t pawns=b->bb[color][PAWN];
    while(pawns){
        int sq=__builtin_ctzll(pawns); pawns&=pawns-1;
        int r=sq>>3;
        if(color==WHITE){
            /* push 1 */
            int s1=sq+8;
            if(r<7 && !((occ_all>>s1)&1ULL)){
                if(r==6){ for(int pr=KNIGHT;pr<=QUEEN;pr++) add_move(moves,&n,sq,s1,pr); }
                else add_move(moves,&n,sq,s1,0);
            }
            /* push 2 */
            if(r==1){
                int s2=sq+16;
                if(!((occ_all>>s2)&1ULL) && !((occ_all>>(sq+8))&1ULL)) add_move(moves,&n,sq,s2,0);
            }
            /* captures */
            int caps[2]={sq+7,sq+9};
            for(int ci=0;ci<2;ci++){
                int s=caps[ci];
                if(s<0||s>63) continue;
                int sr=s>>3;
                if(sr!=r+1) continue; /* wrap guard */
                if((occ_opp>>s)&1ULL){
                    if(r==6){ for(int pr=KNIGHT;pr<=QUEEN;pr++) add_move(moves,&n,sq,s,pr); }
                    else add_move(moves,&n,sq,s,0);
                } else if(s==b->ep){
                    add_move(moves,&n,sq,s,0);
                }
            }
        } else {
            int s1=sq-8;
            if(r>0 && !((occ_all>>s1)&1ULL)){
                if(r==1){ for(int pr=KNIGHT;pr<=QUEEN;pr++) add_move(moves,&n,sq,s1,pr); }
                else add_move(moves,&n,sq,s1,0);
            }
            if(r==6){
                int s2=sq-16;
                if(!((occ_all>>s2)&1ULL) && !((occ_all>>(sq-8))&1ULL)) add_move(moves,&n,sq,s2,0);
            }
            int caps[2]={sq-9,sq-7};
            for(int ci=0;ci<2;ci++){
                int s=caps[ci];
                if(s<0||s>63) continue;
                int sr=s>>3;
                if(sr!=r-1) continue;
                if((occ_opp>>s)&1ULL){
                    if(r==1){ for(int pr=KNIGHT;pr<=QUEEN;pr++) add_move(moves,&n,sq,s,pr); }
                    else add_move(moves,&n,sq,s,0);
                } else if(s==b->ep){
                    add_move(moves,&n,sq,s,0);
                }
            }
        }
    }
    /* knights */
    uint64_t kn=b->bb[color][KNIGHT];
    while(kn){ int sq=__builtin_ctzll(kn); kn&=kn-1; int f=sq&7,r=sq>>3;
        for(int i=0;i<8;i++){ int nf=f+knight_d[i][0], nr=r+knight_d[i][1]; if(nf<0||nf>7||nr<0||nr>7) continue; int s=nr*8+nf; if((occ_own>>s)&1ULL) continue; add_move(moves,&n,sq,s,0);}
    }
    /* king */
    uint64_t kk=b->bb[color][KING];
    if(kk){ int sq=__builtin_ctzll(kk); int f=sq&7,r=sq>>3;
        for(int i=0;i<8;i++){ int nf=f+king_d[i][0], nr=r+king_d[i][1]; if(nf<0||nf>7||nr<0||nr>7) continue; int s=nr*8+nf; if((occ_own>>s)&1ULL) continue; add_move(moves,&n,sq,s,0);}
        int oppr=color^1;
        /* castling */
        if(color==WHITE && sq==4){
            if((b->castling&1)&& !((occ_all>>5)&1ULL)&&!((occ_all>>6)&1ULL) && !is_square_attacked(b,4,oppr)&&!is_square_attacked(b,5,oppr)&&!is_square_attacked(b,6,oppr)) add_move(moves,&n,4,6,0);
            if((b->castling&2)&& !((occ_all>>1)&1ULL)&&!((occ_all>>2)&1ULL)&&!((occ_all>>3)&1ULL) && !is_square_attacked(b,4,oppr)&&!is_square_attacked(b,3,oppr)&&!is_square_attacked(b,2,oppr)) add_move(moves,&n,4,2,0);
        }
        if(color==BLACK && sq==60){
            if((b->castling&4)&& !((occ_all>>61)&1ULL)&&!((occ_all>>62)&1ULL) && !is_square_attacked(b,60,oppr)&&!is_square_attacked(b,61,oppr)&&!is_square_attacked(b,62,oppr)) add_move(moves,&n,60,62,0);
            if((b->castling&8)&& !((occ_all>>57)&1ULL)&&!((occ_all>>58)&1ULL)&&!((occ_all>>59)&1ULL) && !is_square_attacked(b,60,oppr)&&!is_square_attacked(b,59,oppr)&&!is_square_attacked(b,58,oppr)) add_move(moves,&n,60,58,0);
        }
    }
    /* sliders */
    for(int sq=0;sq<64;sq++){
        if((b->bb[color][ROOK]>>sq)&1ULL || (b->bb[color][QUEEN]>>sq)&1ULL){
            static const int dx[4]={1,-1,0,0}; static const int dy[4]={0,0,1,-1};
            for(int d=0;d<4;d++){
                int f=sq&7,r=sq>>3; f+=dx[d]; r+=dy[d];
                while(f>=0&&f<8&&r>=0&&r<8){
                    int s=r*8+f; if((occ_own>>s)&1ULL) break; add_move(moves,&n,sq,s,0); if((occ_opp>>s)&1ULL) break; f+=dx[d]; r+=dy[d];
                }
            }
        }
        if((b->bb[color][BISHOP]>>sq)&1ULL || (b->bb[color][QUEEN]>>sq)&1ULL){
            static const int dx[4]={1,1,-1,-1}; static const int dy[4]={1,-1,1,-1};
            for(int d=0;d<4;d++){
                int f=sq&7,r=sq>>3; f+=dx[d]; r+=dy[d];
                while(f>=0&&f<8&&r>=0&&r<8){
                    int s=r*8+f; if((occ_own>>s)&1ULL) break; add_move(moves,&n,sq,s,0); if((occ_opp>>s)&1ULL) break; f+=dx[d]; r+=dy[d];
                }
            }
        }
    }
    return n;
}

int gen_legal_moves(const Board *b, Move *moves){
    Move ps[MAX_MOVES];
    int n=gen_pseudo(b,ps);
    int legal=0;
    for(int i=0;i<n;i++){
        Board cb=*b;
        make_move(&cb,ps[i]);
        int mover=b->side;
        if(!in_check(&cb,mover)){
            moves[legal++]=ps[i];
        }
    }
    return legal;
}