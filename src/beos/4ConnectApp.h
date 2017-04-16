#ifndef CONNECT_APP_H
#define CONNECT_APP_H

#include <Application.h> 
#include <Window.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Alert.h>
#include <Message.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SimpleGameSound.h>
#include <MessageRunner.h>
#include "FieldView.h"
#include "StatusView.h"


#define MENU_GAME_NEW        1
#define MENU_GAME_FORCE_MOVE 2
#define MENU_GAME_UNDO_MOVE  3
#define MENU_GAME_ABOUT	     4
#define MENU_GAME_QUIT		 5
#define MENU_GAME_SOUND		 6

#define MENU_PLAYERS_HH 10
#define MENU_PLAYERS_HC 11
#define MENU_PLAYERS_CH 12
#define MENU_PLAYERS_CC 13

#define MENU_PLAYERS_P1 14
#define MENU_PLAYERS_P2 15

#define MENU_LEVEL_MIN 100
#define MENU_LEVEL_MAX 130

#define BEST_MOVE_MSG  140
#define BEST_MOVE_COLUMN "column"

#define MENU_HELP       150
#define HELP_TERMINATED_MSG 151

#define TIMER_MSG       152

#define HUMANHUMAN       0
#define HUMANCOMPUTER    1
#define COMPUTERHUMAN    2
#define COMPUTERCOMPUTER 3

class ConnectAppWindow : public BWindow {
public:
	int level;
	bool playing, calculating, sound;
	int playerStarts;
	int playerMode;
	
	BMenuItem *playerModeItems[4], *playerStartsItems[2], *help;
	BMenuBar *menubar;
	FieldView *fieldview;
	StatusView *statusview;

	BPath appPath;
	BSimpleGameSound *pingSnd, *gameOverSnd;

	BMessageRunner *timer;
	
	ConnectAppWindow(BRect);
	bool QuitRequested();	
	void MessageReceived(BMessage *message);

	void MarkMenuItem(BMenuItem *items[], int &oldIndex, int newIndex); 
	void DoMove(char column);
	void BestMove();
	bool IsHumanOnMove();
	bool IsGameOver();
	void NextPlayer();
	void EnableMenuItems(bool enabled);
	void LoadSettings();
	void SaveSettings();
	void SetDefaultValues();
	void UpdateCursor();
	
	void StartTimer();
	void StopTimer();
};

class ConnectApp : public BApplication {
public:
	ConnectAppWindow *window;
	ConnectApp();
};

extern unsigned char wait_cursor[], player1cursor[], player2cursor[], stop_cursor[];

#define my_app ((ConnectApp*)be_app)
#endif