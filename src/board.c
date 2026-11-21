#include "board.h"
#include "move.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

uint64_t knight_att[64];
uint64_t king_att[64];
uint64_t pawn_cap[2][64];

void tables_init(void){
    static int done=0;
    if(done) return;
    for(int sq=0;sq<64;sq++){
        int r=sq/8,f=sq%8;
        uint64_t m=0; int dr[8]={-2,-2,-1,-1,1,1,2,2}; int df[8]={-1,1,-2,2,-2,2,-1,1};
        for(int k=0;k<8;k++){int nr=r+dr[k],nf=f+df[k]; if(nr>=0&&nr<8&&nf>=0&&nf<8)m|=1ULL<<(nr*8+nf);}
        knight_att[sq]=m;
        m=0; for(int ddr=-1;ddr<=1;ddr++)for(int ddf=-1;ddf<=1;ddf++){if(ddr==0&&ddf==0)continue;int nr=r+ddr,nf=f+ddf;if(nr>=0&&nr<8&&nf>=0&&nf<8)m|=1ULL<<(nr*8+nf);}
        king_att[sq]=m;
        for(int side=0;side<2;side++){
            uint64_t pm=0; int dir=(side==WHITE)?1:-1;
            for(int ddf=-1;ddf<=1;ddf+=2){ int nr=r+dir,nf=f+ddf; if(nr>=0&&nr<8&&nf>=0&&nf<8) pm|=1ULL<<(nr*8+nf);}
            pawn_cap[side][sq]=pm;
        }
    }
    done=1;
}

static uint64_t ray_moves(int sq,uint64_t occ,int dr,int df){
    uint64_t res=0; int r=sq/8,f=sq%8; r+=dr; f+=df;
    while(r>=0&&r<8&&f>=0&&f<8){ int ts=r*8+f; uint64_t bit=1ULL<<ts; res|=bit; if(occ&bit) break; r+=dr; f+=df;}
    return res;
}
uint64_t rook_attacks(int sq,uint64_t occ){ uint64_t a=ray_moves(sq,occ,1,0);a|=ray_moves(sq,occ,-1,0);a|=ray_moves(sq,occ,0,1);a|=ray_moves(sq,occ,0,-1);return a;}
uint64_t bishop_attacks(int sq,uint64_t occ){ uint64_t a=ray_moves(sq,occ,1,1);a|=ray_moves(sq,occ,1,-1);a|=ray_moves(sq,occ,-1,1);a|=ray_moves(sq,occ,-1,-1);return a;}

