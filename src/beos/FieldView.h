#ifndef FIELD_VIEW_H
#define FIELD_VIEW_H

#include <View.h>
#include <List.h>
#include "4connect.h"

class FieldView : public BView {
	BBitmap *bmStone[2], *buffer;
	BList animatedStones[2];
	bool connected4[ROWS][COLUMNS];
	char field[ROWS][COLUMNS];
	int animationFrame;
public:
	FieldView(BRect rect, BPath appPath);
	~FieldView();
	void Draw(BRect updateRect);
	void FrameResized(float width, float height);
	void DrawBuffer(BRect updateRect);
	void DrawBuffer();
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void UpdateCursor(bool in_view, float x);
	void ClearField(char player = PLAYER1); // starting player
	void CopyGameField(void);
	int GetColumn(float x);
	void GameOver(int column, int row); // call back function on 4 wins
	void RestartGame(void);
	void AnimationNextFrame(void);
	void LoadStones(const char filename[], BPath graphicsPath, BList &list);
};
#endif
