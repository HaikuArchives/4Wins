/*
   Program: 4connect.c
   Autor: Michael Pfeiffer
   Begin date: 24.11.1997

   Limits: ROWS and COLUMNS must be less than 9 because bit vectors
           hold only 8 bits
*/   
              
#ifdef _WINDOWS
#include <windows.h>
#include "4conwin.h"
#endif

#include <memory.h>
#include "4connect.h"
#include <stdlib.h>

#if __OS2__
	#define INLINE _Inline    // ICC
#else
	#if __BORLANDC__
		#define INLINE  // BORLAND C
	#else
		#ifdef LINUX 
		#define INLINE
		#else
			#ifdef BEOS
			#define INLINE
			extern void CopyGameField(); 
			#else
			#define INLINE static _inline // MSVC++
			#endif
		#endif
	#endif
#endif

#define EXTENDED_SEARCH_INCREASE 3
#define MAX_SEARCH_DEPTH 23


#ifdef STATISTICS
struct _statistics statstics;
#endif

struct _moveList moveList[ROWS*COLUMNS]; // move list stack

int moveListTop;  // points to first elem in list
//static int movesPlayedTop;   // moveListTop before call of AlphaBeta

int movesLeft;

static int depth = 0;
char playerOnMove;                  // player

char gameField[ROWS][COLUMNS];      // pieces on playfield
char piecesInColumn[COLUMNS];       // means positons left in column

// Indexes into bvDiagonalBL and bvDiagonalTL
#define TL_INDEX(row, column) ROWS-1+column-row
#define BL_INDEX(row, column) column + row

// and-bit-masks
#define H_ANDMASK ((1 << COLUMNS)-1)) // not required
#define V_ANDMASK ((1 << ROWS)-1)

static BYTE bvDiagonalAndMask[ROWS+COLUMNS-1];

// bit vectors
static BYTE bvHorizontal[2][ROWS],
     bvVertical[2][COLUMNS],
     bvDiagonalBL[2][ROWS+COLUMNS-1],         // bottom left to top right
     bvDiagonalTL[2][ROWS+COLUMNS-1];         // top left to bottom right

#define CONNECT4POSSIBLE 2
#define CONNECT3POSSIBLE 1
#define CONNECT2POSSIBLE 0

/*static*/ BYTE connect4[CONNECT],
     connectPossible[3][CONNECT], // 2, 3, 4 connect possible
     numberOfBits[CONNECT]; // number of bits set

#ifdef BONUS
static int bonus[ROWS][COLUMNS] = {{0,10,19,20,18,10,0},
                                   {0,10,15,17,16,10,0},
                                   {0,10,13,14,12,10,0},
                                   {0,10,10,10,10,10,0},
                                   {0,0,0,0,0,0,0},
                                   {0,0,0,0,0,0,0}};
int bonusScore = 0; // relative to PLAYER1
#endif

#ifdef MAINVAR
static signed char mainVar[MAX_SEARCH_DEPTH][MAX_SEARCH_DEPTH];
static signed char helpMainVar[MAX_SEARCH_DEPTH];
#endif

#ifdef COUNTEXACT
int rowScore[(1 << ROWS) * (1 << ROWS)];
#endif

#ifdef HASHTABLE
   	#define TRUESCORE 0
   	#define LOWERBOUND 1
   	#define UPPERBOUND 2

    #define MIN_DEPTH 3
    #define MAX_DEPTH (MAX_SEARCH_DEPTH+1)

	#define HTSTRUCT

	#ifdef HTSTRUCT
   	typedef struct _hashtableEntry {
      	int score;
	    char depth;
      	char status;
      	long hashCode;
   	} HashTableEntry;

   	#define HASHTABLE_SIZE   (1 << 12)

	static HashTableEntry hashTable[HASHTABLE_SIZE];
	static HashTableEntry *lastEntry = &hashTable[HASHTABLE_SIZE-1];
    //lastEntry = &hashTable[HASHTABLE_SIZE-1];
	#else
    	#define HASHTABLE_SIZE   (1 << 14)

		int htScore[HASHTABLE_SIZE];
		char htDepth[HASHTABLE_SIZE];
		char htStatus[HASHTABLE_SIZE];
		long htHC[HASHTABLE_SIZE];
	#endif	
	
    #define HASHTABLE_MODULO (HASHTABLE_SIZE-1)

static long hashCode[ROWS*COLUMNS*2];

/*static*/ long hashCodeIndex[ROWS*COLUMNS*2];
/*static*/ long hc = 0, hcIndex = 0;
#endif

#ifdef HASHTABLE
void InitHashtable(void)
{
int i;
   srand(1);

   for (i = 0; i < ROWS*COLUMNS*2; i++)
   {
      hashCode[i] = ((unsigned)rand()) | ((unsigned)rand() << 16);
      hashCodeIndex[i] = ((unsigned)rand()) | ((unsigned)rand() << 16);
   }
}

void ClearHashtable(void)
{ 
	#ifdef HTSTRUCT
		memset(hashTable, 0, sizeof(hashTable));
	#else
		memset(htScore, 0, sizeof(htScore));
		memset(htDepth, 0, sizeof(htDepth));
		memset(htStatus, 0, sizeof(htStatus));
		memset(htHC, 0, sizeof(htHC));
	#endif
}

#define SEARCH_WIDTH 5
INLINE void HTInsert(int score, int depth, int alpha, int beta)
{
#ifdef HTSTRUCT
HashTableEntry *entry = &hashTable[hcIndex & HASHTABLE_MODULO];
int width = SEARCH_WIDTH;
#endif
	#ifdef HTSTRUCT
   	do
   	{
      	if ((depth >= entry->depth) || (entry->hashCode != hc))
      	{
         	entry->score = score;
         	entry->depth = depth;
         	entry->hashCode = hc;
         	if (score < alpha)
            	entry->status = UPPERBOUND;
         	else if (score > beta)
	            entry->status = LOWERBOUND;
    	    else
            	entry->status = TRUESCORE;
         	return;
      	}
      	if (entry == lastEntry) entry = hashTable; else entry++;
      	width --;
   } while (width != 0);
   #else
   #endif
}

