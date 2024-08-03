
#include "dialog.h"

#include "../game.h"
#include "../graphics/Renderer.h"
#include "../nx.h"
using namespace NXE::Graphics;
#include "../TextBox/TextBox.h"
#include "../autogen/sprites.h"
#include "../input.h"
#include "../sound/SoundManager.h"

using namespace Options;
extern std::vector<void *> optionstack;

#define REPEAT_WAIT 30
#define REPEAT_RATE 4

Dialog::Dialog()
{
  if (Renderer::getInstance()->widescreen)
  {
    DLG_X = ((Renderer::getInstance()->screenWidth / 2) - 110);
    DLG_Y = ((Renderer::getInstance()->screenHeight / 2) - 100);
    DLG_W = 240;
    DLG_H = 200;
  }
  else
  {
    DLG_X = ((Renderer::getInstance()->screenWidth / 2) - 88);
    DLG_Y = ((Renderer::getInstance()->screenHeight / 2) - 100);
    DLG_W = 190;
    DLG_H = 200;
  }

  onclear   = NULL;
  ondismiss = NULL;

  fCoords.x = DLG_X;
  fCoords.y = DLG_Y;
  fCoords.w = DLG_W;
  fCoords.h = DLG_H;

  fTextX    = (fCoords.x + (rtl() ? (DLG_W-48) : 48));

  fCurSel      = 0;
  fNumShown    = 0;
  fRepeatTimer = 0;

  optionstack.push_back(this);
}

Dialog::~Dialog()
{
  for (unsigned int i = 0; i < fItems.size(); i++)
    delete fItems.at(i);
  for (std::vector<void *>::iterator it = optionstack.begin(); it != optionstack.end(); ++it)
  {
    if (*it == this)
    {
      optionstack.erase(it);
      break;
    }
  }
}

void Dialog::UpdateSizePos()
{
  if (Renderer::getInstance()->widescreen)
  {
    DLG_X = ((Renderer::getInstance()->screenWidth / 2) - 110);
    DLG_Y = ((Renderer::getInstance()->screenHeight / 2) - 100);
    DLG_W = 240;
    DLG_H = 200;
  }
  else
  {
    DLG_X = ((Renderer::getInstance()->screenWidth / 2) - 88);
    DLG_Y = ((Renderer::getInstance()->screenHeight / 2) - 100);
    DLG_W = 190;
    DLG_H = 200;
  }

  fCoords.x = ((DLG_W / 2) - (fCoords.w / 2)) + DLG_X;
  fCoords.y = ((DLG_H / 2) - (fCoords.h / 2)) + DLG_Y;
  fTextX    = (fCoords.x + (rtl() ? (fCoords.w - 34) : 34));
}

void Dialog::SetSize(int w, int h)
{
  fCoords.w = w;
  fCoords.h = h;
  fCoords.x = ((DLG_W / 2) - (w / 2)) + DLG_X;
  fCoords.y = ((DLG_H / 2) - (h / 2)) + DLG_Y;
  fTextX    = (fCoords.x + (rtl() ? (w -34) : 34));
}

void Dialog::offset(int xd, int yd)
{
  fCoords.x += xd;
  fCoords.y += yd;
  fTextX += (rtl() ? -xd : xd);
}

/*
void c------------------------------() {}
*/

ODItem *Dialog::AddItem(const char *text, void (*activate)(ODItem *, int), void (*update)(ODItem *), int id, int type)
{
  ODItem *item = new ODItem;
  memset(item, 0, sizeof(ODItem));

  strcpy(item->text, text);

  item->activate = activate;
  item->update   = update;
  item->id       = id;
  item->type     = type;

  fItems.push_back(item);

  if (update)
    (*update)(item);

  return item;
}

ODItem *Dialog::AddSeparator()
{
  return AddItem("", NULL, NULL, -1, OD_SEPARATOR);
}

ODItem *Dialog::AddDisabledItem(const char *text)
{
  return AddItem(text, NULL, NULL, -1, OD_DISABLED);
}

ODItem *Dialog::AddDismissalItem(const char *text)
{
  if (!text)
    text = "Return";
  return AddItem(text, NULL, NULL, -1, OD_DISMISS);
}

/*
void c------------------------------() {}
*/

void Dialog::Draw()
{
  UpdateSizePos();
  TextBox::DrawFrame(fCoords.x, fCoords.y, fCoords.w, fCoords.h);

  int x = fTextX;
  int y = (fCoords.y + Renderer::getInstance()->font.getBase());
  for (unsigned int i = 0; i < fItems.size(); i++)
  {
    ODItem *item = (ODItem *)fItems.at(i);

    if (i < (unsigned int)fNumShown)
      DrawItem(x, y, item);

    if (i == (unsigned int)fCurSel)
      Renderer::getInstance()->sprites.drawSprite(x + (rtl() ? 16 : -16), y + 1, SPR_WHIMSICAL_STAR, 1);
    if (item->type == OD_SEPARATOR)
      y += 5;
    else
      y += Renderer::getInstance()->font.getHeight();
  }

  if (fNumShown < 99)
    fNumShown++;
}

