#include "FieldView.h"
#include <TranslationUtils.h>
#include <Bitmap.h>
#include "4ConnectApp.h"
#include <memory.h>
#include <String.h>

extern "C" void CopyGameField(void) {
	if (my_app != NULL && (my_app->window != NULL) && 
		(my_app->window->fieldview != NULL))
		my_app->window->fieldview->CopyGameField();
}

BBitmap* ConvertBitmap(BBitmap* src,
         BRect dst,
         color_space color,
         bool preserve_aspect_ratio) {
  // convert a bitmap from one size/color_space into a different
  // size color_space (possibly maintaining the aspect ratio) by
  // using the app_servers DrawBitmap

  // create a temporary bitmap and attach a view to it so we
  // can draw into it

  BBitmap* tmp = new BBitmap(dst, color, true);
  BBitmap* final = new BBitmap(dst, color);
  BView* view = new BView(dst, "", B_FOLLOW_ALL, B_WILL_DRAW);

  tmp->AddChild(view);
  tmp->Lock();
  view->SetHighColor(B_TRANSPARENT_32_BIT);
  view->FillRect(dst);

  if (preserve_aspect_ratio) {
      float f;
      BRect bounds(src->Bounds());

      if (bounds.Width() > bounds.Height()) {
    // leave width, adjust height to maintain aspect ratio
    f = bounds.Height() / bounds.Width() * dst.Height();
    dst.top = dst.top + ((dst.Height() - f) / 2);
    dst.bottom = dst.top + f;
      } else {
    // leave height, adjust width to maintain aspect ratio
    f = bounds.Width() / bounds.Height() * dst.Width();
    dst.left = dst.left + ((dst.Width() - f) / 2);
    dst.right = dst.left + f;
      }
  }

  view->DrawBitmap(src, src->Bounds(), dst);
  // we can draw do additional drawing at this point such as
  // placing a frame around the image or a 'bug' in the corner
  view->Sync();
  tmp->Unlock();
  // copy bits from temporary bitmap into final bitmap
  memcpy(final->Bits(), tmp->Bits(), tmp->BitsLength());
  // delete the temporary bitmap (deletes the attached view as well)
  delete tmp;
  return final;
}

BBitmap* Map(BBitmap *bmp) {
char *ptr = (char*)bmp->Bits();
uint32 *pixel;
uint32 cmp;
int x, y;
BRect rect(bmp->Bounds());
	cmp = *(uint32*)ptr; 
	for (y = 0; y <= rect.Height(); y++) {
		pixel = (uint32*)&ptr[y*bmp->BytesPerRow()];
		for (x = 0; x <= rect.Width(); x++, pixel++) {
			if (*pixel == cmp) 
				*pixel = B_TRANSPARENT_MAGIC_RGBA32;
		}
	}
	return bmp;
}

void FieldView::LoadStones(const char filename[], BPath graphicsPath, BList &list) {
BPath path;
BBitmap *bmp;
int i = 1;
char number[6];
BString string;
	do {
		sprintf(number, "%4.4d", i);
		string = filename; string.Append(number); //string.Append(".tga"); 
		path = graphicsPath; path.Append(string.String());
		bmp = BTranslationUtils::GetBitmap(path.Path());
		if (bmp != NULL) {
			list.AddItem(Map(ConvertBitmap(bmp, bmp->Bounds(), B_RGBA32, true)));
			delete bmp;
			i++;		
		}
	} while (bmp != NULL);
}

