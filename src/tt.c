#include "tt.h"
#include <stdlib.h>
#include <string.h>
void tt_init(TranspositionTable *tt, size_t bytes){
    /* power of two */
    size_t s=1;
    while(s*2*sizeof(TTEntry)<=bytes) s*=2;
    tt->size=s;
    tt->entries=calloc(s,sizeof(TTEntry));
    tt->mask=s-1;
}
void tt_clear(TranspositionTable *tt){ for(size_t i=0;i<tt->size;i++) tt->entries[i].key=0; }
TTEntry *tt_probe(TranspositionTable *tt, uint64_t key){
    TTEntry *e=&tt->entries[key & tt->mask];
    if(e->key==key) return e;
    return NULL;
}
void tt_store(TranspositionTable *tt, uint64_t key, int depth, int score, int flag, int best){
    TTEntry *e=&tt->entries[key & tt->mask];
    e->key=key; e->depth=depth; e->score=score; e->flag=flag; e->best=best;
}