void Dialog::DrawItem(int x, int y, ODItem *item)
{
  char text[264];

  if (rtl())
  {
    strcpy(text, _(item->suffix).c_str());
    strcat(text, _(item->text).c_str());
  }
  else
  {
    strcpy(text, _(item->text).c_str());
    strcat(text, _(item->suffix).c_str());
  }

  // comes first because it can trim the text on the left side if needed
  if (item->raligntext[0])
  {
    int rx = (fCoords.x + fCoords.w) - 10;
    rx -= Renderer::getInstance()->font.getWidth(_(item->raligntext));
    Renderer::getInstance()->font.draw(rx, y, _(item->raligntext));
  }

  if (item->type == OD_DISABLED)
    Renderer::getInstance()->font.draw(x, y, text, 0x666666);
  else
    Renderer::getInstance()->font.draw(x, y, text);

  // for key remaps
  if (item->righttext[0])
  {
    if (rtl())
    {
      Renderer::getInstance()->font.draw((fCoords.x) + 62, y, item->righttext);
    }
    else
    {
      Renderer::getInstance()->font.draw((fCoords.x + fCoords.w) - 62, y, item->righttext);
    }
  }
}

/*
void c------------------------------() {}
*/

void Dialog::RunInput()
{
  if (inputs[UPKEY] || inputs[DOWNKEY])
  {
    int dir = (inputs[DOWNKEY]) ? 1 : -1;

    if (!fRepeatTimer)
    {
      fRepeatTimer = (lastinputs[UPKEY] || lastinputs[DOWNKEY]) ? REPEAT_RATE : REPEAT_WAIT;
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);

      int nitems = fItems.size();
      for (;;)
      {
        fCurSel += dir;
        if (fCurSel < 0)
          fCurSel = (nitems - 1);
        if (fCurSel >= (int)fItems.size())
          fCurSel = 0;

        if (fCurSel >= 0 && fCurSel < (int)fItems.size())
        {
          ODItem *item = fItems.at(fCurSel);
          if (item && item->type != OD_SEPARATOR && item->type != OD_DISABLED)
            break;
        }
      }
    }
    else
      fRepeatTimer--;
  }
  else
    fRepeatTimer = 0;

  if (justpushed(ACCEPT_BUTTON) || justpushed(RIGHTKEY) || justpushed(LEFTKEY) || justpushed(ENTERKEY))
  {
    int dir = (!inputs[LEFTKEY] || buttonjustpushed() || justpushed(RIGHTKEY) || justpushed(ENTERKEY)) ? 1 : -1;
    if (justpushed(ACCEPT_BUTTON) || justpushed(ENTERKEY))
      dir = 0;

    ODItem *item = NULL;
    if (fCurSel >= 0 && fCurSel < (int)fItems.size())
      item = fItems.at(fCurSel);
    if (item)
    {
      if (item->type == OD_DISMISS)
      {
        if (dir == 0)
        {
          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
          if (ondismiss)
            (*ondismiss)();
          return;
        }
      }
      else if ((item->type == OD_ACTIVATED) && item->activate && (dir == 0))
      {
        if (item->update)
          (*item->update)(item);

        (*item->activate)(item, dir);
      }
      else if ((item->type == OD_CHOICE) && item->activate && (dir != 0))
      {
        (*item->activate)(item, dir);

        if (item->update)
          (*item->update)(item);
      }
    }
  }

  if (justpushed(ESCKEY) || justpushed(DECLINE_BUTTON))
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);
    if (ondismiss)
      (*ondismiss)();
    return;
    //		Dismiss();
    //		return;
  }
}

void Dialog::SetSelection(int sel)
{
  if (sel < 0)
    sel = fItems.size();
  if (sel >= (int)fItems.size())
    sel = (fItems.size() - 1);

  fCurSel = sel;
}

void Dialog::Dismiss()
{
  delete this;
}

void Dialog::Refresh()
{
  for (unsigned int i = 0; i < fItems.size(); i++)
  {
    ODItem *item = fItems.at(i);
    if (item->update)
      (*item->update)(item);
  }
}

void Dialog::Clear()
{
  if (onclear)
    (*onclear)();

  for (unsigned int i = 0; i < fItems.size(); i++)
    delete fItems.at(i);

  fItems.clear();
  fNumShown = 0;
  fCurSel   = 0;
}

std::vector<ODItem *> &Dialog::Items()
{
  return fItems;
}
