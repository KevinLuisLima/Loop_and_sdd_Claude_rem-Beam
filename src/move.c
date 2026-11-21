#include "move.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void add_move(MoveList*ml,int from,int to,int prom){ if(ml->count<256){ ml->moves[ml->count].from=from; ml->moves[ml->count].to=to; ml->moves[ml->count].prom=prom; ml->moves[ml->count].score=0; ml->count++;}}

static uint64_t pawn_moves(const Board*b,int sq,MoveList*ml,int captures){
    int side=b->side; int r=sq/8,f=sq%8; (void)captures;
    uint64_t res=0; (void)res;
    // one step
    int one; if(side==WHITE) one=sq+8; else one=sq-8;
    if(one>=0&&one<64 && piece_at(b,one)<0){
        // two steps
        if((side==WHITE && r==1)||(side==BLACK && r==6)){
            int two = (side==WHITE)?sq+16:sq-16;
            if(piece_at(b,two)<0) { if(captures==0) add_move(ml,sq,two,0);}
        }
        if(captures==0) add_move(ml,sq,one,0);
        // promotion
    }
    // captures
    uint64_t caps = pawn_cap[side][sq] & (side==WHITE?b->occ[1]:b->occ[0]);
    // en passant
    if(b->ep>=0 && (pawn_cap[side][sq]&(1ULL<<b->ep))) caps |= (1ULL<<b->ep);
    uint64_t c=caps; while(c){ int ts=__builtin_ctzll(c); c&=c-1; add_move(ml,sq,ts,0);}
    (void)r;(void)f;
    return 0;
}

void gen_moves(const Board*b, MoveList*ml, bool captures_only){
    ml->count=0;
    tables_init();
    int side=b->side;
    for(int sq=0;sq<64;sq++){
        int p=piece_at(b,sq);
        if(p<0 || (p<6?WHITE:BLACK)!=side) continue;
        int pt=p%6;
        uint64_t targets=0;
        if(pt==0){ pawn_moves(b,sq,ml,0); continue;} // handled
        if(pt==1) targets=knight_att[sq] & (side==WHITE?~b->occ[0]:~b->occ[1]);
        else if(pt==5) targets=king_att[sq] & (side==WHITE?~b->occ[0]:~b->occ[1]);
        else if(pt==3) targets=rook_attacks(sq,b->occ[2]) & (side==WHITE?~b->occ[0]:~b->occ[1]);
        else if(pt==2) targets=bishop_attacks(sq,b->occ[2]) & (side==WHITE?~b->occ[0]:~b->occ[1]);
        else if(pt==4) targets=(rook_attacks(sq,b->occ[2])|bishop_attacks(sq,b->occ[2])) & (side==WHITE?~b->occ[0]:~b->occ[1]);
        uint64_t t=targets;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,sq,ts,0);}
    }
    // castling
    if(side==WHITE){
        if(b->castling&1 && piece_at(b,4)==WK && piece_at(b,5)<0 && piece_at(b,6)<0 && !square_attacked(b,4,BLACK) && !square_attacked(b,5,BLACK) && !square_attacked(b,6,BLACK)) add_move(ml,4,6,0);
        if(b->castling&2 && piece_at(b,4)==WK && piece_at(b,3)<0 && piece_at(b,2)<0 && piece_at(b,1)<0 && !square_attacked(b,4,BLACK) && !square_attacked(b,3,BLACK) && !square_attacked(b,2,BLACK)) add_move(ml,4,2,0);
    } else {
        if(b->castling&4 && piece_at(b,60)==BK && piece_at(b,61)<0 && piece_at(b,62)<0 && !square_attacked(b,60,WHITE) && !square_attacked(b,61,WHITE) && !square_attacked(b,62,WHITE)) add_move(ml,60,62,0);
        if(b->castling&8 && piece_at(b,60)==BK && piece_at(b,59)<0 && piece_at(b,58)<0 && piece_at(b,57)<0 && !square_attacked(b,60,WHITE) && !square_attacked(b,59,WHITE) && !square_attacked(b,58,WHITE)) add_move(ml,60,58,0);
    }
    // promotion moves: fix pawn moves to last rank generate prom
    // Re-generate promotion handling: adjust moves list? We added moves with prom=0; those with pawn to last rank should become 4. Easier: in gen, detect.
    // We'll post-process: any pawn move to last rank -> replace single move with 4
    MoveList tmp=*ml; ml->count=0;
    for(int i=0;i<tmp.count;i++){
        Move m=tmp.moves[i];
        int p=piece_at(b,m.from);
        if(p>=0 && p%6==0){ int toR=m.to/8; if((b->side==WHITE && toR==7)||(b->side==BLACK && toR==0)){
            add_move(ml,m.from,m.to,1); add_move(ml,m.from,m.to,2); add_move(ml,m.from,m.to,3); add_move(ml,m.from,m.to,4); continue;
        }}
        // captures_only filter optional
        if(captures_only){
            if(piece_at(b,m.to)<0 && !(b->ep==m.to && p%6==0)) continue;
        }
        add_move(ml,m.from,m.to,0);
    }
    // if captures_only, remove non captures already done
}

void gen_legal_moves(const Board*b, MoveList*ml){
    MoveList pseudo; gen_moves(b,&pseudo,false);
    ml->count=0;
    for(int i=0;i<pseudo.count;i++){
        Board copy=*b;
        Undo u; if(board_make_move(&copy,pseudo.moves[i].from,pseudo.moves[i].to,pseudo.moves[i].prom,&u)){
            add_move(ml,pseudo.moves[i].from,pseudo.moves[i].to,pseudo.moves[i].prom);
        }
    }
}

bool is_legal(const Board*b, Move m){
    Board copy=*b; Undo u; return board_make_move(&copy,m.from,m.to,m.prom,&u);
}

int move_to_uci(Move m, char*out){
    out[0]='a'+m.from%8; out[1]='1'+m.from/8; out[2]='a'+m.to%8; out[3]='1'+m.to/8;
    if(m.prom>0){ const char*pr="qrbn"; out[4]=pr[m.prom-1]; out[5]=0;} else out[4]=0;
    return 0;
}
int parse_move_uci(const Board*b, const char*s, Move*m){
    if(strlen(s)<4) return -1;
    int from=(s[1]-'1')*8+(s[0]-'a');
    int to=(s[3]-'1')*8+(s[2]-'a');
    int prom=0;
    if(s[4]){
        if(s[4]=='q') prom=1;
        if(s[4]=='r') prom=2;
        if(s[4]=='b') prom=3;
        if(s[4]=='n') prom=4;
    }
    m->from=from;m->to=to;m->prom=prom;m->score=0;
    // validate legality
    MoveList ml; gen_legal_moves(b,&ml);
    for(int i=0;i<ml.count;i++) if(ml.moves[i].from==from && ml.moves[i].to==to && ml.moves[i].prom==prom) return 0;
    return -1;
}
