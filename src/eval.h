#ifndef EVAL_H
#define EVAL_H
#include "types.h"
int eval_pos(Position *pos, Weights *w);
int weights_load(Weights *w, const char *path);
int weights_save(Weights *w, const char *path);
void weights_default(Weights *w);
#endif