
// sprites routines
#include "graphics.h"
#include "../nx.h"
#include <cstring>
#include <vector>
#include <string>
#include "../siflib/sif.h"
#include "../siflib/sifloader.h"
#include "../siflib/sectSprites.h"
#include "../siflib/sectStringArray.h"
#include "../autogen/sprites.h"
#include "../settings.h"
#include "../ResourceManager.h"
using namespace Graphics;

#include "sprites.h"

static NXSurface *spritesheet[MAX_SPRITESHEETS];
static int num_spritesheets;
static std::vector<std::string> sheetfiles;

SIFSprite sprites[MAX_SPRITES];
int num_sprites;

static bool batch_draw_enabled = false;


// offset things like blockl/r/u/d, bounding box etc by the draw point of all
// sprites so that these things are consistent with where the sprite appears to be
static void offset_by_draw_points()
{
	for(int s=0;s<num_sprites;s++)
	{
		int dx = -sprites[s].frame[0].dir[0].drawpoint.x;
		int dy = -sprites[s].frame[0].dir[0].drawpoint.y;
		
		sprites[s].bbox.offset(dx, dy);
		sprites[s].slopebox.offset(dx, dy);
		sprites[s].solidbox.offset(dx, dy);
		
		sprites[s].block_l.offset(dx, dy);
		sprites[s].block_r.offset(dx, dy);
		sprites[s].block_u.offset(dx, dy);
		sprites[s].block_d.offset(dx, dy);
		
		for(int f=0;f<sprites[s].nframes;f++)
		{
			for(int d=0;d<sprites[s].ndirs;d++)
			{
				int dx = -sprites[s].frame[f].dir[d].drawpoint.x;
				int dy = -sprites[s].frame[f].dir[d].drawpoint.y;
				sprites[s].frame[f].dir[d].pf_bbox.offset(dx, dy);
			}
		}
	}
}

// for sprites which only have 1 dir (no separate frames for left & right),
// create a 2nd identical dir as the rest of the engine doesn't bother
// with this complication.
static void expand_single_dir_sprites()
{
	for(int s=0;s<num_sprites;s++)
	{
		if (sprites[s].ndirs == 1)
		{
			sprites[s].ndirs = 2;
			for(int f=0;f<sprites[s].nframes;f++)
				sprites[s].frame[f].dir[1] = sprites[s].frame[f].dir[0];
		}
	}
}



// create slope boxes for all sprites, used by the slope-handling routines
// these are basically just a form of bounding box describing the bounds of the
// blockd points.
static void create_slope_boxes()
{
	for(int s=0;s<num_sprites;s++)
	{
		if (sprites[s].block_d.count != 0)
		{
			int leftmost = 99999;
			int rightmost = -99999;
			for(int i=0;i<sprites[s].block_d.count;i++)
			{
				if (sprites[s].block_d[i].x < leftmost)  leftmost = sprites[s].block_d[i].x;
				if (sprites[s].block_d[i].x > rightmost) rightmost = sprites[s].block_d[i].x;
			}
			
			sprites[s].slopebox.x1 = leftmost;
			sprites[s].slopebox.x2 = rightmost;
			
			if (sprites[s].block_u.count)
				sprites[s].slopebox.y1 = (sprites[s].block_u[0].y + 1);
			else
				sprites[s].slopebox.y1 = 0;
			
			sprites[s].slopebox.y2 = (sprites[s].block_d[0].y - 1);
		}
	}
	
	sprites[SPR_MYCHAR].slopebox.y1 += 3;
}


