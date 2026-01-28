#ifndef EVAL_H
#define EVAL_H

#include <stdlib.h>
#include "types.h"

bool eval_game_ended(PositionList *board_history, int *result);
int eval(PositionList *board_history);
void init_eval_tables();

#endif