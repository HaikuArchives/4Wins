#include "4ConnectApp.h"

unsigned char wait_cursor[] = 
{16, // width & hight
1, // color depth
8, 8, // hotspot coordinates
// color data (32 bytes)
0x07, 0xE0, 0x18, 0x18, 0x20, 0x04, 0x70, 0x0E,
0x78, 0x1E, 0xFC, 0x3F, 0xFE, 0x7F, 0xFF, 0xFF,
0xFF, 0xFF, 0xFE, 0x7F, 0xFC, 0x3F, 0x78, 0x1E,
0x70, 0x0E, 0x20, 0x04, 0x18, 0x18, 0x07, 0xE0,
// transparency bitmask (32 bytes)
0x07, 0xE0,0x1F, 0xF8,0x3F, 0xFC,0x7F, 0xFE,
0x7F, 0xFE,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,
0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0x7F, 0xFE,
0x7F, 0xFE,0x3F, 0xFC,0x1F, 0xF8,0x07, 0xE0
};        

unsigned char player2cursor[] =
{16, // width & hight
1, // color depth
8, 8, // hotspot coordinates
// color data (32 bytes)
0x07, 0xE0, 0x1F, 0xF8, 0x3F, 0xFC, 0x7F, 0xFE,
0x7F, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFE,
0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8, 0x07, 0xE0,
// transparency bitmask (32 bytes)
0x07, 0xE0,0x1F, 0xF8,0x3F, 0xFC,0x7F, 0xFE,
0x7F, 0xFE,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,
0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0x7F, 0xFE,
0x7F, 0xFE,0x3F, 0xFC,0x1F, 0xF8,0x07, 0xE0
};    

unsigned char player1cursor[] = 
{16, // width & hight
1, // color depth
8, 8, // hotspot coordinates
// color data (32 bytes)
0x07, 0xE0, 0x1A, 0xB8, 0x35, 0x54, 0x6A, 0xAA,
0x55, 0x56, 0xAA, 0xAB, 0xD5, 0x55, 0xAA, 0xAB,
0xD5, 0x55, 0xAA, 0xAB, 0xD5, 0x55, 0xAA, 0xAB,
0x55, 0x56, 0x2A, 0xAC, 0x1D, 0x78, 0x07, 0xE0,
// transparency bitmask (32 bytes)
0x07, 0xE0,0x1F, 0xF8,0x3F, 0xFC,0x7F, 0xFE,
0x7F, 0xFE,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,
0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,
0x7F, 0xFE,0x3F, 0xFC,0x1F, 0xF8,0x07, 0xE0
};  

unsigned char stop_cursor[] = 
{16, // width & hight
1, // color depth
8, 8, // hotspot coordinates
// color data (32 bytes)
0x47, 0xE2, 0xF8, 0x1F, 0x70, 0x0E, 0x78, 0x1E,
0x5C, 0x3A, 0x8E, 0x71, 0x87, 0xE1, 0x83, 0xC1,
0x83, 0xC1, 0x87, 0xE1, 0x8E, 0x71, 0x5C, 0x3A,
0x78, 0x1E, 0x70, 0x0E, 0xF8, 0x1F, 0x47, 0xE2,
// transparency bitmask (32 bytes)
0x47, 0xE2,0xFF, 0xFF,0x7F, 0xFE,0x7F, 0xFE,
0x7F, 0xFE,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,
0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0x7F, 0xFE,
0x7F, 0xFE,0x7F, 0xFE,0xFF, 0xFF,0x47, 0xE2
};     

static char statusText[2][17] = {"Player one.", "Player two."},
			playerWinsText[2][17] = {"Player one wins.", "Player two wins."};

static sem_id sem;
static thread_id calculate_move_tid;
static int depth;

int32 calculate_move_thread(void *data) {
int score;
char column;
BMessage *message;
	do {
		acquire_sem(sem); 
		column = BestMove(depth, &score);
		message = new BMessage(BEST_MOVE_MSG);
		message->AddInt8(BEST_MOVE_COLUMN, (int) column);
		my_app->window->PostMessage(message);
	} while(true);
}

int32 call_help_thread(void *data) {
thread_id help;
BPath path = my_app->window->appPath;
	path.Append("help/index.html");
char *argv[3] = {"/beos/apps/NetPositive",path.Path(), NULL};
int32 argc = 2;
status_t status;
	help = load_image(argc, argv, environ);
	wait_for_thread(help, &status);
	my_app->window->PostMessage(new BMessage(HELP_TERMINATED_MSG));
}

