#ifndef EVAL_H
#define EVAL_H

#include <stdlib.h>
#include "types.h"

int eval(PositionList *board_history);
void init_eval_tables();

#endif