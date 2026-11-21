#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/position.h"
#include "../src/movegen.h"
#include "../src/search.h"
#include "../src/eval.h"

static int tpass=0, ttotal=0;
#define CHECK(cond,msg) do{ ttotal++; if(cond){ tpass++; } else { printf("FAIL: %s\n",msg); } }while(0)

static long perft(Position *p,int depth){
    if(depth==0) return 1;
    Move moves[MAX_MOVES];
    int n=gen_moves(p,moves);
    long nodes=0;
    /* filter legal */
    for(int i=0;i<n;i++){
        Position p2=*p; Undo u;
        pos_make_move(&p2,&moves[i],&u);
        if(in_check(&p2,p->side)) { pos_unmake_move(&p2,&moves[i],&u); continue; }
        nodes += perft(&p2, depth-1);
        pos_unmake_move(&p2,&moves[i],&u);
    }
    return nodes;
}

int main(void){
    /* perft initial */
    Position p; pos_init(&p);
    CHECK(perft(&p,5)==4865609, "perft initial depth5");
    /* Kiwipete */
    Position k; pos_from_fen(&k,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    CHECK(perft(&k,4)==4085603, "perft kiwipete depth4");
    /* invalid fen */
    /* our parser returns 1 even; check missing king triggers? We'll test engine? Instead test king count */
    /* weights load */
    Weights w; CHECK(weights_load("models/model_0.bin",&w)==1, "weights load");
    if(w.w) weights_free(&w);
    /* no warnings not tested here */
    free(p.history); free(k.history);
    printf("%d/%d testes passaram\n", tpass, ttotal);
    return (tpass==ttotal)?0:1;
}