static bool load_sif(const std::string& fname)
{
SIFLoader sif;
uint8_t *sheetdata, *spritesdata;
int sheetdatalength, spritesdatalength;

	if (sif.LoadHeader(fname))
		return 1;
	
	if (!(sheetdata = sif.FindSection(SIF_SECTION_SHEETS, &sheetdatalength)))
	{
		staterr("load_sif: file '%s' missing SIF_SECTION_SHEETS", fname.c_str());
		return 1;
	}
	
	if (!(spritesdata = sif.FindSection(SIF_SECTION_SPRITES, &spritesdatalength)))
	{
		staterr("load_sif: file '%s' missing SIF_SECTION_SPRITES", fname.c_str());
		return 1;
	}
	
	// decode sheets
	sheetfiles.clear();
	if (SIFStringArraySect::Decode(sheetdata, sheetdatalength, &sheetfiles))
		return 1;
	
	// decode sprites
	if (SIFSpritesSect::Decode(spritesdata, spritesdatalength, \
						&sprites[0], &num_sprites, MAX_SPRITES))
	{
		staterr("load_sif: SIFSpritesSect decoder failed");
		return 1;
	}
	
	sif.CloseFile();

#if RESSCALE==1
	// Ayy hack
	memcpy(&sprites[SPR_CURLYCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CURLYCHAR].spritesheet = 64;
	sheetfiles.push_back("CurlyChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_SUECHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SUECHAR].spritesheet = 65;
	sheetfiles.push_back("SueChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_KINGCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_KINGCHAR].spritesheet = 66;
	sheetfiles.push_back("KingChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_JACKCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_JACKCHAR].spritesheet = 67;
	sheetfiles.push_back("JackChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_COLONCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_COLONCHAR].spritesheet = 68;
	sheetfiles.push_back("ColonChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_CHAKOCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CHAKOCHAR].spritesheet = 69; //nice
	sheetfiles.push_back("ChakoChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_SANTACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SANTACHAR].spritesheet = 70;
	sheetfiles.push_back("SantaChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_TOROKOCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_TOROKOCHAR].spritesheet = 71;
	sheetfiles.push_back("TorokoChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_PUPPYCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_PUPPYCHAR].spritesheet = 72;
	sheetfiles.push_back("PuppyChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_BOOSTERCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_BOOSTERCHAR].spritesheet = 73;
	sheetfiles.push_back("BoosterChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_DRGEROCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_DRGEROCHAR].spritesheet = 74;
	sheetfiles.push_back("DrGeroChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_NRSHASUMICHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_NRSHASUMICHAR].spritesheet = 75;
	sheetfiles.push_back("NurseHasumiChar.pbm");
	// Ayy hack
	memcpy(&sprites[SPR_JENKACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_JENKACHAR].spritesheet = 76;
	sheetfiles.push_back("JenkaChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_HUMANSUECHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_HUMANSUECHAR].spritesheet = 77;
	sheetfiles.push_back("HumanSueChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_CROWNCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CROWNCHAR].spritesheet = 78;
	sheetfiles.push_back("CrownChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_ROOTCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_ROOTCHAR].spritesheet = 79;
	sheetfiles.push_back("RootChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_SUGURICHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SUGURICHAR].spritesheet = 80;
	sheetfiles.push_back("SuguriChar.pbm");

	// Ayy hack
	memcpy(&sprites[SPR_SORACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SORACHAR].spritesheet = 81;
	sheetfiles.push_back("SoraChar.pbm");
#else
	// Ayy hack
	memcpy(&sprites[SPR_CURLYCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CURLYCHAR].spritesheet = 64;
	sheetfiles.push_back("CurlyChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_SUECHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SUECHAR].spritesheet = 65;
	sheetfiles.push_back("SueChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_KINGCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_KINGCHAR].spritesheet = 66;
	sheetfiles.push_back("KingChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_JACKCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_JACKCHAR].spritesheet = 67;
	sheetfiles.push_back("JackChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_COLONCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_COLONCHAR].spritesheet = 68;
	sheetfiles.push_back("ColonChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_CHAKOCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CHAKOCHAR].spritesheet = 69; //nice
	sheetfiles.push_back("ChakoChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_SANTACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SANTACHAR].spritesheet = 70;
	sheetfiles.push_back("SantaChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_TOROKOCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_TOROKOCHAR].spritesheet = 71;
	sheetfiles.push_back("TorokoChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_PUPPYCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_PUPPYCHAR].spritesheet = 72;
	sheetfiles.push_back("PuppyChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_BOOSTERCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_BOOSTERCHAR].spritesheet = 73;
	sheetfiles.push_back("BoosterChar.bmp");


	// Ayy hack
	memcpy(&sprites[SPR_DRGEROCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_DRGEROCHAR].spritesheet = 74;
	sheetfiles.push_back("DrGeroChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_NRSHASUMICHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_NRSHASUMICHAR].spritesheet = 75;
	sheetfiles.push_back("NurseHasumiChar.bmp");
	// Ayy hack
	memcpy(&sprites[SPR_JENKACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_JENKACHAR].spritesheet = 76;
	sheetfiles.push_back("JenkaChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_HUMANSUECHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_HUMANSUECHAR].spritesheet = 77;
	sheetfiles.push_back("HumanSueChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_CROWNCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_CROWNCHAR].spritesheet = 78;
	sheetfiles.push_back("CrownChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_ROOTCHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_ROOTCHAR].spritesheet = 79;
	sheetfiles.push_back("RootChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_SUGURICHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SUGURICHAR].spritesheet = 80;
	sheetfiles.push_back("SuguriChar.bmp");

	// Ayy hack
	memcpy(&sprites[SPR_SORACHAR], &sprites[SPR_MYCHAR], sizeof(SIFSprite));
	sprites[SPR_SORACHAR].spritesheet = 81;
	sheetfiles.push_back("SoraChar.bmp");
#endif
	
	create_slope_boxes();
	offset_by_draw_points();
	expand_single_dir_sprites();
	
	return 0;
}



bool Sprites::Init()
{
	memset(spritesheet, 0, sizeof(spritesheet));
	
	// load sprites info--sheet positions, bounding boxes etc
#if RESSCALE==1
	if (load_sif(ResourceManager::getInstance()->getLocalizedPath("sprites.sif")))
#else
	if (load_sif(ResourceManager::getInstance()->getLocalizedPath("sprites2x.sif")))
#endif
		return 1;
	
	num_spritesheets = sheetfiles.size();
	return 0;
}

void Sprites::Close()
{
	FlushSheets();
	sheetfiles.clear();
}

void Sprites::FlushSheets()
{
	for(int i=0;i<MAX_SPRITESHEETS;i++)
	{
		if (spritesheet[i])
		{
			delete spritesheet[i];
			spritesheet[i] = NULL;
		}
	}
}

/*
void c------------------------------() {}
*/

namespace Sprites {
// ensure the given spritesheet is loaded
static void LoadSheetIfNeeded(int sheetno)
{
	if (!spritesheet[sheetno])
	{
		spritesheet[sheetno] = new NXSurface;
		spritesheet[sheetno]->LoadImage(ResourceManager::getInstance()->getLocalizedPath(sheetfiles.at(sheetno)), true);
		
	}
}
}

void Sprites::draw_in_batch(bool enabled)
{
    batch_draw_enabled = enabled;
}

// master sprite drawing function
void Sprites::BlitSprite(int x, int y, int s, int frame, uint8_t dir, \
								int xoff, int yoff, int wd, int ht, int alpha)
{
	LoadSheetIfNeeded(sprites[s].spritesheet);
	
	dir %= sprites[s].ndirs;
	SIFDir *sprdir = &sprites[s].frame[frame].dir[dir];
	
    spritesheet[sprites[s].spritesheet]->alpha = alpha;

    if (batch_draw_enabled)
    {
        DrawBatchAdd(spritesheet[sprites[s].spritesheet], \
                     x, y, \
                     (sprdir->sheet_offset.x + xoff), \
                     (sprdir->sheet_offset.y + yoff), \
                     wd, ht);
    }
    else
    {
        DrawSurface(spritesheet[sprites[s].spritesheet], \
                    x, y, \
                    (sprdir->sheet_offset.x + xoff), \
                    (sprdir->sheet_offset.y + yoff), \
                    wd, ht);
    }
    spritesheet[sprites[s].spritesheet]->alpha = 255;
}

/*
void c------------------------------() {}
*/


// draw sprite "s" at [x,y]. drawing frame "frame" and dir "dir".
void Sprites::draw_sprite(int x, int y, int s, int frame, uint8_t dir)
{
	BlitSprite(x, y, s, frame, dir, 0, 0, sprites[s].w, sprites[s].h);
}

RectI Sprites::get_sprite_rect(int x, int y, int s, int/* frame*/, uint8_t/* dir*/)
{
    return RectI(x, y, sprites[s].w, sprites[s].h);
}

// draw sprite "s", place it's draw point at [x,y] instead of it's upper-left corner.
void Sprites::draw_sprite_at_dp(int x, int y, int s, int frame, uint8_t dir)
{
	x -= sprites[s].frame[frame].dir[dir].drawpoint.x;
	y -= sprites[s].frame[frame].dir[dir].drawpoint.y;
	BlitSprite(x, y, s, frame, dir, 0, 0, sprites[s].w, sprites[s].h);
}

// draw a portion of a sprite, such as a sprite in the middle of "teleporting".
// only the area between clipy1 (inclusive) and clipy2 (exclusive) are visible.
void Sprites::draw_sprite_clipped(int x, int y, int s, int frame, uint8_t dir, \
						int clipx1, int clipx2, int clipy1, int clipy2)
{
	BlitSprite(x + clipx1, y + clipy1, s, frame, dir, clipx1, clipy1, \
				(clipx2 - clipx1), (clipy2 - clipy1));
}

// draw a clipped sprite while clipping only the width.
// used for drawing percentage bars, etc.
void Sprites::draw_sprite_clip_width(int x, int y, int s, int frame, int wd)
{
	BlitSprite(x, y, s, frame, 0, 0, 0, wd, sprites[s].h);
}

// draws a sprite at less than it's actual width by chopping it into two chunks.
// on the left, the first "repeat_at" pixels are drawn.
// then, the remaining "wd" is drawn from the right half of the sprite.
// used for things like drawing the textboxes.
void Sprites::draw_sprite_chopped(int x, int y, int s, int frame, int wd, int repeat_at, int alpha)
{
int xoff;

	if (wd >= sprites[s].w)
	{
		BlitSprite(x, y, s, frame, 0, 0, 0, sprites[s].w, sprites[s].h, alpha);
		return;
	}
	
	// draw the left part
	BlitSprite(x, y, s, frame, 0, 0, 0, repeat_at, sprites[s].h, alpha);
	x += repeat_at;
	wd -= repeat_at;
	
	// draw the rest of it
	xoff = (sprites[s].w - wd);
	
	BlitSprite(x, y, s, frame, 0, xoff, 0, wd, sprites[s].h, alpha);
}

// draws a sprite to any arbitrary width by repeating it over the given distance.
// if needed, the rightmost instance of the sprite is clipped.
void Sprites::draw_sprite_repeating_x(int x, int y, int s, int frame, int wd)
{
	int wdleft = wd;
	while(wdleft > 0)
	{
		int blitwd = wdleft;
		if (blitwd > sprites[s].w) blitwd = sprites[s].w;
		
		BlitSprite(x, y, s, frame, 0, 0, 0, blitwd, sprites[s].h);
		x += blitwd;
		wdleft -= blitwd;
	}
}

/*
void c------------------------------() {}
*/

// return the NXSurface for a given spritesheet #
NXSurface *Sprites::get_spritesheet(int sheetno)
{
	LoadSheetIfNeeded(sheetno);
	return spritesheet[sheetno];
}