INLINE int HTFind(int *score, int depth, int *alpha, int *beta)
{
#ifdef HTSTRUCT
HashTableEntry *entry = &hashTable[hcIndex & HASHTABLE_MODULO];
int width = SEARCH_WIDTH;
#endif
   #ifdef HTSTRUCT
   do
   {
      	if ((entry->depth >= depth) && (entry->hashCode == hc))
      	{
			switch (entry->status)
			{
				case TRUESCORE:
					*score = entry->score;
					// *beta = -20000;
					return TRUE;
				case UPPERBOUND:
					if (entry->score < *beta) *beta = entry->score + 1;
					if (*alpha > *beta)
					{
						*score = *alpha;
						return TRUE;
					}
					else
						return FALSE;
				case LOWERBOUND:
					if (entry->score > *alpha) *alpha = entry->score - 1;
					if (*alpha > *beta)
					{
						*score = *alpha;
						return TRUE;
					}
					else
						return FALSE;
			}
			return FALSE;
/*         	*score = entry->score;
         	return ((entry->status == OK) ||
            	((entry->status == LOWERBOUND) && (*score < alpha)) ||
            	((entry->status == UPPERBOUND) && (*score > beta)));*/
      	}

      	if (entry == lastEntry) entry = hashTable; else entry++;
      	width --;
   } while (width != 0);
   return FALSE;
   #else
   #endif
}
#endif

static void setConnectPossible(char n, char index, char bit)
{
	if (n >= 2)
    {
    	if (n > 4) n = 4;
        n -= 2;
        assert((n >= 0) && (n <= 2));
        assert((index >= 0) && ((unsigned)index <= CONNECT-1));
       	assert((bit >= 0) && (bit <= 6));
        connectPossible[n][index] |= 1 << bit;
    }
}

#ifdef LINUX
#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#endif

#ifdef BEOS
int fmin(int a, int b) {
	return (a < b) ? a : b;
}

int fmax(int a, int b) {
	return (a > b) ? a : b;
}
#endif

// Initialization of tables
void Init(void)
{
int i, j, max, n;
char set;
char start, pos, q, z;
   	NewGame(PLAYER1);

    for (i = 0; i < CONNECT; i++)
    {
    	n = max = 0; set = FALSE; // counts max number of sequential bits
        numberOfBits[i] = 0;
        for (j = 0; j < 8; j++)
        {
        if (i & (1 << j)) numberOfBits[i]++;

        if (set)
        {
        	if (i & (1 << j))
            	n++;
			else
                set = FALSE;
        }
        else
        	if (i & (1 << j))
            {
            	set = TRUE; n = 1;
            }
            if (max < n) max = n;
        }
        connect4[i] = max >= 4; // at least 4 connect?

        // state machine:
        // sets bits where 2, 3 or 4 connect is possible
        connectPossible[0][i] = connectPossible[1][i] = connectPossible[2][i] = 0;
        q = 1;
        for (j = 0; j < COLUMNS; j++)
        {
        	z = i & (1 << j);
            switch(q)
            {
            case 1:
                  if (z) { start = j; q = 2; }
                  else { pos = j; q = 3; }
                  break;
            case 2:
                  if (!z) { pos = j; q = 4; }
                  break;
            case 3:
                  if (z) { start = j; q = 6; }
                  else pos = j;
                  break;
            case 4:
                  if (z) q = 5;
                  else
                  {
                     setConnectPossible((char)(pos-start+1), (char)i, (char)pos);
                     pos = j; q = 3;
                  }
                  break;
            case 5:
                  if (!z)
                  {
                     setConnectPossible((char)(j-start), (char)i, (char)pos);
                     start = pos+1; pos = j; q = 4;
                  }
                  break;
            case 6:
                  if (!z)
                  {
                     setConnectPossible((char)(j-start+1), (char)i, (char)pos);
                     pos = j; q = 4;
                  }
                  break;
            }
        }
        switch (q)
       	{
        case 4:
            setConnectPossible((char)(pos-start+1), (char)i, (char)pos);
            break;
        case 5:
            setConnectPossible((char)(j-start), (char)i, (char)pos);
            break;
        case 6:
            setConnectPossible((char)(j-start+1), (char)i, (char)pos);
            break;
		}
	}
/*
	for (i = 0; i < COLUMNS+ROWS-1; i++)
	{
	#ifdef BEOS
	int from = fmax(0, i-fmin(ROWS, COLUMNS)+1), to = fmin(i, fmax(COLUMNS, ROWS)-1);
	#else
	int from = max(0, i-min(ROWS, COLUMNS)+1), to = min(i, max(COLUMNS, ROWS)-1);
	#endif
	}
*/
	#ifdef HASHTABLE
		InitHashtable();
	#endif

	#ifdef COUNTEXACT
	{
	char player, opponent;
		for (player = 0; player < (1 << ROWS); player++)
		{
			for (opponent = 0; opponent < (1 << ROWS); opponent ++)
			{
	        	if (player > opponent)     
    	     		rowScore[player | (opponent << ROWS)] = 
    	     		4*numberOfBits[player]-numberOfBits[opponent];
         		else if (player < opponent)
            	    rowScore[player | (opponent << ROWS)] = 
            	    numberOfBits[player] - 4*numberOfBits[opponent];
			}
		}
	}
	#endif
}

// Initialization of global variables for a new game
void NewGame(char StartPlayer)
{
char i, j;
	assert((StartPlayer == PLAYER1) || (StartPlayer == PLAYER2));
	for (i = 0; i < ROWS; i++)
	{
		bvHorizontal[0][i] = bvHorizontal[1][i] = 0;
		for (j = 0; j < COLUMNS; j++)
			gameField[i][j] = FREE;
	}

	for (i = 0; i < COLUMNS; i++)
	{
		piecesInColumn[i] = ROWS;
		bvVertical[1][i] = bvVertical[0][i] = 0;
	}

	for (i = 0; i < ROWS+COLUMNS-1; i++)
        bvDiagonalTL[0][i] = bvDiagonalTL[1][i] =
        bvDiagonalBL[0][i] = bvDiagonalBL[1][i] = 0;

    moveListTop = -1;
    //movesPlayedTop = -1;
    movesLeft = COLUMNS * ROWS;
    playerOnMove = StartPlayer;
   	#ifdef BONUS
    	bonusScore = 0;
   	#endif
    #if defined(_WINDOWS) | defined(BEOS)
		CopyGameField();	
	#endif   	
}

