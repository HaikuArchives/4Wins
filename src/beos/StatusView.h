#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H

#include <StringView.h>

class StatusView : public BStringView {
public:
	StatusView(BRect rect, const char *name, const char *text);
};

#endif
