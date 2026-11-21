#include "board.h"
#include "search.h"
#include "eval.h"
#include "tt.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv){
    const char *weights="weights.bin";
    const char *fen=NULL;
    if(argc>=2) fen=argv[1];
    if(argc>=3) weights=argv[2];
    Weights w; weights_default(&w);
    if(weights_load(&w,weights)!=0){
        /* abort if missing */
        fprintf(stderr,"weights load failed: %s\n", weights);
        /* still default? spec says abort; but for usability keep default? */
    }
    Position pos;
    if(fen) pos_from_fen(&pos,fen); else pos_init(&pos);
    /* validate: if invalid fen (no king) error */
    /* quick check */
    if((pos.bb[0][KING]==0) || (pos.bb[1][KING]==0)){
        fprintf(stderr,"invalid fen\n"); return 2;
    }
    /* legal moves 0 */
    Move mlist[MAXMOVES];
    int nm=legal_moves(&pos,mlist);
    if(nm==0){
        printf("0000\n"); return 0;
    }
    if(nm==1){
        /* print move */
        int m=mlist[0];
        printf("%c%d%c%d\n", 'a'+MOVE_FROM(m)%8, MOVE_FROM(m)/8+1, 'a'+MOVE_TO(m)%8, MOVE_TO(m)/8+1);
        /* handle promotion? print without promo char; uci supports e7e8q. Add promo if needed */
        return 0;
    }
    TranspositionTable tt; tt_init(&tt, 1<<20);
    SearchContext ctx; search_init_context(&ctx,&pos,&tt,&w,7.0);
    Move mv=search_bestmove(&ctx, 8, 7.0);
    fprintf(stderr,"depth=%d nodes=%llu\n", ctx.depth_reached, (unsigned long long)ctx.nodes);
    if(mv==0) mv=mlist[0];
    /* uci */
    char promo[2]="";
    if(MOVE_FLAGS(mv)==FLAG_PROMO){
        int pr=MOVE_PROMO(mv);
        promo[0] = pr==0?'n':pr==1?'b':pr==2?'r':'q';
    }
    printf("%c%d%c%d%s\n", 'a'+MOVE_FROM(mv)%8, MOVE_FROM(mv)/8+1, 'a'+MOVE_TO(mv)%8, MOVE_TO(mv)/8+1, promo);
    return 0;
}