INLINE void MarkBitVectors(char row, char column)
{
BYTE bit = 1 << column;
#ifdef HASHTABLE
int htIndex;
#endif
	#if 1
   	if (playerOnMove == PLAYER1)
   	{
		bvHorizontal[PLAYER1][row] ^= bit;

        assert((row+column >= 0) && (row+column < ROWS+COLUMNS));
        bvDiagonalBL[PLAYER1][BL_INDEX(row,column)] ^= bit;

        assert((TL_INDEX(row, column) >= 0) && (TL_INDEX(row, column) < ROWS+COLUMNS));
        bvDiagonalTL[PLAYER1][TL_INDEX(row, column)] ^= bit;

        bvVertical[PLAYER1][column] ^= 1 << row;

      	#ifdef HASHTABLE
        	htIndex = PLAYER1 | ((COLUMNS*row+column)<<1);
         	hc ^= hashCode[htIndex];
         	hcIndex ^= hashCodeIndex[htIndex];
      	#endif
	}
    else
    {
    	bvHorizontal[PLAYER2][row] ^= bit;

        assert((row+column >= 0) && (row+column < ROWS+COLUMNS));
        bvDiagonalBL[PLAYER2][BL_INDEX(row,column)] ^= bit;

        assert((TL_INDEX(row, column) >= 0) && (TL_INDEX(row, column) < ROWS+COLUMNS));
        bvDiagonalTL[PLAYER2][TL_INDEX(row, column)] ^= bit;

        bvVertical[PLAYER2][column] ^= 1 << row;

      	#ifdef HASHTABLE
        	htIndex = PLAYER2 | ((COLUMNS*row+column)<<1);
         	hc ^= hashCode[htIndex];
         	hcIndex ^= hashCodeIndex[htIndex];
      	#endif
	}
    #else
        bvHorizontal[playerOnMove][row] ^= bit;
    
        assert((row+column >= 0) && (row+column < ROWS+COLUMNS));
        bvDiagonalBL[playerOnMove][BL_INDEX(row,column)] ^= bit;

        assert((TL_INDEX(row, column) >= 0) && (TL_INDEX(row, column) < ROWS+COLUMNS));
        bvDiagonalTL[playerOnMove][TL_INDEX(row, column)] ^= bit;

        bvVertical[playerOnMove][column] ^= 1 << row;

		#ifdef HASHTABLE
			htIndex = playerOnMove | ((COLUMNS*row+column)<<1);
        	hc ^= hashCode[htIndex];
        	hcIndex ^= hashCodeIndex[htIndex];
    	#endif
    #endif
}

INLINE void _MakeMove(char column)
{
char row;
	assert(moveListTop < ROWS*COLUMNS-1);
	row = --piecesInColumn[column];
    assert(row >= 0);

    moveListTop++;

    moveList[moveListTop].row = row;
    moveList[moveListTop].column = column;

    gameField[row][column] = playerOnMove;
    MarkBitVectors(row, column);
    #ifdef BONUS
		if (playerOnMove == PLAYER1)
			bonusScore += bonus[row][column];
		else
            bonusScore -= bonus[row][column];
	#endif
	NEXTPLAYER;
	movesLeft--;

	depth++;
}

INLINE void _UnmakeMove(void)
{
char column = moveList[moveListTop].column,
     row = moveList[moveListTop].row;
	moveListTop--;
    assert(moveListTop >= -1);
    assert(piecesInColumn[column] < ROWS);
    assert(moveList[moveListTop+1].row == piecesInColumn[column]);
    gameField[row][column] = FREE;
    (piecesInColumn[column])++;
    NEXTPLAYER;
    movesLeft++;

    #ifdef BONUS
    	if (playerOnMove == PLAYER1)
        	bonusScore -= bonus[row][column];
        else
            bonusScore += bonus[row][column];
    #endif
    MarkBitVectors(row, column);
    depth--;
}

/* caused last move 4 in a row? */
char Is4Connect(char *winner)
{
char column, row;
      if (moveListTop == -1) return FALSE;
#ifndef SEARCH4CONNECT
      column = moveList[moveListTop].column;
      row = piecesInColumn[column];
      assert(row == moveList[moveListTop].row);
      *winner = gameField[row][column];
      #if 1
      return (playerOnMove == PLAYER1) ?
            connect4[bvHorizontal[PLAYER2][row]] ||
            connect4[bvVertical[PLAYER2][column]] ||
            connect4[bvDiagonalBL[PLAYER2][BL_INDEX(row, column)]] ||
            connect4[bvDiagonalTL[PLAYER2][TL_INDEX(row, column)]]
            :
    	    connect4[bvHorizontal[PLAYER1][row]] ||
            connect4[bvVertical[PLAYER1][column]] ||
            connect4[bvDiagonalBL[PLAYER1][BL_INDEX(row, column)]] ||
            connect4[bvDiagonalTL[PLAYER1][TL_INDEX(row, column)]];
      #else
      return connect4[bvHorizontal[OPPONENT][row]] ||
            connect4[bvVertical[OPPONENT][column]] ||
            connect4[bvDiagonalBL[OPPONENT][BL_INDEX(row, column)]] ||
            connect4[bvDiagonalTL[OPPONENT][TL_INDEX(row, column)]];
    #endif
#else
      column = moveList[moveListTop].column;
      row = piecesInColumn[column];
      assert(row == moveList[moveListTop].row);

      assert(row < ROWS);
      *winner = player = gameField[row][column];

      n = 0;
      x = column; y = row+1;
      while ((y < ROWS) && (gameField[y][x] == player)) { n++; y++; }
      if (n >= 3) return 1;

      n = 0; x = column-1; y = row;
      while ((x >= 0) && (gameField[y][x] == player)) { n++; x--; };
      x = column+1;
      while ((x < COLUMNS) && (gameField[y][x] == player)) { n++; x++; };
      if (n >= 3) return 1;

      n = 0; x = column-1; y = row-1;
      while ((x >= 0) && (y >= 0) && (gameField[y][x] == player)) { n++; x--; y--; };
      x = column+1; y = row+1;
      while ((x < COLUMNS) && (y < ROWS) && (gameField[y][x] == player)) { n++; x++; y++; };
      if (n >= 3) return 1;

      n = 0; x = column-1; y = row+1;
      while ((x >= 0) && (y < ROWS) && (gameField[y][x] == player)) { n++; x--; y++; };
      x = column+1; y = row-1;
      while ((x < COLUMNS) && (y >= 0) && (gameField[y][x] == player)) { n++; x++; y--; };
      if (n >= 3) return 1;

      return 0;
#endif
}

