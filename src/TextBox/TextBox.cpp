
#include "TextBox.h"

#include "../autogen/sprites.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../nx.h"
#include "../game.h"
#include "../settings.h"
#include "../sound/SoundManager.h"
#include "../Utils/Logger.h"
#include "../i18n/minibidi.h"

#include <utf8.h>
#include <iostream>
using namespace NXE::Graphics;

#define MAXLINELEN_FACE 26
#define MAXLINELEN_NO_FACE 35

#define CONTENT_Y 10
#define FACE_W 48

bool TextBox::Init()
{
  fCoords.w = MSG_W;
  fCoords.h = MSG_H;
  fCoords.x = ((Renderer::getInstance()->screenWidth / 2) - (MSG_W / 2));

  SetFace(0);
  SetVisible(false);
  ResetState();

  return 0;
}

void TextBox::Deinit() {}

/*
void c------------------------------() {}
*/

// reset the message box to it's starting parameters.
// called at start and end of a script.
//
// note that this does NOT reset the Face parameter.
// because some scripts (e.g., when balrog busts in the Shack),
// call <FAC before running the <MSG, while the box is still
// invisible.
void TextBox::ResetState(void)
{
  LOG_DEBUG("TextBox::ResetState()");

  fVisible = false;
  fFlags   = TB_DEFAULTS;
  fFace    = 0;

  fScrolling   = false;
  fTextYOffset = 0;

  fTextTimer  = 0;
  fCanSpeedUp = true;

  fCursorVisible = false;
  fCursorTimer   = 0;

  YesNoPrompt.ResetState();
  ItemImage.ResetState();
  StageSelect.ResetState();
  SaveSelect.ResetState();

  ClearText();
}

// set textbox visible (or not) and set the values
// of the flags TB_DRAW_AT_TOP and TB_NO_BORDER.
void TextBox::SetVisible(bool enable, uint8_t flags)
{
  LOG_DEBUG("TextBox::SetVisible({})", enable ? "true" : "false");

  fCoords.w = MSG_W;
  fCoords.h = MSG_H;
  fCoords.x = ((Renderer::getInstance()->screenWidth / 2) - (MSG_W / 2));

  if (enable && fVisible)
    ClearText();

  fVisible = enable;
  flags |= (fFlags & ~(TB_DRAW_AT_TOP | TB_NO_BORDER));
  SetFlags(flags);
}

void TextBox::RecalculateOffsets()
{
  fCoords.w = MSG_W;
  fCoords.h = MSG_H;
  fCoords.x = ((Renderer::getInstance()->screenWidth / 2) - (MSG_W / 2));
  fCoords.y = (fFlags & TB_DRAW_AT_TOP) ? MSG_UPPER_Y : ((Renderer::getInstance()->screenHeight - MSG_H) - 2);
}

void TextBox::SetFlags(uint8_t flags)
{
  fFlags    = flags;
  fCoords.y = (fFlags & TB_DRAW_AT_TOP) ? MSG_UPPER_Y : ((Renderer::getInstance()->screenHeight - MSG_H) - 2);
}

void TextBox::SetFlags(uint8_t flags, bool enable)
{
  LOG_DEBUG("TextBox::SetFlags({:#x}, {})", flags, enable ? "true" : "false");

  if (enable)
    SetFlags(fFlags | flags);
  else
    SetFlags(fFlags & ~flags);
}

/*
void c------------------------------() {}
*/

// add the specified text into the message buffer
// so it starts appearing.
void TextBox::AddText(const std::string &str)
{
  if (!fVisible)
    return;

  char buf[256];
  crtoslashn((char*)str.c_str(), buf);

  LOG_DEBUG("Str: {}", buf);

  std::string result = str;
  std::vector<uint32_t> utf32result;
  utf8::utf8to32(str.begin(), str.end(), std::back_inserter(utf32result));
  doBidi(&utf32result[0], utf32result.size(), true, false);
  result.clear();
  utf8::utf32to8(utf32result.begin(), utf32result.end(), std::back_inserter(result));

  if (rtl())
  {
    std::string tmp = fCharsWaiting;
    fCharsWaiting = result;
    fCharsWaiting.append(tmp);
  }
  else
  {
      fCharsWaiting.append(result);
  }

  crtoslashn((char*)fCharsWaiting.c_str(), buf);

  LOG_DEBUG("fCharsWaiting: {}", buf);

  faceFrame = 0;
  faceStep = 1;
}

// clear all text in the message box
void TextBox::ClearText()
{
  fCharsWaiting.clear();
  for (int i = 0; i < MSG_NLINES; i++)
  {
    fLines[i].clear();
  }

  fCurLine    = 0;
  fCurLineLen = 0;
  fTextTimer  = 0;

  fTextYOffset = 0;
  fScrolling   = false;

  ShowCursor(false);
}

void TextBox::SetText(const std::string &str)
{
  ClearText();
  AddText(str);
}

/*
void c------------------------------() {}
*/

