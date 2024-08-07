
/*
        The powerup display for textboxes.
        E.g. when you get a life capsule or new weapon and
                 it shows you a picture of it.
*/

#include "ItemImage.h"

#include "../graphics/Renderer.h"
using namespace NXE::Graphics;
#include "TextBox.h"

#define ITEMBOX_W 76
#define ITEMBOX_H 32

#define ITEMBOX_X (Renderer::getInstance()->screenWidth / 2) - 32
#define ITEMBOX_Y (Renderer::getInstance()->screenHeight / 2)

/*
void c------------------------------() {}
*/

void TB_ItemImage::ResetState()
{
  fVisible = false;
}

void TB_ItemImage::SetVisible(bool enable)
{
  fVisible = enable;
}

void TB_ItemImage::SetSprite(int sprite, int frame)
{
  fSprite  = sprite;
  fFrame   = frame;
  fYOffset = 1;
}

/*
void c------------------------------() {}
*/

void TB_ItemImage::Tick(void)
{
}

void TB_ItemImage::Draw(void)
{
  if (!fVisible)
    return;

  // animate moving item downwards into box
  int desty = (ITEMBOX_H / 2) - (Renderer::getInstance()->sprites.sprites[fSprite].th / 2);
  if (++fYOffset > desty)
    fYOffset = desty;

  // draw the box frame
  TextBox::DrawFrame(ITEMBOX_X, ITEMBOX_Y, ITEMBOX_W, ITEMBOX_H);

  // draw the item
  int x = ITEMBOX_X + ((ITEMBOX_W / 2) - (Renderer::getInstance()->sprites.sprites[fSprite].tw / 2));
  if (Renderer::getInstance()->sprites.sprites[fSprite].tw == 14)
    x--; // hack for ArmsIcons

  Renderer::getInstance()->sprites.drawSprite(x, ITEMBOX_Y + fYOffset, fSprite, fFrame);
}
