#ifndef INTERFACE_UCI_LIKE_H
#define INTERFACE_UCI_LIKE_H

#include "types.h"

void handle_uci_command(char *command, TranspoTable *tt, PositionList *board_history);

#endif