void TextBox::SetFace(int newface)
{
  LOG_DEBUG("TextBox::SetFace({})", newface);
  fFace        = newface;
  if (newface != 0)
  {
    fFaceXOffset = (rtl() ? FACE_W : -FACE_W);
  }
  faceFrame = 0;
  faceStep = 1;
}

void TextBox::ShowCursor(bool enable)
{
  LOG_DEBUG("TextBox::ShowCursor({})", enable ? "true" : "false");
  fCursorVisible = enable;
  fCursorTimer   = 8;
}

/*
void c------------------------------() {}
*/

// returns true if a message box is up
bool TextBox::IsVisible(void)
{
  return fVisible;
}

// returns true if a message box is up AND is still displaying letters
bool TextBox::IsBusy(void)
{
  return (fVisible && (!fCharsWaiting.empty()));
}

/*
void c------------------------------() {}
*/

void TextBox::Draw(void)
{
  if (fVisible)
  {
    DrawTextBox();

    // draw all the extra prompts, boxes, etc if needed
    ItemImage.Draw();
    YesNoPrompt.Draw();
    StageSelect.Draw();
    SaveSelect.Draw();
  }
}

void TextBox::Tick(void)
{
  if (fVisible)
  {
    TickTextBox();

//    ItemImage.Tick();
    YesNoPrompt.Tick();
    StageSelect.Tick();
    SaveSelect.Tick();
  }
  Draw();
}

void TextBox::TickTextBox()
{
  // allow player to speed up text by holding the button
  if (buttondown())
  {
    if (fCanSpeedUp)
      fTextTimer = 9999;
  }
  else
  {
    fCanSpeedUp = true;
  }

  // in the middle of scrolling a line up?
  if (fScrolling)
  {

    fTextYOffset -= (MSG_LINE_SPACING / 4);
    if (fTextYOffset <= -MSG_LINE_SPACING)
    {
      fTextYOffset = 0;
      fTextTimer   = 0;
      fScrolling   = false;

      for (int i = 0; i < MSG_NLINES - 1; i++)
      {
        fLines[i] = fLines[i + 1];
      }

      fLines[MSG_NLINES - 1].clear();
      fCurLine    = (MSG_NLINES - 2);
      fCurLineLen = 1;
    }
  }
  else
  { // add text into the box
    if (!fCharsWaiting.empty())
    {
      if (++fTextTimer >= 3)
      {
        fTextTimer = 0;
        AddNextChar();
      }
    }
    else
    {
      faceFrame = 0;
      faceStep = 0;
    }
  }

  // blink the cursor (it is visible when < 7)
  if (!fCursorVisible || (fFlags & TB_CURSOR_NEVER_SHOWN))
  {
    fCursorTimer = 9999;
  }
  else
  {
    if (++fCursorTimer >= 20)
      fCursorTimer = 0;
  }
}


void TextBox::DrawTextBox()
{
  int text_top = (fCoords.y + 10);
  int text_x   = (fCoords.x + 14);
  if (rtl())
  {
    text_x   = (fCoords.x + fCoords.w - 14);
  }

  // draw the frame
  if (!(fFlags & TB_NO_BORDER))
  {
    DrawFrame(fCoords.x, fCoords.y, fCoords.w, fCoords.h);
  }

  // set clipping region to inside of frame, so that text cannot
  // overflow during scrolling, etc.
  if (rtl())
  {
    Renderer::getInstance()->setClip(0, text_top, fCoords.x + fCoords.w, 48);
  }
  else
  {
    Renderer::getInstance()->setClip((fCoords.x + 14), text_top, Renderer::getInstance()->screenWidth, 48);
  }

  // draw face
  if (fFace != 0)
  {
    if (rtl())
    {
      if (settings->animated_facepics)
        Renderer::getInstance()->sprites.drawSpriteMirrored(fCoords.x + fCoords.w - FACE_W + fFaceXOffset - 14, fCoords.y + CONTENT_Y - 3, SPR_FACES_0 + faceFrame, fFace);
      else
        Renderer::getInstance()->sprites.drawSpriteMirrored(fCoords.x + fCoords.w - FACE_W + fFaceXOffset - 14, fCoords.y + CONTENT_Y - 3, SPR_FACES, fFace);
    }
    else
    {
      if (settings->animated_facepics)
        Renderer::getInstance()->sprites.drawSprite((fCoords.x + 14) + fFaceXOffset, fCoords.y + CONTENT_Y - 3, SPR_FACES_0 + faceFrame, fFace);
      else
        Renderer::getInstance()->sprites.drawSprite((fCoords.x + 14) + fFaceXOffset, fCoords.y + CONTENT_Y - 3, SPR_FACES, fFace);
    }

    text_x += rtl() ? -(FACE_W + 8) : (FACE_W + 8); // move text over by width of face

    // face slide-in animation
    if (rtl())
    {
      if (fFaceXOffset > 0)
      {
        fFaceXOffset -= (Renderer::getInstance()->sprites.sprites[SPR_FACES].w / 6);
        if (fFaceXOffset < 0)
          fFaceXOffset = 0;
      }
    }
    else
    {
      if (fFaceXOffset < 0)
      {
        fFaceXOffset += (Renderer::getInstance()->sprites.sprites[SPR_FACES].w / 6);
        if (fFaceXOffset > 0)
          fFaceXOffset = 0;
      }
    }
  }

  // draw text lines (the 4th line is for the first char shown on the new line during scrolling)
  int y = (text_top + fTextYOffset);

  for (int i = 0; i < MSG_NLINES; i++)
  {
    int lineWidth = 0;
    if (!(fFlags & TB_NO_BORDER))
        lineWidth = Renderer::getInstance()->font.draw(text_x, y, fLines[i]);
    else
        lineWidth = Renderer::getInstance()->font.draw(text_x, y, fLines[i], 0xFFFFFF, true);

    // draw the cursor
    if (i == fCurLine && fCursorTimer < 7)
    {
      int x = (text_x + (rtl() ? -lineWidth : lineWidth));
      Renderer::getInstance()->fillRect(x, y, x + 4, y + Renderer::getInstance()->font.getBase(), 255, 255, 255);
    }

    y += MSG_LINE_SPACING;
  }

  // release the clipping region clipping our drawing to the text box
  Renderer::getInstance()->clearClip();
}

