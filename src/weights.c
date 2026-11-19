#include "weights.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

int load_weights(const char *path, float *out, int n){
    if(!path||!out) return 0;
    FILE *f=fopen(path,"rb");
    if(!f) return 0;
    uint32_t version=0, num=0;
    if(fread(&version,4,1,f)!=1){fclose(f);return 0;}
    if(fread(&num,4,1,f)!=1){fclose(f);return 0;}
    /* little-endian assumed */
    if(version!=1 || num!=(uint32_t)n){fclose(f);return 0;}
    size_t got=fread(out,sizeof(float),n,f);
    /* check full file? */
    if(got!=(size_t)n){fclose(f);return 0;}
    for(int i=0;i<n;i++) if(!isfinite(out[i])){fclose(f);return 0;}
    fclose(f);
    return 1;
}