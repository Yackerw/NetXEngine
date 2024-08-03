
#include "YesNoPrompt.h"

#include "../autogen/sprites.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../nx.h"
#include "../player.h"
#include "../sound/SoundManager.h"
using namespace NXE::Graphics;;

enum
{
  STATE_APPEAR,
  STATE_WAIT,
  STATE_YES_SELECTED,
  STATE_NO_SELECTED
};

#define YESNO_X (Renderer::getInstance()->screenWidth / 2) + 56
#define YESNO_Y (Renderer::getInstance()->screenHeight / 2) + 22
#define YESNO_POP_SPEED 4

/*
void c------------------------------() {}
*/

void TB_YNJPrompt::ResetState()
{
  fVisible = false;
  fAnswer  = -1;
}

void TB_YNJPrompt::SetVisible(bool enable)
{
  fVisible = enable;

  if (fVisible)
  {
    fState    = STATE_APPEAR;
    fCoords.y = YESNO_Y + (YESNO_POP_SPEED * 2);
    fAnswer   = -1;
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_PROMPT);
  }
}

/*
void c------------------------------() {}
*/

void TB_YNJPrompt::Tick()
{
  if (!fVisible)
    return;

  switch (fState)
  {
    case STATE_APPEAR:
    {
      fCoords.y -= YESNO_POP_SPEED;

      if (fCoords.y <= YESNO_Y)
      {
        fCoords.y = YESNO_Y;
        fState    = STATE_WAIT;
        fTimer    = 15;
      }
      else
        break;
    }
    case STATE_WAIT:
    {
      if (fTimer)
      {
        fTimer--;
        break;
      }

      fState = STATE_YES_SELECTED;
    }
    break;

    case STATE_YES_SELECTED:
    case STATE_NO_SELECTED:
    {
      if (justpushed(LEFTKEY) || justpushed(RIGHTKEY))
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_MOVE);

        fState = (fState == STATE_YES_SELECTED) ? STATE_NO_SELECTED : STATE_YES_SELECTED;
      }

      if (justpushed(ACCEPT_BUTTON))
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MENU_SELECT);
        lastinputs[ACCEPT_BUTTON]  = true;
        lastpinputs[ACCEPT_BUTTON] = true;

        fAnswer = (fState == STATE_YES_SELECTED) ? YES : NO;
        SetVisible(false);
      }
      if (justpushed(DECLINE_BUTTON))
      {
        fState = STATE_NO_SELECTED;
      }
    }
    break;
  }
  Draw();
}

void TB_YNJPrompt::Draw()
{
  if (!fVisible)
    return;

  Renderer::getInstance()->sprites.drawSprite(YESNO_X, fCoords.y, SPR_YESNO, 0, 0);

  // draw hand selector
  if (fState == STATE_YES_SELECTED || fState == STATE_NO_SELECTED)
  {
    int xoff = (fState == STATE_YES_SELECTED) ? -4 : 37;
    Renderer::getInstance()->sprites.drawSprite(YESNO_X + xoff, fCoords.y + 12, SPR_YESNOHAND, 0, 0);
  }
}

/*
void c------------------------------() {}
*/

bool TB_YNJPrompt::ResultReady()
{
  return (fAnswer != -1);
}

int TB_YNJPrompt::GetResult()
{
  return fAnswer;
}
