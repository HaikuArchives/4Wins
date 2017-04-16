#include "StatusView.h"

StatusView::StatusView(BRect rect, const char *name, const char *text) : 
	BStringView(rect, name, text, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE) {
	//SetFont();
	//SetFontSize();
	SetViewColor(225,225,225,0);
	SetLowColor(225,225,225,0);
}