void ConnectAppWindow::BestMove() {
	//be_app->SetCursor(wait_cursor);
	statusview->SetText("Searching best move...");
	EnableMenuItems(false);
	depth = level;
	release_sem(sem);
	// start calculation of the best move
	calculating = true;
	UpdateCursor();
}

void ConnectAppWindow::DoMove(char column) {
char winner;
	PlayerMakeMove(column);
	fieldview->DrawBuffer();
	UpdateCursor();
	if (::Is4Connect(&winner)) { 
		if (sound) gameOverSnd->StartPlaying();
		statusview->SetText(playerWinsText[!playerOnMove]);
		fieldview->GameOver(moveList[moveListTop].column, moveList[moveListTop].row);
		playing = false; StartTimer();
	} else if (movesLeft == 0) {
		if (sound) gameOverSnd->StartPlaying();
		playing = false;
		statusview->SetText("The game is drawn.");
	} else 
		NextPlayer();
}

void ConnectAppWindow::UpdateCursor() {
BPoint point;
uint32 buttons;
BRect rect = fieldview->Bounds();
	fieldview->GetMouse(&point, &buttons);
	fieldview->UpdateCursor(rect.Contains(point), point.x);
}

void ConnectAppWindow::NextPlayer() {
	switch(playerMode) {
	case HUMANHUMAN:
		statusview->SetText(statusText[playerOnMove]);
		break;
	case HUMANCOMPUTER:
		if (playerOnMove == PLAYER2) BestMove();
		else statusview->SetText(statusText[playerOnMove]);
		break;
	case COMPUTERHUMAN:
		if (playerOnMove == PLAYER1) BestMove();
		else statusview->SetText(statusText[playerOnMove]);
		break;
	case COMPUTERCOMPUTER:
		BestMove();
		break;
	}
}

ConnectAppWindow::ConnectAppWindow(BRect aRect) 
	: BWindow(aRect, "4 Wins", B_TITLED_WINDOW, B_OUTLINE_RESIZE) {
bool sound_ok;
	// initialisation
	timer = NULL;
	playing = true; calculating = false;
	LoadSettings();
	// load sounds
	BPath path, path2;
	BEntry entry;
	app_info info;
	if (B_OK == be_app->GetAppInfo(&info)) {
		entry = BEntry(&info.ref);
		entry.GetPath(&path);
		path.GetParent(&path);
	} else
		path = BPath(".");
	appPath = path;

	path2 = path; path2.Append("sounds/ComputerMove");
	entry = BEntry(path2.Path());
	entry_ref er;
	entry.GetRef(&er);
	pingSnd = new BSimpleGameSound(&er);
	sound_ok = (pingSnd->InitCheck() == B_OK);
	if (sound_ok) {
		path.Append("sounds/GameOver");
		entry = BEntry(path.Path());
		entry.GetRef(&er);
		gameOverSnd = new BSimpleGameSound(&er);
		if (gameOverSnd->InitCheck() != B_OK) {
			delete pingSnd; pingSnd = NULL;
			sound_ok = false; sound = false;
		}
	} else sound = false;
	// add menu bar
	BRect rect = BRect(0,0,aRect.Width(), aRect.Height());
	menubar = new BMenuBar(rect, "menu_bar");
	BMenu *menu; 
	BMenuItem *item; 

	menu = new BMenu("Game");
	menu->AddItem(new BMenuItem("New", new BMessage(MENU_GAME_NEW), 'N'));
	menu->AddItem(new BMenuItem("Force Move", new BMessage(MENU_GAME_FORCE_MOVE), 'F'));
	menu->AddItem(new BMenuItem("Undo Move", new BMessage(MENU_GAME_UNDO_MOVE), 'U'));
	menu->AddItem(item = new BMenuItem("Sound", new BMessage(MENU_GAME_SOUND), 'S'));
	item->SetEnabled(sound_ok);
	item->SetMarked(sound);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("About 4 Wins...", new BMessage(MENU_GAME_ABOUT)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(MENU_GAME_QUIT), 'Q')); 
	menubar->AddItem(menu);

	menu = new BMenu("Players");
	menu->AddItem(playerModeItems[0] = new BMenuItem("Human vs. Human", new BMessage(MENU_PLAYERS_HH)));
	menu->AddItem(playerModeItems[1]  = new BMenuItem("Human vs. Computer", new BMessage(MENU_PLAYERS_HC)));
	menu->AddItem(playerModeItems[2] = new BMenuItem("Computer vs. Human", new BMessage(MENU_PLAYERS_CH)));
	menu->AddItem(playerModeItems[3] = new BMenuItem("Computer vs. Computer", new BMessage(MENU_PLAYERS_CC)));
	playerModeItems[playerMode]->SetMarked(true);
	menu->AddSeparatorItem();
	menu->AddItem(playerStartsItems[0] = new BMenuItem("Player One Starts", new BMessage(MENU_PLAYERS_P1)));
	menu->AddItem(playerStartsItems[1] = new BMenuItem("Player Two Starts", new BMessage(MENU_PLAYERS_P2)));
	playerStartsItems[playerStarts]->SetMarked(true);
	menubar->AddItem(menu);

	menu = new BMenu("Level");
	menu->SetRadioMode(true);
	for (int i = 1; i <= 20; i++) {
		char level[3];
		sprintf(level,"%d", i);
		menu->AddItem(item = new BMenuItem(level, new BMessage(MENU_LEVEL_MIN+i-1)));
		if (i == this->level) item->SetMarked(true);
	}
	menubar->AddItem(menu);
	menubar->AddItem(help = new BMenuItem("Help", new BMessage(MENU_HELP)));

	AddChild(menubar);
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	// add status view
	rect.top = rect.bottom - (finfo.ascent+finfo.descent+finfo.leading);
	AddChild(statusview = new StatusView(rect, "Statusview", statusText[playerStarts]));
	// add field view
	aRect.Set(0, menubar->Bounds().Height()+1, aRect.Width(), aRect.Height()-statusview->Bounds().Height()-1);
	fieldview = NULL;
	AddChild(fieldview = new FieldView(aRect, appPath));
	// make window visible
	Show();
}