FieldView::FieldView(BRect rect, BPath appPath) : 
	BView(rect, NULL, B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_SUBPIXEL_PRECISE | 
		B_FRAME_EVENTS) {
	SetViewColor(B_TRANSPARENT_COLOR);
BBitmap *bmp;
	Init();
	BPath path(appPath); path.Append("graphics/button-yellow0001");
	bmp = BTranslationUtils::GetBitmap(path.Path());
	bmStone[0] = Map(ConvertBitmap(bmp, bmp->Bounds(), B_RGBA32, true));
	delete bmp;
	path = appPath; path.Append("graphics/button-red0001");
	bmp = BTranslationUtils::GetBitmap(path.Path());
	bmStone[1] = Map(ConvertBitmap(bmp, bmp->Bounds(), B_RGBA32, true));
	delete bmp;
	path = appPath; path.Append("graphics/");
	LoadStones("button-yellow", path, animatedStones[0]);
	LoadStones("button-red", path, animatedStones[1]);
	ClearField();
	CopyGameField();

	buffer = new BBitmap(rect, B_RGB16, true);
	FrameResized(rect.Width(), rect.Height());
}

FieldView::~FieldView() {
	delete buffer;
}

void FieldView::CopyGameField() {
int x, y;
	for (y = 0; y < ROWS; y++)
		for (x = 0; x < COLUMNS; x++) field[y][x] = gameField[y][x];	
}

void FieldView::ClearField(char player) {
	NewGame(player); 	
	memset(connected4, false, COLUMNS*ROWS*sizeof(bool));
}

void FieldView::Draw(BRect updateRect) {
	if ((buffer != NULL) && (buffer->Bounds() == Bounds())) {
		DrawBitmap(buffer, updateRect, updateRect);
	}
}

void FieldView::FrameResized(float width, float height) {
	if ((buffer != NULL) && (buffer->Bounds() != Bounds())) {
		delete buffer;
		BRect rect(0, 0, width, height); 
		buffer = new BBitmap(rect, B_RGB16, true);
		DrawBuffer(rect);
	} 	
}

void FieldView::DrawBuffer() {
	DrawBuffer(Bounds());
	Invalidate(Bounds());
}

void FieldView::DrawBuffer(BRect updateRect) {
BView view(buffer->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
BRect rect = Bounds(); 
float dx = (rect.right-rect.left)/COLUMNS, dy = (rect.bottom-rect.top)/ROWS;
float d = (dx < dy) ? dx : dy;
float r = d / 2, ir = r * 0.7, ir2 = ir*1.2;
float left = (rect.Width() - d*COLUMNS)/2.0 + r,
	  top = (rect.Height() - d*ROWS)/2.0 + r;
int x, y, x0, y0, frames, animDelta, animStep;
BPoint from;
rgb_color stoneColor[3] = {{0,255,0,0},{255,0,0,0},{255,255,255,0}};
rgb_color stoneOutlineColor[3] = //{{0,0,0,0},{0,0,0,0},{0,0,0,0}}; 
							   {{0,192,0,0},{192,0,0,0},{0,0,0,0}};
	if (moveListTop > 0) { // position of stone which caused 4 wins
		x0 = moveList[moveListTop].column;
		y0 = moveList[moveListTop].row;
		frames = animatedStones[field[y0][x0]].CountItems();
		animDelta = frames * 0.1;
		if (animDelta <= 0) animDelta = 1;
	}

	buffer->Lock();	
	buffer->AddChild(&view);

	view.SetHighColor(128,128,255);
	view.FillRect(Bounds());

	view.SetHighColor(0,0,192);
	view.FillRect(rect);

	view.SetDrawingMode(B_OP_OVER);
	from.y = top;
	for (y = 0; y < ROWS; y++) {
		from.x = left;
		for (x = 0; x < COLUMNS; x++) {
		int stone = field[y][x];
			view.SetHighColor(stoneColor[FREE]); 
			view.FillEllipse(from, ir, ir);
			view.SetHighColor(stoneOutlineColor[FREE]); 
			view.StrokeEllipse(from, ir, ir);
			if (stone != FREE) {
				if (!connected4[y][x])
					view.DrawBitmap(bmStone[stone], 
						BRect(from.x-ir2, from.y-ir2, from.x+ir2, from.y+ir2));
				else {
					if (x0 == x)
						animStep = animDelta*(y-y0); 
					else
						animStep = animDelta*(x0-x);
				
					BList &list = animatedStones[stone];
				
					view.DrawBitmap((BBitmap*)list.ItemAt((animationFrame+animStep+frames) % frames),
						BRect(from.x-ir2, from.y-ir2, from.x+ir2, from.y+ir2));
				}
			}
			from.x += d;
		}
		from.y += d;
	}

	view.Sync();
	
	buffer->RemoveChild(&view);
	buffer->Unlock();	
}

int FieldView::GetColumn(float x) {
	BRect rect = Bounds();	
	float dx = (rect.right-rect.left)/COLUMNS, dy = (rect.bottom-rect.top)/ROWS;
	float d = (dx < dy) ? dx : dy;	
	int	column = (int)((x-(rect.Width()-d*COLUMNS)/2) / d);

	if (column < 0) 
		column = 0;
	else if (column >= COLUMNS) 
		column = COLUMNS-1;		
	return column;	
}

void FieldView::MouseDown(BPoint point) {
	if (!((ConnectAppWindow*)Window())->IsHumanOnMove()) return;

	((ConnectAppWindow*)Window())->DoMove(GetColumn(point.x)); 
}

void FieldView::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
	UpdateCursor(transit != B_EXITED_VIEW, point.x);
}

