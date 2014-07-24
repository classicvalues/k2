#include "hash.h"
#include "extern.h"
//--------------------------------
#define LIGHT(X, s2m)   ((X) && (((X) & white) == (s2m)))
#define DARK(X, s2m)    ((X) && (((X) & white) != (s2m)))

#define MOVE_FROM_PV    121
#define EQUAL_CAPTURE   119
#define SECOND_KILLER   123
#define FIRST_KILLER    125
#define KING_CAPTURE    255
#define PV_FOLLOW       255
#define BAD_CAPTURES    15

#define APPRICE_NONE    0
#define APPRICE_CAPT    1
#define APPRICE_ALL     2


//--------------------------------
void    InitMoveGen();
int     GenMoves(Move *list, int apprice, Move *best_move);
void    GenPawn(Move *list, int *movCr, short_list<UC, lst_sz>::iterator it);
void    GenPawnCap(Move *list, int *movCr, short_list<UC, lst_sz>::iterator it);
void    GenCastles(Move *list, int *movCr);
int     GenCaptures(Move *list);
void    AppriceMoves(Move *list, int moveCr, Move *best_move);
void    Next(Move *m, int cur, int top, Move *ans);
void    AppriceQuiesceMoves(Move *list, int moveCr);
short   SEE(UC to, short frStreng, short val);
short_list<UC, lst_sz>::reverse_iterator SeeMinAttacker(UC to);
void    AppriceHistory(Move *list, int moveCr);
//--------------------------------