void ConnectAppWindow::EnableMenuItems(bool enabled) {
	menubar->FindItem(MENU_GAME_NEW)->SetEnabled(enabled);
	menubar->FindItem(MENU_GAME_FORCE_MOVE)->SetEnabled(enabled);
	menubar->FindItem(MENU_GAME_UNDO_MOVE)->SetEnabled(enabled);
}

void ConnectAppWindow::MarkMenuItem(BMenuItem *items[], int &oldIndex, int newIndex) {
	if (oldIndex != newIndex) {
		items[oldIndex]->SetMarked(false);
		items[newIndex]->SetMarked(true);
		oldIndex = newIndex;
	}
}


void ConnectAppWindow::MessageReceived(BMessage *message) {
	switch(message->what) {
	case MENU_GAME_NEW: 
		playing = true; StopTimer();
		fieldview->ClearField(playerStarts); fieldview->DrawBuffer();
		NextPlayer();
		break; 
	case MENU_GAME_UNDO_MOVE: {
		int undo;
		if (calculating) break;
		if (playing) {
			if (playerMode == HUMANHUMAN) 
				undo = 1;
			else if ((playerMode == HUMANCOMPUTER) || (playerMode == COMPUTERHUMAN)) 
				undo = 2; 
			else 
				undo = 0;
		} else {
			if (playerMode == HUMANHUMAN) undo = 1;
			else if (playerMode == COMPUTERCOMPUTER) undo = 0;
			else if (playerMode == HUMANCOMPUTER) 
				undo = (playerOnMove == PLAYER1) ? 2 : 1;
			else 
				undo = (playerOnMove == PLAYER2) ? 2 : 1;
		}
		if (Undo(undo)) {
			StopTimer();
			playing = true; fieldview->RestartGame();
			fieldview->DrawBuffer();
			NextPlayer();
		}
		break; }
	case MENU_PLAYERS_HH:
	case MENU_PLAYERS_HC:
	case MENU_PLAYERS_CH:
	case MENU_PLAYERS_CC:
		MarkMenuItem(playerModeItems, playerMode, message->what-MENU_PLAYERS_HH);
		if (IsHumanOnMove()) NextPlayer();		
		break;
	case MENU_PLAYERS_P1:
	case MENU_PLAYERS_P2:
		MarkMenuItem(playerStartsItems, playerStarts, message->what-MENU_PLAYERS_P1);		
		if (IsHumanOnMove() && (movesLeft == COLUMNS*ROWS)) {
			playerOnMove = playerStarts;
			NextPlayer();
		}
		break;
	case MENU_GAME_FORCE_MOVE: {
		if (playing) BestMove();
		break; }
	case MENU_GAME_ABOUT: {
		BAlert *about = new BAlert("About 4 Wins", 
			"4 Wins 1.1.\n"
			"This game is freeware.\n\n"
			"AI written 1997.\n"
			"GUI ported to BeOS 1999.\n\n"
			"By Michael Pfeiffer.\n\n"
			"EMail: m.pfeiffer@jk.uni-linz.ac.at [expires 1.7.2000].","Close");
		about->Go();
		break; }
	case MENU_HELP: {
		help->SetEnabled(false);
		thread_id tid = spawn_thread(call_help_thread, 
									 "call_help_thread", 
									 B_NORMAL_PRIORITY, NULL);	
		resume_thread(tid); }		
		break; 
	case HELP_TERMINATED_MSG:
		help->SetEnabled(true);
		break;
	case MENU_GAME_QUIT:
		QuitRequested();
		break;
	case BEST_MOVE_MSG: {
		if (sound) pingSnd->StartPlaying();
		int8 column;
		calculating = false;
		message->FindInt8(BEST_MOVE_COLUMN, 0, &column);
		//be_app->SetCursor(B_HAND_CURSOR);
		EnableMenuItems(true);
		DoMove((char) column); }
		break; 
	case MENU_GAME_SOUND:
		sound = !sound;
		menubar->FindItem(MENU_GAME_SOUND)->SetMarked(sound);
		break;
	case TIMER_MSG:
		fieldview->AnimationNextFrame();
		break;
	default:
		if ((message->what >= MENU_LEVEL_MIN) && (message->what <= MENU_LEVEL_MAX)) {
			level = message->what - MENU_LEVEL_MIN+1;
		} else BWindow::MessageReceived(message);
	}
}