void board_init(Board*b){ tables_init(); memset(b,0,sizeof(*b)); b->ep=-1; board_load_fen(b,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");}

int piece_at(const Board*b,int sq){ for(int p=0;p<12;p++) if(b->bb[p]&(1ULL<<sq)) return p; return -1;}

static void update_occ(Board*b){
    b->occ[2]=0;b->occ[0]=0;b->occ[1]=0;
    for(int p=0;p<12;p++){ b->occ[2]|=b->bb[p]; if(p<6)b->occ[0]|=b->bb[p]; else b->occ[1]|=b->bb[p];}
}

int board_load_fen(Board*b,const char*fen){
    tables_init(); memset(b,0,sizeof(*b)); b->ep=-1; b->halfmove=0; b->fullmove=1; b->castling=0; b->hist_count=0;
    char buf[512]; snprintf(buf,sizeof(buf),"%s",fen);
    char*tok=strtok(buf," ");
    const char*pieceChars="PNBRQKpnbrqk";
    int sq=56;
    for(const char*c=tok;*c && sq>=0;c++){
        if(*c=='/'){sq-=16;continue;}
        if(*c>='1'&&*c<='8'){sq += *c-'0';continue;}
        const char*pc=strchr(pieceChars,*c);
        if(!pc) return -1;
        int pi=pc-pieceChars; b->bb[pi]|=1ULL<<sq; sq++;
    }
    update_occ(b);
    tok=strtok(NULL," "); if(!tok) return -1; b->side=(tok[0]=='w')?WHITE:BLACK;
    tok=strtok(NULL," ");
    if(tok){ if(strchr(tok,'K'))b->castling|=1;if(strchr(tok,'Q'))b->castling|=2;if(strchr(tok,'k'))b->castling|=4;if(strchr(tok,'q'))b->castling|=8;}
    tok=strtok(NULL," ");
    if(tok && tok[0]!='-'){ int f=tok[0]-'a', r=tok[1]-'1'; if(f>=0&&f<8&&r>=0&&r<8)b->ep=r*8+f;}
    tok=strtok(NULL," "); if(tok)b->halfmove=atoi(tok);
    tok=strtok(NULL," "); if(tok)b->fullmove=atoi(tok);
    return 0;
}

void board_to_fen(const Board*b,char*out,size_t size){
    const char*pc="PNBRQKpnbrqk";
    char buf[128]; int pos=0;
    for(int r=7;r>=0;r--){
        int empty=0;
        for(int f=0;f<8;f++){
            int sq=r*8+f; int p=-1; for(int i=0;i<12;i++) if(b->bb[i]&(1ULL<<sq)){p=i;break;}
            if(p==-1){empty++;continue;}
            if(empty){buf[pos++]='0'+empty;empty=0;}
            buf[pos++]=pc[p];
        }
        if(empty){buf[pos++]='0'+empty;}
        if(r>0) buf[pos++]='/';
    }
    buf[pos]=0;
    char cast[5]; int ci=0;
    if(b->castling&1) cast[ci++]='K';
    if(b->castling&2) cast[ci++]='Q';
    if(b->castling&4) cast[ci++]='k';
    if(b->castling&8) cast[ci++]='q';
    if(ci==0) cast[ci++]='-';
    cast[ci]=0;
    char ep[3]; if(b->ep>=0){ep[0]='a'+b->ep%8; ep[1]='1'+b->ep/8; ep[2]=0;}else strcpy(ep,"-");
    snprintf(out,size,"%s %c %s %s %d %d",buf,b->side==WHITE?'w':'b',cast,ep,b->halfmove,b->fullmove);
}

bool square_attacked(const Board*b,int sq,int by_side){
    tables_init();
    uint64_t pawns = b->bb[by_side==WHITE?WP:BP];
    uint64_t pb=pawns;
    while(pb){int ps=__builtin_ctzll(pb);pb&=pb-1;if(pawn_cap[by_side][ps]&(1ULL<<sq))return 1;}
    uint64_t knights=b->bb[by_side==WHITE?WN:BN]; uint64_t kb=knights;
    while(kb){int ks=__builtin_ctzll(kb);kb&=kb-1;if(knight_att[ks]&(1ULL<<sq))return 1;}
    uint64_t ks2=b->bb[by_side==WHITE?WK:BK]; if(ks2){int ksq=__builtin_ctzll(ks2); if(king_att[ksq]&(1ULL<<sq))return 1;}
    uint64_t occ=b->occ[2];
    uint64_t rq=b->bb[by_side==WHITE?WR:BR]|b->bb[by_side==WHITE?WQ:BQ];
    int r=sq/8,f=sq%8;
    int drs[4]={1,-1,0,0},dfs[4]={0,0,1,-1};
    for(int d=0;d<4;d++){int rr=r+drs[d],ff=f+dfs[d]; while(rr>=0&&rr<8&&ff>=0&&ff<8){int ts=rr*8+ff;uint64_t bit=1ULL<<ts;if(occ&bit){if(rq&bit)return 1;break;}rr+=drs[d];ff+=dfs[d];}}
    uint64_t bq=b->bb[by_side==WHITE?WB:BB]|b->bb[by_side==WHITE?WQ:BQ];
    int bds[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    for(int d=0;d<4;d++){int rr=r+bds[d][0],ff=f+bds[d][1]; while(rr>=0&&rr<8&&ff>=0&&ff<8){int ts=rr*8+ff;uint64_t bit=1ULL<<ts;if(occ&bit){if(bq&bit)return 1;break;}rr+=bds[d][0];ff+=bds[d][1];}}
    return 0;
}

bool board_make_move(Board*b,int from,int to,int prom,Undo*u){
    tables_init();
    memset(u,0,sizeof(*u));
    u->from=from; u->to=to; u->prom=prom;
    u->side=b->side;
    u->castling=b->castling;
    u->ep=b->ep;
    u->halfmove=b->halfmove;
    u->fullmove=b->fullmove;
    int p=piece_at(b,from);
    if(p<0) return false;
    u->moved_piece=p;
    int capt=piece_at(b,to);
    int ep_cap_sq=-1;
    if(piece_type((Piece)p)==0 && to==b->ep && capt<0){
        ep_cap_sq=(b->side==WHITE)? to-8 : to+8;
        capt=piece_at(b,ep_cap_sq);
    }
    u->captured_piece=capt;
    u->captured_sq = (ep_cap_sq>=0)? ep_cap_sq : to;
    u->castled=0;
    u->rook_from=-1; u->rook_to=-1;
    // legality: apply temp by modifying b then check king; but do carefully with undo if illegal.
    // remove piece from from
    b->bb[p] &= ~(1ULL<<from);
    if(capt>=0){ b->bb[capt] &= ~(1ULL<<u->captured_sq);}
    // castling rook move
    if(piece_type((Piece)p)==5 && (to-from==2 || from-to==2)){
        u->castled=1;
        if(to-from==2){ u->rook_from=from+3; u->rook_to=from+1;}
        else { u->rook_from=from-4; u->rook_to=from-1;}
        int rp=piece_at(b,u->rook_from);
        b->bb[rp] &= ~(1ULL<<u->rook_from);
        b->bb[rp] |= 1ULL<<u->rook_to;
    }
    // promotion placement
    int placed=p;
    if(prom>0 && piece_type((Piece)p)==0){
        int base = (b->side==WHITE) ? (prom==1?WQ: prom==2?WR: prom==3?WB:WN) : (prom==1?BQ: prom==2?BR: prom==3?BB:BN);
        placed=base;
    }
    b->bb[placed] |= 1ULL<<to;
    // switch side
    b->side = 1-b->side;
    // update castling rights
    if(u->moved_piece==WK){b->castling&=~3;}
    if(u->moved_piece==BK){b->castling&=~12;}
    if(from==0 || u->captured_sq==0) b->castling&=~2;
    if(from==7 || u->captured_sq==7) b->castling&=~1;
    if(from==56 || u->captured_sq==56) b->castling&=~8;
    if(from==63 || u->captured_sq==63) b->castling&=~4;
    // ep
    b->ep=-1;
    if(piece_type((Piece)u->moved_piece)==0 && (to-from==16 || from-to==16)){
        b->ep=(from+to)/2;
    }
    // halfmove
    if(piece_type((Piece)u->moved_piece)==0 || u->captured_piece>=0) b->halfmove=0; else b->halfmove++;
    if(b->side==WHITE) b->fullmove++;
    update_occ(b);
    // legality
    uint64_t myking = (u->side==WHITE)? b->bb[WK] : b->bb[BK];
    int ksq=__builtin_ctzll(myking);
    if(square_attacked(b,ksq,1-u->side)){
        // undo (restore from u by unmake-like direct restore use tmp copy? We already mutated; but can call board_unmake after? board_unmake expects u; u has old values except side restored? board_unmake restores. However board_unmake requires b state after move. It will work.
        board_unmake(b,u);
        return false;
    }
    return true;
}

void board_unmake(Board*b,const Undo*u){
    // restore pre-move state fields
    b->side=u->side;
    b->castling=u->castling;
    b->ep=u->ep;
    b->halfmove=u->halfmove;
    b->fullmove=u->fullmove;
    // remove placed piece from to
    if(u->prom>0 && piece_type((Piece)u->moved_piece)==0){
        // promoted piece at to
        int promoted = piece_at(b,u->to);
        if(promoted>=0) b->bb[promoted]&=~(1ULL<<u->to);
        b->bb[u->moved_piece]|=1ULL<<u->from;
    } else {
        // careful: piece currently at to is moved_piece
        b->bb[u->moved_piece]&=~(1ULL<<u->to);
        b->bb[u->moved_piece]|=1ULL<<u->from;
    }
    // restore captured
    if(u->captured_piece>=0){
        b->bb[u->captured_piece]|=1ULL<<u->captured_sq;
    }
    // un-castle rook
    if(u->castled){
        // rook index
        // find rook piece by color
        // Determine rook piece type
        int rook_piece = (u->side==WHITE)?WR:BR;
        b->bb[rook_piece]&=~(1ULL<<u->rook_to);
        b->bb[rook_piece]|=1ULL<<u->rook_from;
    }
    update_occ(b);
}