static char is4poss(char column)
{
//char *table = connectPossible[2];
char row, bit, rowBit; //, opponent = OPPONENT;

   assert(piecesInColumn[column] > 0);

   row = piecesInColumn[column]-1;
   bit = 1 << column; rowBit = 1 << row;

#if 1
   return ((connectPossible[CONNECT4POSSIBLE][bvHorizontal[PLAYER1][row]] & bit) ||
       (connectPossible[CONNECT4POSSIBLE][bvVertical[PLAYER1][column]] & rowBit) ||
       (connectPossible[CONNECT4POSSIBLE][bvDiagonalTL[PLAYER1][TL_INDEX(row, column)]] & bit) ||
       (connectPossible[CONNECT4POSSIBLE][bvDiagonalBL[PLAYER1][BL_INDEX(row, column)]] & bit) ||

       (connectPossible[CONNECT4POSSIBLE][bvHorizontal[PLAYER2][row]] & bit) ||
       (connectPossible[CONNECT4POSSIBLE][bvVertical[PLAYER2][column]] & rowBit) ||
       (connectPossible[CONNECT4POSSIBLE][bvDiagonalTL[PLAYER2][TL_INDEX(row, column)]] & bit) ||
       (connectPossible[CONNECT4POSSIBLE][bvDiagonalBL[PLAYER2][BL_INDEX(row, column)]] & bit));
#else
   return ((table[bvHorizontal[playerOnMove][row]] & bit) ||
       (table[bvVertical[playerOnMove][column]] & rowBit) ||
       (table[bvDiagonalTL[playerOnMove][TL_INDEX(row, column)]] & bit) ||
       (table[bvDiagonalBL[playerOnMove][BL_INDEX(row, column)]] & bit) ||

       (table[bvHorizontal[opponent][row]] & bit) ||
       (table[bvVertical[opponent][column]] & rowBit) ||
       (table[bvDiagonalTL[opponent][TL_INDEX(row, column)]] & bit) ||
       (table[bvDiagonalBL[opponent][BL_INDEX(row, column)]] & bit));
#endif
}

static char is3poss(char column)
{
//char *table = connectPossible[1];
char row, bit, rowBit; //, opponent = OPPONENT;

   assert(piecesInColumn[column] > 0);

   row = piecesInColumn[column]-1;
   bit = 1 << column; rowBit = 1 << row;

#if 1
   return ((connectPossible[CONNECT3POSSIBLE][bvHorizontal[PLAYER1][row]] & bit) ||
       (connectPossible[CONNECT3POSSIBLE][bvVertical[PLAYER1][column]] & rowBit) ||
       (connectPossible[CONNECT3POSSIBLE][bvDiagonalTL[PLAYER1][TL_INDEX(row, column)]] & bit) ||
       (connectPossible[CONNECT3POSSIBLE][bvDiagonalBL[PLAYER1][BL_INDEX(row, column)]] & bit) ||

       (connectPossible[CONNECT3POSSIBLE][bvHorizontal[PLAYER2][row]] & bit) ||
       (connectPossible[CONNECT3POSSIBLE][bvVertical[PLAYER2][column]] & rowBit) ||
       (connectPossible[CONNECT3POSSIBLE][bvDiagonalTL[PLAYER2][TL_INDEX(row, column)]] & bit) ||
       (connectPossible[CONNECT3POSSIBLE][bvDiagonalBL[PLAYER2][BL_INDEX(row, column)]] & bit));
#else
   return ((table[bvHorizontal[playerOnMove][row]] & bit) ||
       (table[bvVertical[playerOnMove][column]] & rowBit) ||
       (table[bvDiagonalTL[playerOnMove][TL_INDEX(row, column)]] & bit) ||
       (table[bvDiagonalBL[playerOnMove][BL_INDEX(row, column)]] & bit) ||

       (table[bvHorizontal[opponent][row]] & bit) ||
       (table[bvVertical[opponent][column]] & rowBit) ||
       (table[bvDiagonalTL[opponent][TL_INDEX(row, column)]] & bit) ||
       (table[bvDiagonalBL[opponent][BL_INDEX(row, column)]] & bit));
#endif
}

#ifdef DEEPSORT
static char is2poss(char column)
{
char *table = connectPossible[0];
char row, bit, rowBit, opponent = OPPONENT;

   assert(piecesInColumn[column] > 0);

   row = piecesInColumn[column]-1;
   bit = 1 << column; rowBit = 1 << row;

   return ((table[bvHorizontal[playerOnMove][row]] & bit) ||
       (table[bvVertical[playerOnMove][column]] & rowBit) ||
       (table[bvDiagonalTL[playerOnMove][TL_INDEX(row, column)]] & bit) ||
       (table[bvDiagonalBL[playerOnMove][BL_INDEX(row, column)]] & bit));
}
#endif