void FieldView::UpdateCursor(bool in_view, float x) {
	if (!in_view) {
		be_app->SetCursor(B_HAND_CURSOR);
	} else {
		if (((ConnectAppWindow*)Window())->IsHumanOnMove()) {
			int column = GetColumn(x);
			if (field[0][column] == FREE) {
				if (playerOnMove == PLAYER1)
					be_app->SetCursor(player1cursor);
				else
					be_app->SetCursor(player2cursor);
			} else
				be_app->SetCursor(stop_cursor);
		} else if (((ConnectAppWindow*)Window())->IsGameOver())
			be_app->SetCursor(B_HAND_CURSOR);
		else
			be_app->SetCursor(wait_cursor);
	}
}

void FieldView::GameOver(int column, int row) {
int x = column, xs, y = row, ys, i;
char stone = field[row][column];
	if (stone == FREE) return;
	// horizontally
	while ((x >= 0) && (field[y][x] == stone)) x--;
	x++; xs = x;
	for (i = 0; (x < COLUMNS) && (field[y][x] == stone); i++, x++);
	x--;
	if (i >= 4) for (; xs <= x ; x--) connected4[y][x] = true;
	
	// vertically
	x = column;
	while ((y >= 0) && (field[y][x] == stone)) y--;
	y++; ys = y;
	for (i = 0; (y < ROWS) && (field[y][x] == stone); i++, y++);
	y--;
	if (i >= 4) for (; ys <= y ; y--) connected4[y][x] = true;
	
	// diagonally from bottom left to top right
	y = row;
	while ((y >= 0) && (x >= 0) && (field[y][x] == stone)) x--, y--;
	x++, y++; ys = y;
	for (i = 0; (y < ROWS) && (x < COLUMNS) && (field[y][x] == stone); i++, x++, y++);
	x--; y--;
	if (i >= 4) for (; ys <= y ; x--, y--) connected4[y][x] = true;
	
	// diagonally from top left to bottom right 
	x = column; y = row; 
	while ((y < ROWS) && (x >= 0) && (field[y][x] == stone)) x--, y++;
	x++, y--; ys = y;
	for (i = 0; (y >= 0) && (x < COLUMNS) && (field[y][x] == stone); i++, x++, y--);
	x--; y++;
	if (i >= 4) for (; y <= ys; x--, y++) connected4[y][x] = true;
	animationFrame = 1;
}

void FieldView::RestartGame(void) {
	memset(connected4, false, COLUMNS*ROWS*sizeof(bool));
}

void FieldView::AnimationNextFrame(void) {
	animationFrame ++;
	DrawBuffer();
}