// adds the next char to the box, or, in TB_LINE_AT_ONCE mode,
// the entire next line.
void TextBox::AddNextChar(void)
{
  bool line_at_once = (fFlags & TB_LINE_AT_ONCE);
  int maxlinelen    = GetMaxLineLen();

  if (settings->animated_facepics)
  {
    if (++faceTimer > 1)
    {
      faceTimer = 0;
      faceFrame += faceStep;

      if (faceFrame < 0)
      {
        faceStep = 1;
        faceFrame = 0;
      }
      if (faceFrame >= 3)
      {
        faceStep = -1;
        faceFrame = 2;
      }
    }
  }

  while (!fCharsWaiting.empty())
  {
    std::string::iterator it;
    char32_t ch;
    if (rtl())
    {
      it = fCharsWaiting.end();
      ch              = utf8::prior(it, fCharsWaiting.begin());
      fCharsWaiting.erase(it, fCharsWaiting.end());
    }
    else
    {
      it = fCharsWaiting.begin();
      ch              = utf8::next(it, fCharsWaiting.end());
      fCharsWaiting.erase(fCharsWaiting.begin(), it);
    }

    if (ch == 10)
      continue; // ignore LF's, we look only for CR

    LOG_DEBUG("ch: {}", (uint32_t)ch);

    // go to next line on CR's, or wrap text if needed
    if ((fCurLineLen > maxlinelen) || ch == 13)
    { // went over end of line
      fCurLineLen = 0;
      fCurLine++;

      // - in line-at-once mode we stop once we hit a CR
      // - in char-at-once mode we don't count the CR as being the one char we added
      if (ch == 13)
      {
        if (line_at_once)
          break;
        else
          continue;
      }
    }

    // CR's make no sound
    if (!line_at_once && ch != 13)
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_MSG);

    fCurLineLen++;

    if (rtl())
    {
      std::string line = fLines[fCurLine];
      fLines[fCurLine].clear();
      utf8::append(ch, std::back_inserter(fLines[fCurLine]));
      fLines[fCurLine].append(line);
    }
    else
    {
      utf8::append(ch, std::back_inserter(fLines[fCurLine]));
    }

    if (fCurLine >= MSG_NLINES - 1)
    { // went over bottom of box
      fScrolling = true;
    }

    // adding CR's takes no time
    if (line_at_once || ch == 13)
      continue;
    else
      break;
  }
}

int TextBox::GetMaxLineLen()
{
  if (fFlags & TB_VARIABLE_WIDTH_CHARS)
    return 9999;

  return (fFace != 0) ? MAXLINELEN_FACE : MAXLINELEN_NO_FACE;
}

/*
void c------------------------------() {}
*/

void TextBox::SetCanSpeedUp(bool newstate)
{
  fCanSpeedUp = newstate;
  if (!fCanSpeedUp)
    fTextTimer = 0;
}

/*
void c------------------------------() {}
*/

// draws the frame of an empty text box spanning
// the specified coordinates.
void TextBox::DrawFrame(int x, int y, int w, int h)
{
  Renderer::getInstance()->sprites.drawSpriteChopped(x, y, SPR_TEXTBOX, 0, w, 8, 210); // draw top
  y += 8;

  for (int draw = 0; draw < h - 16; draw += 8)
  {
    Renderer::getInstance()->sprites.drawSpriteChopped(x, y, SPR_TEXTBOX, 1, w, 8, 210); // draw middle
    y += 8;
  }

  Renderer::getInstance()->sprites.drawSpriteChopped(x, y, SPR_TEXTBOX, 2, w, 8, 210); // draw bottom
}