#ifdef INCREMENTAL_EVAL
static int possibility(char column)
{
char *table = connectPossible[CONNECT4POSSIBLE],
   *bvH = bvHorizontal[playerOnMove],
   *bvV = bvVertical[playerOnMove],
   *bvTL = bvDiagonalTL[playerOnMove],
   *bvBL = bvDiagonalBL[playerOnMove];
char row, bit, rowBit, tl, bl;

   assert(piecesInColumn[column] > 0);

   row = piecesInColumn[column]-1;
   tl=TL_INDEX(row, column); bl = BL_INDEX(row, column);
   bit = 1 << column; rowBit = 1 << row;

   if ((table[bvH[row]] & bit) ||
       (table[bvV[column]] & rowBit) ||
       (table[bvTL[tl]] & bit) ||
       (table[bvBL[bl]] & bit))
      return VALUE4;

   table = connectPossible[CONNECT3POSSIBLE];
   if ((table[bvH[row]] & bit) ||
       (table[bvV[column]] & rowBit) ||
		 (table[bvTL[tl]] & bit) ||
       (table[bvBL[bl]] & bit))
      return VALUE3;

   table = connectPossible[CONNECT2POSSIBLE];
   if ((table[bvH[row]] & bit) ||
       (table[bvV[column]] & rowBit) ||
       (table[bvTL[tl]] & bit) ||
       (table[bvBL[bl]] & bit))
      return VALUE2;

   return 0;
}
#endif

#ifdef IN2OUT_ORDER
#define ORDER(column) (column % 2 == 0) ? (COLUMNS-1)/2-column/2 : (COLUMNS-1)/2+(column+1)/2
#else
#define ORDER(column) column
#endif

static int GenerateMoves(char moves[])
{
char w = 0, w3 = 0, w0 = 0, i, column;
char winner;
BYTE connect3[COLUMNS], connect[COLUMNS];
#ifdef DEEPSORT
   char w2 = 0;
   BYTE connect2[COLUMNS];
#endif
#ifdef MAINVAR
unsigned char mainVarCol = helpMainVar[depth];
#endif
      if (Is4Connect(&winner)) return 0;
#if 1
      #ifdef MAINVAR
         if ((mainVarCol != -1) && POSSIBLEMOVE(mainVarCol)) moves[w++] = mainVarCol;
      #endif
      for (i = 0; i < COLUMNS; i++)
      {
         column = ORDER(i);
         #ifdef MAINVAR
         if ((mainVarCol != column) && POSSIBLEMOVE(column))
         #else
         if (POSSIBLEMOVE(column))
         #endif
         {
            if (is4poss(column))
               moves[w++] = column;
            else if (is3poss(column))
               connect3[w3++] = column;
            #ifdef DEEPSORT
            else if (is2poss(column))
               connect2[w2++] = column;
            #endif
            else
               connect[w0++] = column;
         }
      }

      i = 0;
      while (w3) { moves[w++] = connect3[i++]; w3--; }
      #ifdef DEEPSORT
         i = 0;
         while (w2) { moves[w++] = connect2[i++]; w2--; }
      #endif
      i = 0;
      while (w0) { moves[w++] = connect[i++]; w0--; }
#else
      for (i = 0; i < COLUMNS; i++)
      {
         column = ORDER(i);
         if (POSSIBLEMOVE(column)) moves[w++] = column;
      }
#endif
      return w;
}

#ifdef INCREMENTAL_EVAL
   static int Evaluate(int depth, int points)
#else
   int Evaluate(void)
