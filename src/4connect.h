#ifndef CONNECT_HEADER
#define CONNECT_HEADER

//#define NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if __OS2__
#include "os2disp.h"
#include "listener\dbgpipe.h"
#endif

#ifdef DOS
#include <conio.h>
#include "listener\dbgpipe.h"
#endif

#ifdef LINUX
#include "linuxdisp.h"
#include "listener/dbgpipe.h"
#endif

#ifdef BEOS
#include "listener/dbgpipe.h"
#endif 

// Configuration:
#define COUNT4CONNECT     // counts 4 connect possibilities in rows, columns and diagonally
//#define COUNTEXACT        // exact but slower and plays worse?!?!
//#define HASHTABLE      // use hashtable
// #define SEARCH4CONNECT // otherwise use bit vectors
#define IN2OUT_ORDER      // otherwise iterate from column 1 to column 7
// #define STATSTICS      // counts calls of Evaluate
//#define MAINVAR        // use main variante
//#define BONUS
// #define INCREMENTAL_EVAL // !calculates position values during AlphaBeta
// #define DEBUG_OUTPUT   // !displays move valuation
// #define DEEPSORT       // !


#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#define ROWS    6  // 0 < ROWS <= COLUMNS
#define COLUMNS 7  // COLUMNS <= 8

#define CONNECT (1 << COLUMNS) // number of bit combinations of 7 bits

#define INF 32767
#define WIN 32000

#define VALUE4 700
#define VALUE3 20
#define VALUE2 10


#define PLAYER1 0
#define PLAYER2 1
#define FREE    2

#define NEXTPLAYER playerOnMove = !playerOnMove
#define OPPONENT   (!playerOnMove)

#define POSSIBLEMOVE(column) piecesInColumn[column]

#ifdef STATISTICS
extern struct _statistics {
   long evaluates;
} statistics;
#endif

extern struct _moveList {
   char column, row;
} moveList[ROWS*COLUMNS];           // move list stack
extern int moveListTop;             // points to first elem in list
// read only variables:
// extern int depth;
extern int movesLeft;
extern char playerOnMove;

extern char gameField[ROWS][COLUMNS]; // pieces on field
extern char piecesInColumn[COLUMNS];

#ifdef HASHTABLE
long hc, hcIndex;
#endif

void NewGame(char player);
void Init(void);
char PlayerMakeMove(char column);
char BestMove(int depth, int *score);
char Undo(int times);
char Is4Connect(char *winner);
int Save4Connect(FILE *file);
int Restore4Connect(FILE *file);
int Evaluate(void);
#ifdef __cplusplus
}
#endif

#endif
