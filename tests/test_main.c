#include "chess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int failures=0, total=0;
#define CHECK(cond,msg) do{ total++; if(!(cond)){ failures++; printf("FAIL: %s\n",msg);} else printf("PASS: %s\n",msg);}while(0)

int main(void){
    /* perft initial */
    Board b; board_startpos(&b);
    CHECK(perft(b,1)==20,"perft depth1");
    CHECK(perft(b,2)==400,"perft depth2");
    CHECK(perft(b,3)==8902,"perft depth3");
    CHECK(perft(b,4)==197281,"perft depth4");
    CHECK(perft(b,5)==4865609,"perft depth5");
    /* kiwipete */
    Board k;
    int ok=board_from_fen(&k,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    CHECK(ok==0,"kiwipete parse");
    CHECK(perft(k,4)==4085603,"kiwipete perft4");
    /* position3 */
    Board p3;
    ok=board_from_fen(&p3,"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/3B4 w - - 0 1");
    CHECK(ok==0,"pos3 parse");
    CHECK(perft(p3,5)==1168111,"pos3 perft5");
    /* fen roundtrip */
    char fen[256]; board_to_fen(&b,fen,sizeof(fen));
    Board b2; CHECK(board_from_fen(&b2,fen)==0,"fen roundtrip parse");
    /* invalid fen */
    Board invalid; CHECK(board_from_fen(&invalid,"garbage")==-1,"invalid fen");
    /* legal moves count initial 20 */
    Move mv[MAX_MOVES]; CHECK(gen_legal_moves(&b,mv)==20,"initial legal moves");
    /* weights load invalid */
    CHECK(eval_load("/nonexistent.bin")==-1,"weights missing");
    /* one legal move returns? */
    /* memory bounds not measured */
    printf("%d/%d testes passaram\n", total-failures, total);
    return failures?1:0;
}