#endif
{
int score;
char winner;
      #ifdef STATISTICS
         statistics.evaluates ++;
      #endif

      if (Is4Connect(&winner))
         score =  (winner == playerOnMove) ? WIN-depth : -(WIN-depth);
      #if 1
      else
      #ifndef COUNTEXACT
      {
      char *connect = connectPossible[CONNECT4POSSIBLE];
      char i, opponent = OPPONENT;
      BYTE *andMask;
   BYTE *bvHp, *bvHo,
         *bvVp, *bvVo,
         *bvDTLp, *bvDTLo,
         *bvDBLp, *bvDBLo;
      #ifdef COUNT4CONNECT
            int count = 0;
         #endif
         score = 0;

         bvVp = &bvVertical[playerOnMove][0]; bvVo = &bvVertical[opponent][0];
         for (i = 0; i < COLUMNS; i++, bvVp++, bvVo++)
         {
            if (connect[*bvVp] & ~*bvVo & V_ANDMASK)
               score += VALUE4;

            if (connect[*bvVo] & ~*bvVp & V_ANDMASK)
               score -= VALUE4;
         }

         bvHp = &bvHorizontal[playerOnMove][0]; bvHo = &bvHorizontal[opponent][0];
         for (i = 0; i < ROWS; i++, bvHp++, bvHo++)
         {
         #ifndef COUNT4CONNECT
            if (connect[*bvHp] & ~*bvHo)
               score += VALUE4;

            if (connect[*bvHo] & ~*bvHp)
               score -= VALUE4;
         #else
            count += numberOfBits[connect[*bvHp] & ~*bvHo]
                  -  numberOfBits[connect[*bvHo] & ~*bvHp];
         #endif
         }

         bvDTLp = &bvDiagonalTL[playerOnMove][3]; bvDTLo = &bvDiagonalTL[opponent][3];
         bvDBLp = &bvDiagonalBL[playerOnMove][3]; bvDBLo = &bvDiagonalBL[opponent][3];
         andMask = &bvDiagonalAndMask[3];
         for (i = 3; i < 9; i++, bvDTLp++, bvDTLo++, bvDBLp++, bvDBLo++, andMask++)
         {
         #ifndef COUNT4CONNECT
            if (connect[*bvDTLp] & ~*bvDTLo & *andMask)
               score += VALUE4;

            if (connect[*bvDTLo] & ~*bvDTLp & *andMask)
               score -= VALUE4;

            if (connect[*bvDBLp] & ~*bvDBLo & *andMask)
               score += VALUE4;

            if (connect[*bvDBLo] & ~*bvDBLp & *andMask)
               score -= VALUE4;
         #else
            count += numberOfBits[connect[*bvDTLp] & ~*bvDTLo & *andMask]
                 +  numberOfBits[connect[*bvDBLp] & ~*bvDBLo & *andMask]
                  -  numberOfBits[connect[*bvDTLo] & ~*bvDTLp & *andMask]
                  -  numberOfBits[connect[*bvDBLo] & ~*bvDBLp & *andMask];
         #endif
         }
         #ifdef COUNT4CONNECT
         score += VALUE4 * count;
         #endif
      }
   #else // COUNTEXACT
   {
   //#define COUNT3POSSIBLE 
   int row, column, count=0;
   //BYTE possP[COLUMNS], possO[COLUMNS];
   BYTE possP, possO;
   BYTE *bvHp = bvHorizontal[playerOnMove],
       *bvHo = bvHorizontal[OPPONENT],
       *bvVp = bvVertical[playerOnMove],
       *bvVo = bvVertical[OPPONENT],
       *bvDTLp = bvDiagonalTL[playerOnMove], *bvDTLo = bvDiagonalTL[OPPONENT],
       *bvDBLp = bvDiagonalBL[playerOnMove], *bvDBLo = bvDiagonalBL[OPPONENT];
       //*_possP = possP, *_possO = possO;
      //BYTE *connect = connectPossible[CONNECT4POSSIBLE];
	  BYTE bit, vertBit, tlIndex, blIndex;
	  #ifdef COUNT3POSSIBLE
	  int count3 = 0;
	  BYTE poss3P, poss3O;
	  #endif
      for (column = 0; column < COLUMNS; column++,
               bvVp++, bvVo++/*, _possP++, _possO++*/)
      {
         /*
         *_possP = connectPossible[CONNECT4POSSIBLE][*bvVp] & ~*bvVo & V_ANDMASK;
         *_possO = connectPossible[CONNECT4POSSIBLE][*bvVo] & ~*bvVp & V_ANDMASK;
         */
         possP = connectPossible[CONNECT4POSSIBLE][*bvVp] & ~*bvVo & V_ANDMASK;
         possO = connectPossible[CONNECT4POSSIBLE][*bvVo] & ~*bvVp & V_ANDMASK;
         #ifdef COUNT3POSSIBLE
         poss3P = connectPossible[CONNECT3POSSIBLE][*bvVp] & ~*bvVo & V_ANDMASK;
         poss3O = connectPossible[CONNECT3POSSIBLE][*bvVo] & ~*bvVp & V_ANDMASK;
         #endif
         bit = 1 << column;
         for (row = piecesInColumn[column]-1; row >= 0; row--)
         {
               vertBit = 1 << row;
               tlIndex = TL_INDEX(row, column); blIndex = BL_INDEX(row, column);
               if ((connectPossible[CONNECT4POSSIBLE][bvHp[row]] & bit) ||
                   (connectPossible[CONNECT4POSSIBLE][bvDTLp[tlIndex]] & bit) ||
                   (connectPossible[CONNECT4POSSIBLE][bvDBLp[blIndex]] & bit))
                  //*_possP |= vertBit;
                  possP |= vertBit;
               if ((connectPossible[CONNECT4POSSIBLE][bvHo[row]] & bit) ||
                   (connectPossible[CONNECT4POSSIBLE][bvDTLo[tlIndex]] & bit) ||
                   (connectPossible[CONNECT4POSSIBLE][bvDBLo[blIndex]] & bit))
                  //*_possO |= vertBit;
                  possO |= vertBit;
            #ifdef COUNT3POSSIBLE
               if ((connectPossible[CONNECT3POSSIBLE][bvHp[row]] & bit) ||
                   (connectPossible[CONNECT3POSSIBLE][bvDTLp[tlIndex]] & bit) ||
                   (connectPossible[CONNECT3POSSIBLE][bvDBLp[blIndex]] & bit))
                  //*_possP |= vertBit;
                  poss3P |= vertBit;
               if ((connectPossible[CONNECT3POSSIBLE][bvHo[row]] & bit) ||
                   (connectPossible[CONNECT3POSSIBLE][bvDTLo[tlIndex]] & bit) ||
                   (connectPossible[CONNECT3POSSIBLE][bvDBLo[blIndex]] & bit))
                  //*_possO |= vertBit;
                  poss3O |= vertBit;
			#endif
         }
         //count += numberOfBits[*_possP] - numberOfBits[*_possO];
         #if 1 
         	count += rowScore[possP | (possO << ROWS)];
         #else
         if (possP > possO)     
         	count += 4*numberOfBits[possP]-numberOfBits[possO];
         else if (possP < possO)
            count += numberOfBits[possP] - 4*numberOfBits[possO];
         #endif
         
         #ifdef COUNT3POSSIBLE
         if (poss3P > poss3O)
         	count3 += 4*numberOfBits[poss3P] - numberOfBits[poss3O];
         else if (poss3P < poss3O)                                         
         	count3 += numberOfBits[poss3P] - 4*numberOfBits[poss3O];         	
         #endif
      }
      #ifdef COUNT3POSSIBLE
      score = VALUE4/4*count+VALUE3/4*count3;
      #else 
      score = count; //VALUE4/4*count;
      #endif
   }
   #endif
      #endif

   #if 0
      else
      while (top < moveListTop)
         {
            score += (COLUMNS-1)/2 - abs((COLUMNS-1)/2 - moveList[top].column);
            top ++;
         }
      #endif

#ifdef INCREMENTAL_EVAL
   return score+_score;
#else
   #ifdef BONUS
   return (playerOnMove == PLAYER1) ? score + bonusScore : score - bonusScore;
   #else
   return score;
   #endif
#endif
}

#ifdef MAINVAR
static void CopyMainVar(char pos)
{
int i = 0;
   mainVar[depth][depth] = pos;
   do
   {
      i ++;
      mainVar[depth][depth+i] = mainVar[depth+1][depth+i];
   } while (mainVar[depth+1][depth+i] != -1);
}
#endif

