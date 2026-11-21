#include "attacks.h"
#include "types.h"
#include <assert.h>

static uint64_t kn[64], kg[64], pw[2][64];

static const int knight_dx[8] = { -2,-2,-1,-1,1,1,2,2 };
static const int knight_dy[8] = { -1,1,-2,2,-2,2,-1,1 };

void init_attacks(void){
    static int done=0; if(done) return; done=1;
    for(int s=0;s<64;s++){
        int x=s%8, y=s/8;
        uint64_t k=0,km=0;
        for(int i=0;i<8;i++){
            int nx=x+knight_dx[i], ny=y+knight_dy[i];
            if(nx>=0&&nx<8&&ny>=0&&ny<8) k |= 1ULL << (ny*8+nx);
        }
        kn[s]=k;
        for(int dx=-1;dx<=1;dx++) for(int dy=-1;dy<=1;dy++){
            if(dx==0&&dy==0) continue;
            int nx=x+dx, ny=y+dy;
            if(nx>=0&&nx<8&&ny>=0&&ny<8) km |= 1ULL << (ny*8+nx);
        }
        kg[s]=km;
        /* pawns */
        for(int c=0;c<2;c++){
            uint64_t p=0;
            if(c==0){
                if(x>0 && y<7) p |= 1ULL << (s+7);
                if(x<7 && y<7) p |= 1ULL << (s+9);
            } else {
                if(x>0 && y>0) p |= 1ULL << (s-9);
                if(x<7 && y>0) p |= 1ULL << (s-7);
            }
            pw[c][s]=p;
        }
    }
}

uint64_t knight_attacks(int sq){ return kn[sq]; }
uint64_t king_attacks(int sq){ return kg[sq]; }
uint64_t pawn_attacks(int color,int sq){ return pw[color][sq]; }

uint64_t bishop_attacks(int sq, uint64_t occ){
    uint64_t res=0;
    int x=sq%8,y=sq/8;
    /* NE */
    for(int i=1;x+i<8 && y+i<8;i++){ int s=(y+i)*8+(x+i); res|=1ULL<<s; if(occ&(1ULL<<s)) break; }
    /* NW */
    for(int i=1;x-i>=0 && y+i<8;i++){ int s=(y+i)*8+(x-i); res|=1ULL<<s; if(occ&(1ULL<<s)) break; }
    /* SE */
    for(int i=1;x+i<8 && y-i>=0;i++){ int s=(y-i)*8+(x+i); res|=1ULL<<s; if(occ&(1ULL<<s)) break; }
    /* SW */
    for(int i=1;x-i>=0 && y-i>=0;i++){ int s=(y-i)*8+(x-i); res|=1ULL<<s; if(occ&(1ULL<<s)) break; }
    return res;
}

uint64_t rook_attacks(int sq, uint64_t occ){
    uint64_t res=0;
    int x=sq%8,y=sq/8;
    for(int i=x+1;i<8;i++){ int s=y*8+i; res|=1ULL<<s; if(occ&(1ULL<<s)) break;}
    for(int i=x-1;i>=0;i--){ int s=y*8+i; res|=1ULL<<s; if(occ&(1ULL<<s)) break;}
    for(int i=y+1;i<8;i++){ int s=i*8+x; res|=1ULL<<s; if(occ&(1ULL<<s)) break;}
    for(int i=y-1;i>=0;i--){ int s=i*8+x; res|=1ULL<<s; if(occ&(1ULL<<s)) break;}
    return res;
}

uint64_t queen_attacks(int sq,uint64_t occ){ return bishop_attacks(sq,occ)|rook_attacks(sq,occ); }

static uint64_t occ_all(Position *pos){ return pos->bb[0][0]|pos->bb[0][1]|pos->bb[0][2]|pos->bb[0][3]|pos->bb[0][4]|pos->bb[0][5]|pos->bb[1][0]|pos->bb[1][1]|pos->bb[1][2]|pos->bb[1][3]|pos->bb[1][4]|pos->bb[1][5]; }

int square_attacked(Position *pos, int sq, int by_color){
    /* pawn */
    if(by_color==WHITE){
        if(sq>=9 && (sq%8)>0 && (pos->bb[WHITE][PAWN]&(1ULL<<(sq-9)))) return 1;
        if(sq>=7 && (sq%8)<7 && (pos->bb[WHITE][PAWN]&(1ULL<<(sq-7)))) return 1;
    } else {
        if(sq<=54 && (sq%8)>0 && (pos->bb[BLACK][PAWN]&(1ULL<<(sq+7)))) return 1;
        if(sq<=56 && (sq%8)<7 && (pos->bb[BLACK][PAWN]&(1ULL<<(sq+9)))) return 1;
    }
    if(pos->bb[by_color][KNIGHT] & knight_attacks(sq)) return 1;
    if(pos->bb[by_color][KING] & king_attacks(sq)) return 1;
    uint64_t o = occ_all(pos);
    if(pos->bb[by_color][BISHOP] & bishop_attacks(sq,o)) return 1;
    if(pos->bb[by_color][ROOK] & rook_attacks(sq,o)) return 1;
    if(pos->bb[by_color][QUEEN] & queen_attacks(sq,o)) return 1;
    return 0;
}