#include "chess.h"
#include "weights.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void){
    int pass=0, total=0;
    /* perft initial depth 5 */
    {
        Position p; pos_init_start(&p);
        long long nodes=perft(p,5);
        total++; if(nodes==4865609){pass++;} else printf("FAIL perft init5 %lld\n", nodes);
    }
    /* Kiwipete depth4 */
    {
        Position p; pos_from_fen(&p,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        long long nodes=perft(p,4);
        total++; if(nodes==4085603){pass++;} else printf("FAIL kiwipete4 %lld\n", nodes);
    }
    /* pos3 depth5 */
    {
        Position p; pos_from_fen(&p,"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/3B4 w - - 0 1");
        long long nodes=perft(p,5);
        total++; if(nodes==674624){pass++;} else printf("FAIL pos3_5 %lld\n", nodes);
    }
    /* FEN roundtrip */
    {
        Position p; pos_init_start(&p); total++;
        char buf[512]; pos_to_fen(&p,buf,sizeof(buf));
        if(strstr(buf,"rnbqkbnr")) pass++;
    }
    /* weights load invalid */
    {
        float w[NUM_WEIGHTS]; total++;
        if(!load_weights("/no/such.bin",w,NUM_WEIGHTS)) pass++;
    }
    printf("%d/%d testes passaram\n", pass, total);
    return 0;
}