#ifdef INCREMENTAL_EVAL
static int AlphaBeta(int a, int b, int distance, int points)
#else
static int AlphaBeta(int a, int b, int distance)
#endif
{
int score, t;
#ifdef INCREMENTAL_EVAL
int poss;
#endif
char w, m, i;
char moves[COLUMNS];
	#ifdef MAINVAR
    	mainVar[depth][depth] = -1;
    #endif

    #ifdef HASHTABLE
    {
       	if ((depth >= MIN_DEPTH) && (depth <= MAX_DEPTH) && HTFind(&score, distance, &a, &b)) 
       		return score;
		else 
			score = -INF;
    }
    #else
    	score = -INF;
    #endif

    if ((distance == 0) || (movesLeft == 0) || ((w = GenerateMoves(moves)) == 0))
    #ifdef INCREMENTAL_EVAL                                             
    	return Evaluate(depth, points);
    #else
        return Evaluate();
    #endif

    #ifdef MULTITASK
    for (i = 0; !stopCalculation && (i < w); i++)
    #else
    for (i = 0; i < w; i++)
    #endif
    {
	      /* Little code segment to allow cooperative multitasking */
		#if !__OS2__
		#ifdef MULTITASK
		{
        MSG msg;
        extern HANDLE hAccel;
        	if (!stopCalculation && PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        	{
	            if ( !TranslateAccelerator (hwndMainFrame, hAccel, &msg) ) 
            	{
               		TranslateMessage(&msg);
               		DispatchMessage(&msg);
            	}
         	}
      	}
	    #endif
		#endif

        m = moves[i];
        #ifdef INCREMENTAL_EVAL
        	poss = possibility(m);
            _MakeMove(m);
            t = -AlphaBeta(-b, -a, distance -1, -points-poss);
        #else
            _MakeMove(m);
			t = -AlphaBeta(-b, -a, distance -1);
        #endif
        _UnmakeMove();

	    #ifdef HASHTABLE
		{
         	if ((depth >= MIN_DEPTH) && (depth <= MAX_DEPTH)) HTInsert(score, distance, a, b);
      	}
      	#endif

        if (t > score)
        {
            score = t;

            if (score >= b)
            {
            #ifdef MAINVAR
				CopyMainVar(m);
            #endif
            return score;
            }

            if (score > a)
            {
				a = score;
                #ifdef MAINVAR
					CopyMainVar(m);
                #endif
            }
		}
	}
	return score;
}

#if 1

char BestMove(int searchDepth, int *score)
{
int t, a, b, distance, bestValue;
#ifdef INCREMENTAL_EVAL
int points;
#endif
char moves[COLUMNS], w, m, i, j, h;
int values[COLUMNS], v;
   	#ifdef HASHTABLE
    	ClearHashtable();
   	#endif
   	#ifdef MAINVAR
      	for (i = 0; i < MAX_SEARCH_DEPTH; i++) helpMainVar[i] = -1;
      	mainVar[depth][depth] = -1;
   	#endif
    #ifdef STATSTICS
    	statstics.evaluates = 0;
    #endif
    w = GenerateMoves(moves);
    if (w == 0) return -1;

    //movesPlayedTop = moveListTop+1;
    depth = 0;
    #ifdef MULTITASK
    	for (distance = 1; !stopCalculation && (distance <= searchDepth); distance ++)
    #else
    	for (distance = 1; distance <= searchDepth; distance ++)
    #endif
    {
        a = -INF; b = INF; bestValue = -INF;

        _DbgPrint("\nDistance %d ", distance);
        // maxSearchDepth = distance + EXTENDED_SEARCH_INCREASE;
        // pos = moves[0];
        #ifdef MULTITASK
        for (i = 0; !stopCalculation && (i < w); i++)
        #else
        for (i = 0; i < w; i++)
        #endif
        {
        	m = moves[i];

            //mainVar[depth][depth] = -1;
            // display move[m]
            #ifdef INCREMENTAL_EVAL
               	points = possibility(m);
               	_MakeMove(m);
               	//printf("%d->", m+1); fflush(NULL);
               	t = -AlphaBeta(-b, -a, distance-1, -points);
            #else
               	_MakeMove(m);
               	_DbgPrint("%d->", m+1);
               	t = -AlphaBeta(-b, -a, distance-1);
            #endif
            _UnmakeMove();
            _DbgPrint("%d, ", t);

			#ifdef MULTITASK
			if (!stopCalculation)
			#endif
			{
				values[i] = t;
				if (t > bestValue) { 
					bestValue = t;
					if (a < t) a = t;
				}
				// bubble sort
				for (j = i; (i > 0) && (values[i-1] < values[i]); i--) {
					v = values[i-1]; values[i-1] = values[i]; values[i] = v;
					h = moves[i-1]; moves[i-1] = moves[i]; moves[i] = h;
				}
				
               	#ifdef MAINVAR
               	CopyMainVar(m);
               	#endif
			}
/*
			#ifdef MULTITASK
            if ((t > bestValue) && (!stopCalculation))
            #else
            if (t > bestValue)
            #endif
            {
               	bestValue = t;
               	if (a < t) a = t;
               	for (j = i; j != 0; j--)
               	{
                	moves[j] = moves[j-1];
               	}
               	moves[0] = m;

               	#ifdef MAINVAR
               	CopyMainVar(m);
               	#endif
            }
*/
/*            if (i == 0)
            {
               bestValue = t;
               #ifdef MAINVAR
               CopyMainVar(m);
               #endif
            }
            else if (t > bestValue)
            {
               bestValue = t;
               #ifdef MAINVAR
               CopyMainVar(m);
               #endif
               for (j = i; j != 0; j--)
               {
                     moves[j] = moves[j-1];
               }
               moves[0] = m;
            }
            if (score > a)
            {
               a = score;
               #ifdef MAINVAR
                  CopyMainVar(m);
               #endif
            }
*/
        }
        #ifdef MAINVAR
     	for (i = 0; (i < MAX_SEARCH_DEPTH) && (mainVar[0][i] != -1); i++)
         	helpMainVar[i] = mainVar[0][i];
        #endif
	}
    _DbgPrint("\n", NULL);

    *score = bestValue;
    return moves[0];
}

#else

char BestMove(int searchDepth, int *score)
{
int t, a, b, distance;
#ifdef INCREMENTAL_EVAL
int points;
#endif
char moves[COLUMNS], w, m, i, j;
char winner;
   #ifdef HASHTABLE
   ClearHashtable();
   #endif

      #ifdef STATSTICS
         statstics.evaluates = 0;
      #endif
      w = GenerateMoves(moves);
      if (w == 0) return -1;

      movesPlayedTop = moveListTop+1;

      depth = 0;

      for (distance = 1; distance <= searchDepth; distance ++)
      {
         if (distance == 1)
         {
            a = -INF; b = INF;
         }
         else
         {
            a = -INF; b = INF;
         }

         _DbgPrint("\nDistance %d ", distance);
         // maxSearchDepth = distance + EXTENDED_SEARCH_INCREASE;
         // pos = moves[0];
         for (i = 0; i < w; i++)
         {
            m = moves[i];

            //mainVar[depth][depth] = -1;
            // display move[m]
            #ifdef INCREMENTAL_EVAL
               points = possibility(m);
               _MakeMove(m);
               //printf("%d->", m+1); fflush(NULL);
               t = -AlphaBeta(-b, -a, distance-1, -points);
            #else
               _MakeMove(m);
               _DbgPrint("%d->", m+1);
               t = -AlphaBeta(-b, -a, distance-1);
            #endif
            _UnmakeMove();
         	_DbgPrint("%d, ", t);

            if (i == 0)
            {
               if (t < a)
               {
                     a = -INF;
                     b = t;
                     // display
                     _MakeMove(m);
                     t = -AlphaBeta(-b, -a, distance-1);
                     _UnmakeMove();
               }
               else if (t >= b)
               {
                     a = t;
                     b = INF;
                     // display
                     _MakeMove(m);
                     t = -AlphaBeta(-b, -a, distance-1);
                     _UnmakeMove();
               }
              a = t;
               b = a+1;
               // display
               // CopyMainVar(m);
               // display mainVar
            }
            else
            {
               if (t > a)
               {
               int bestValue = a;
                     a = t;
                     b = INF;
                     _MakeMove(m);
                     t = -AlphaBeta(-b, -a, distance-1);
                     _UnmakeMove();

                     if (t > bestValue)
                     {
                        a = t;
                        b = a+1;
                        // display move
                        // CopyMainVar(m);
                        // display mainVar
                        for (j = i; j != 0; j--)
                        {
                           moves[j] = moves[j-1];
                        }
                        moves[0] = m;
                        _DbgPrint("%d->", m+1);
                        _DbgPrint("%d, ", t);
                     }
               }
            }
         }
      }
      _DbgPrint("\n", NULL);
      *score = a;
      return moves[0];
}

#endif

char PlayerMakeMove(char column)
{
      if (POSSIBLEMOVE(column))
      {
         _MakeMove(column);
         depth = 0;
		    #if defined(_WINDOWS) | defined(BEOS)
				CopyGameField();	
			#endif
         return TRUE;
      }
      else
         return FALSE;
}

// undoes one or two moves and returns success
char Undo(int times)
{
      if ((1 <= times) && (times <= 2))
      {
         if (movesLeft < COLUMNS*ROWS)
         {
            if (times == 1)
            {
               _UnmakeMove();
               depth = 0;
			    #if defined(_WINDOWS) | defined(BEOS)
				CopyGameField();	
				#endif
               return TRUE;
            }
            else
            {
               if (movesLeft < COLUMNS*ROWS-1)
               {
                     _UnmakeMove(); _UnmakeMove();
	                 depth = 0;
				    #if defined(_WINDOWS) | defined(BEOS)
						CopyGameField();	
					#endif
                     return TRUE;
               }
            }
         }
      }
      return FALSE;
}

#ifndef BONUS
  	#ifndef HASHTABLE
   		#define ID 1
    #else
    	#define ID 2
    #endif
#else
   	#ifndef HASHTABLE
    	#define ID 3
   	#else
    	#define ID 4
   	#endif
#endif

int Save4Connect(FILE *file)
{
int id;
	id = ID;
	if (1 != fwrite(&id, sizeof(1), 1, file)) return FALSE;
	fwrite(moveList, sizeof(moveList), 1, file);
	fwrite(&moveListTop, sizeof(moveListTop), 1, file);
	fwrite(&movesLeft, sizeof(movesLeft), 1, file);
	fwrite(&playerOnMove, sizeof(playerOnMove), 1, file);
	fwrite(gameField, sizeof(gameField), 1, file);
	fwrite(piecesInColumn, sizeof(piecesInColumn), 1, file);
	fwrite(bvHorizontal, sizeof(bvHorizontal), 1, file);
	fwrite(bvVertical, sizeof(bvVertical), 1, file);
	fwrite(bvDiagonalBL, sizeof(bvDiagonalBL), 1, file);
	fwrite(bvDiagonalTL, sizeof(bvDiagonalTL), 1, file);

	#ifdef BONUS
	fwrite(&bonusScore, sizeof(bonusScore), 1, file);
	#endif
	
	#ifdef HASHTABLE
	fwrite(&hc, sizeof(hc), 1, file);
	fwrite(&hcIndex, sizeof(hcIndex), 1, file);
	#endif	
	
	return TRUE;
}

int Restore4Connect(FILE *file)
{
int id;
	id = ID;
	if ((1 != fread(&id, sizeof(1), 1, file)) || (id != ID)) return FALSE;
	fread(moveList, sizeof(moveList), 1, file);
	fread(&moveListTop, sizeof(moveListTop), 1, file);
	fread(&movesLeft, sizeof(movesLeft), 1, file);
	fread(&playerOnMove, sizeof(playerOnMove), 1, file);
	fread(gameField, sizeof(gameField), 1, file);
	fread(piecesInColumn, sizeof(piecesInColumn), 1, file);
	fread(bvHorizontal, sizeof(bvHorizontal), 1, file);
	fread(bvVertical, sizeof(bvVertical), 1, file);
	fread(bvDiagonalBL, sizeof(bvDiagonalBL), 1, file);
	fread(bvDiagonalTL, sizeof(bvDiagonalTL), 1, file);

	#ifdef BONUS
	fread(&bonusScore, sizeof(bonusScore), 1, file);
	#endif
	
	#ifdef HASHTABLE
	fread(&hc, sizeof(hc), 1, file);
	fread(&hcIndex, sizeof(hcIndex), 1, file);
	#endif	
    #if defined(_WINDOWS) | defined(BEOS)
		CopyGameField();	
	#endif
	return TRUE;
}
