
#ifndef _DIALOG_H
#define _DIALOG_H

#include "options.h"

#include <vector>
namespace Options
{

struct ODItem;

enum OD_TYPES
{
  OD_ACTIVATED,
  OD_SEPARATOR,
  OD_DISMISS,
  OD_CHOICE,
  OD_DISABLED,
};

class Dialog : public FocusHolder
{
public:
  Dialog();
  ~Dialog();

  int DLG_X;
  int DLG_Y;
  int DLG_W;
  int DLG_H;

  ODItem *AddItem(const char *text, void (*activate)(ODItem *, int) = NULL, void (*update)(ODItem *) = NULL,
                  int id = -1, int type = OD_ACTIVATED);
  ODItem *AddSeparator();
  ODItem *AddDisabledItem(const char *text);
  ODItem *AddDismissalItem(const char *text = NULL);

  void Draw();
  void RunInput();
  void Dismiss();
  void Clear();
  void Refresh();

  void UpdateSizePos();
  void SetSize(int w, int h);
  void offset(int xd, int yd);

  void SetSelection(int sel);
  unsigned int GetSelection()
  {
    return fCurSel;
  }
  void ShowFull()
  {
    fNumShown = 99;
  }

  void (*onclear)();
  void (*ondismiss)();
  std::vector<ODItem *> &Items();

private:
  void DrawItem(int x, int y, ODItem *item);

  int fCurSel;
  unsigned int fNumShown; // for text-draw animation on entry
  int fRepeatTimer;
  std::vector<ODItem *> fItems;

  struct
  {
    int x, y, w, h;
  } fCoords;
  int fTextX;
};

struct ODItem
{
  char text[100];
  char suffix[32];
  char righttext[64];
  char raligntext[32];
  int type, id;

  void (*update)(ODItem *item);
  void (*activate)(ODItem *item, int dir);
};

} // namespace Options

#endif