static char settingsFilename[] = "4 Connect settings";

void ConnectAppWindow::SetDefaultValues() {
	level = 3;
	playerStarts = PLAYER1;
	playerMode = HUMANHUMAN;		
	sound = true;
}

bool ReadAndCheck(BFile &file, int &data, int min, int max) {
	return (file.Read(&data, sizeof(data)) == sizeof(data))	&&
			(data >= min) && (data <= max);
}

void ConnectAppWindow::LoadSettings() {
BFile file;
BPath path;
int s;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(settingsFilename);
		if ((file.SetTo(path.Path(), B_READ_ONLY) == B_OK) && 
			ReadAndCheck(file, level, 1, 20) &&
			ReadAndCheck(file, playerStarts, 0, 1) &&
			ReadAndCheck(file, playerMode, 0, 3) &&
			ReadAndCheck(file, s, 0, 1)) {
				sound = s;
				return;
			}
	}
	SetDefaultValues();
}

void ConnectAppWindow::SaveSettings() {
BFile file;
BPath path;
int s;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(settingsFilename);
		if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) { 
			file.Write(&level, sizeof(level));
			file.Write(&playerStarts, sizeof(playerStarts));
			file.Write(&playerMode, sizeof(playerMode));
			s = sound;
			file.Write(&s, sizeof(s));
		}
	}
}

#define FRAMES 20

void ConnectAppWindow::StartTimer() {
	if (timer == NULL) {
		timer = new BMessageRunner(this, new BMessage(TIMER_MSG), 1000000/FRAMES);
	}
}

void ConnectAppWindow::StopTimer() {
	if (timer != NULL) {
		delete timer; timer = NULL;
	}
}

bool ConnectAppWindow::IsHumanOnMove() {
	return playing && !calculating; 
}

bool ConnectAppWindow::IsGameOver() {
	return !playing;
}

bool ConnectAppWindow::QuitRequested() {
	StopTimer();
	SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

ConnectApp::ConnectApp() : BApplication("application/x-vnd.4connect") {
	BRect aRect;
	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 410, 380);
	window = NULL;
	window = new ConnectAppWindow(aRect);		
	sem = create_sem(0, "sem");
	calculate_move_tid = spawn_thread(calculate_move_thread, 
									  "calculate_move_thread", 
									  B_NORMAL_PRIORITY, NULL);	
	resume_thread(calculate_move_tid);
}

int main(int argc, char *argv[]) { 
	be_app = NULL;
	new ConnectApp();
	be_app->Run();
	delete be_app; 
	return 0;
}