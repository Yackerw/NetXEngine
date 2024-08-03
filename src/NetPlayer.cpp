#include "nx.h"
#include "player.h"
#include "playerstats.h"
#include "caret.h"
#include "ObjManager.h"
#include "map.h"
#include "tsc.h"
#include "input.h"
#include "game.h"
#include "common/misc.h"
#include "ai/weapons/whimstar.h"
#include "p_arms.h"
#include "ai/sym/smoke.h"
#include "autogen/sprites.h"
#include "graphics/sprites.h"
#include "settings.h"
#include "screeneffect.h"
#include "inventory.h"
#include "NetPlayer.h"
#include "Networking.h"
#include "chat.h"
//#include "object.h"
//#include "player.h"
#include "ipfuncs.h"
#include "graphics/Renderer.h"

using namespace NXE::Graphics;

int nameevent;

char pskin;

char name[15]; //player name
char names[MAXCLIENTS][15]; //player names

Player *players = (Player*)calloc(32, sizeof(Player));
bool netpinputs[32][INPUT_COUNT];
bool netlastpinputs[32][INPUT_COUNT];

char plskins[MAXCLIENTS];

Player netInitPlayer() {
	Player p;
	memset(&p, 0, sizeof(Player));
	p.lookaway = false;
	p.walking = false;
	p.dead = false;
	p.drowned = false;
	p.disabled = false;

	p.clipx2 = 16;
	p.clipy2 = 16;
	p.clip_enable = true;
	p.nxflags = 1;

	p.frame = 0;

	p.hurt_time = 0;
	p.hurt_flash_state = 0;
	p.water_shield_frame = 0;
	p.movementmode = MOVEMODE_NORMAL;
	p.inputs_locked_lasttime = true;

	p.booststate = BOOST_OFF;
	p.lastbooststate = BOOST_OFF;
	p.boosterfuel = BOOSTER_FUEL_QTY;

	p.xinertia = 0;
	p.yinertia = 0;

	p.riding = NULL;
	p.lastriding = NULL;
	p.cannotride = NULL;

	// no
	p.DamageText = NULL;
	p.XPText = NULL;

	//p.sprite = netPSelectSprite();

	// this prevents a splash if we start underwater, and prevents us
	// from drowning immediately since our air isn't yet set up
	p.touchattr = TA_WATER;
	p.airleft = 1000;
	p.airshowtimer = 0;

	return p;
}

void netHandlePlayer(int pl) {
	// freeze player for the split-second between <TRA to a new map and the
	// start of the on-entry script for that map. (Fixes: player could shoot during
	// end sequence if he holds key down).
	// Not relevant to networked players, but potentially fixes other stuff?
	if (game.switchstage.mapno != -1)
		return;

	Player* p = &players[pl];
	netUpdateBlockstates(p);
	if (!p->dead)
	{
		netPHandleAttributes(p);			// handle special tile attributes
		netPHandleSolidMushyObjects(p);		// handle objects like bugs marked "solid / mushy"

		// Update sprite
		p->skin = plskins[pl];

		netPDoHurtFlash(p);

		switch (p->movementmode)
		{
		case MOVEMODE_NORMAL:
		{
			netPDoBooster(p);
			//netPDoBoosterEnd(p); // Not important!
			netPDoWalking(pl);
			netPDoLooking(pl);
			netPDoFalling(pl);
			netPSelectFrame(p);
		}
		break;

		case MOVEMODE_ZEROG:		// Ironhead battle/UNI 1
		{
			//netPHandleZeroG(p); // TODO: lol
		}
		break;

		default:
		{
			p->xinertia = p->yinertia = 0;
		}
		break;
		}

		// handle some special features, like damage and bouncy, of
		// 100% solid objects such as moving blocks. It's put at the end
		// so that we can see the desired inertia of the player before
		// it's canceled out by any block points that are set. That way
		// we can tell if the player is trying to move into it.
		netPHandleSolidBrickObjects(p);
	}

	// apply inertia
	netPDoPhysics(p);
}

// player aftermove routine
void netHandlePlayer_am(int pl)
{
	//debug("xinertia: %s", strhex(p->xinertia));
	//debug("yinertia: %s", strhex(p->yinertia));
	//debug("booststate: %d", p->booststate);
	//debug("y: %d", p->y / CSFI);
	//debug("riding %x", p->riding);
	//debug("block: %d%d%d%d", p->blockl, p->blockr, p->blocku, p->blockd);

	Player *p = &players[pl];

	// if player is riding some sort of platform apply it's inertia to him (don't trust this)
	//if (p->riding)
	//{
	//	p->apply_xinertia(p->riding->xinertia);
	//	p->apply_yinertia(p->riding->yinertia);
	//}

	// keep player out of blocks "SMB1 style"
	netPDoRepel(p);

	// handle landing and bonking head
	if (p->blockd && p->yinertia > 0)
	{
		if (p->yinertia > 0x400 && !p->hide)
		{
			rumble(0.3, 100);
                  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_THUD);
		}

		p->yinertia = 0;
		p->jumping = 0;
	}
	else if (p->blocku && p->yinertia < 0)
	{
		// he behaves a bit differently when bonking his head on a
		// solid-brick object vs. bonking his head on the map.

		// bonk-head star effect
		if (p->yinertia < -0x200 && !p->hide && \
			p->blocku == BLOCKED_MAP)
		{
                  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BONK_HEAD);
			rumble(0.4, 200);
			effect(p->CenterX(), p->y, EFFECT_BONKPLUS);
		}

		// bounces off ceiling with booster 0.8
		if (p->booststate == BOOST_08)
		{
			p->yinertia = 0x200;
		}
		else if (p->bopped_object && p->bopped_object->yinertia != 0)
		{
			// no clear yinertia when bop head on OBJ_BLOCK_MOVEV in labyrinth.
		}
		else
		{
			p->yinertia = 0;
		}

		p->jumping = false;
	}

	p->lastwalking = p->walking;
	p->lastriding = p->riding;
	p->inputs_locked_lasttime = p->inputs_locked;
	memcpy(netlastpinputs[pl], netpinputs[pl], sizeof(netlastpinputs[pl]));
}

void netPDoPhysics(Player *p)
{
	if (p->xinertia > 0x5ff)  p->xinertia = 0x5ff;
	if (p->xinertia < -0x5ff) p->xinertia = -0x5ff;
	if (p->yinertia > 0x5ff)  p->yinertia = 0x5ff;
	if (p->yinertia < -0x5ff) p->yinertia = -0x5ff;

	if (p->blockd && p->yinertia > 0)
		p->yinertia = 0;

	p->apply_yinertia(p->yinertia);

	// if xinertia is less than the decel speed then maintain the value but don't actually
	// move anything. It seems a bit odd...but that's the best I can figure to make it
	// behave like the original.
	if (p->xinertia > p->decelspeed || p->xinertia < -p->decelspeed)
	{
		p->apply_xinertia(p->xinertia);
	}
}

// handles tile attributes of tiles player is touching
void netPHandleAttributes(Player *p)
{
	static const Point pattrpoints[] = { { 8, 8 },{ 8, 14 } };
	static const Point hurt_bottom_attrpoint = { 8, 7 };
	unsigned int attr;
	int tile;

	// get attributes of tiles player it touching.
	// first, we'll check the top pattrpoint alone; this is the point at
	// which you go underwater, when that point is lower than the water level.
	// ** There is a spot in Labyrinth W just after the Shop where the positioning
	// of this point is a minor element in the gameplay, and so it must be set
	// correctly. If set too high you will not be underwater after climbing up the
	// small slope and you can just jump over the wall that you shouldn't be able to.
	attr = p->GetAttributes(&pattrpoints[0], 1, &tile);

	// water handler -- water uses only the top pattrpoint
	if (attr & TA_WATER)
	{
		// check if we just entered the water
		if (!(p->touchattr & TA_WATER))
		{
			// splash on entering water quick enough
			if ((p->yinertia > 0x200 && !p->blockd) || \
				(p->xinertia < -0x200 || p->xinertia > 0x200))
			{
				int x = p->CenterX();
				int y = p->CenterY();
				int splashtype = !(p->touchattr & TA_HURTS_PLAYER) ? \
					OBJ_WATER_DROPLET : OBJ_LAVA_DROPLET;

				for (int i = 0; i < 8; i++)
				{
					Object *o = CreateObject(x + (random(-8, 8) * CSFI), y, splashtype);
					o->xinertia = random(-0x200, 0x200) + p->xinertia;
					o->yinertia = random(-0x200, 0x80) - (p->yinertia >> 1);
				}

				NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SPLASH);
			}
		}

		// setup physics constants for water
		p->walkspeed = 0x196;
		p->fallspeed = 0x2ff;

		p->fallaccel = 0x28;
		p->jumpfallaccel = 0x10;

		p->walkaccel = 0x2a;
		p->jumpwalkaccel = 0x10;

		p->decelspeed = 0x19;
		// was set at 0x280 but I believe that makes it impossible to clear one of the long
		// spike jumps in River
		p->jumpvelocity = 0x280; //0x2c0;
	}
	else
	{
		// setup normal physics constants
		p->walkspeed = 0x32c;////0x030e;
		p->fallspeed = 0x5ff;

		p->fallaccel = 0x50;
		p->jumpfallaccel = 0x20;

		p->walkaccel = 0x55;
		p->jumpwalkaccel = 0x20;

		p->decelspeed = 0x33;
		p->jumpvelocity = 0x500;

		// reset air supply
		p->airleft = 1000;
		if (p->airshowtimer) p->airshowtimer--;
	}
}

// the player's block points are assymetrical--block u/d are closer together than block l/r.
// So it's quite possible to get e.g. your blockl points embedded in a wall by
// falling off the top of it. This function implements a SMB1-style "repel" that
// allows this to happen but then pushes the player out of the block over the next
// few frames.
void netPDoRepel(Player*p)
{
	// since this function is called from the aftermove, regular p->blockl etc
	// won't be updated until the following frame, so we always check the attributes
	// directly here.
	static const int REPEL_SPEED = (1 * CSFI);

	// pushes player out of walls if he become embedded in them, ala Super Mario 1.
	// this can happen for example because his R,L block points are further out than
	// his D block points so it's possible to fall really close to a block and
	// embed the R or L points further into the block than they should be
	if (p->CheckAttribute(player->repel_r, player->nrepel_r, TA_SOLID_PLAYER))
	{
		if (!p->CheckAttribute(&Renderer::getInstance()->sprites.sprites[p->sprite].block_l, TA_SOLID_PLAYER))
		{
			p->x -= REPEL_SPEED;
			//debug("REPEL [to left]");
		}
	}

	if (p->CheckAttribute(player->repel_l, player->nrepel_l, TA_SOLID_PLAYER))
	{
		if (!p->CheckAttribute(&Renderer::getInstance()->sprites.sprites[p->sprite].block_r, TA_SOLID_PLAYER))
		{
			p->x += REPEL_SPEED;
			//debug("REPEL [to right]");
		}
	}

	// vertical repel doesn't happen normally, but if we get embedded in a
	// block somehow, it can happen.
	/*
	// do repel down
	if (p->CheckAttribute(player->repel_u, player->nrepel_u, TA_SOLID_PLAYER))
	{
	if (!p->CheckAttribute(&sprites[p->sprite].block_d, TA_SOLID_PLAYER))
	{
	p->y += REPEL_SPEED;
	//debug("REPEL [down]");
	}
	}
	*/
	// do repel up
	if (p->CheckAttribute(player->repel_d, player->nrepel_d, TA_SOLID_PLAYER))
	{
		if (!p->CheckAttribute(&Renderer::getInstance()->sprites.sprites[p->sprite].block_u, TA_SOLID_PLAYER))
		{
			p->y -= REPEL_SPEED;
			//debug("REPEL [up]");
		}
	}

}

void netPHandleSolidMushyObjects(Player *p)
{
	for (int i = 0; i<nOnscreenObjects; i++)
	{
		Object *o = onscreen_objects[i];

		if (o->flags & FLAG_SOLID_MUSHY)
			netPRunSolidMushy(o,p);
	}
}

// handle "solid mushy" objects, such as bugs. These objects are solid but not 100% super
// solid like a brick. Their solidity is more of an "it repels the player" kind of way.
// NOTE: This is also responsible for the horizontal motion you see when hit by many kinds
// of enemies. The hurtplayer damage routine makes you hop vertically, but it is this that
// throws you away horizontally.
void netPRunSolidMushy(Object *o, Player* p)
{
	// cache these, so we're not calling the same functions over and over again
	const int p_left = p->SolidLeft();
	const int p_right = p->SolidRight();
	const int p_top = p->SolidTop();
	const int p_bottom = p->SolidBottom();

	const int o_left = o->SolidLeft();
	const int o_right = o->SolidRight();
	const int o_top = o->SolidTop();
	const int o_bottom = o->SolidBottom();

	static const int MUSHY_MARGIN = (3 * CSFI);
	static const int STAND_MARGIN = (1 * CSFI);
	static const int REPEL_FORCE = 0x200;

	// hitting sides of object
	if ((p_top < (o_bottom - MUSHY_MARGIN)) && (p_bottom >(o_top + MUSHY_MARGIN)))
	{
		// left side
		if ((p_right > o_left) && (p_right < o->CenterX()))
		{
			if (p->xinertia > -REPEL_FORCE)
				p->xinertia -= REPEL_FORCE;
		}

		// right side
		if ((p_left < o_right) && (p_left > o->CenterX()))
		{
			if (p->xinertia < REPEL_FORCE)
				p->xinertia += REPEL_FORCE;
		}
	}

	// bonking head on object or standing on object

	// to tell if we are within horizontal bounds to be standing on the object,
	// we will check if we have NOT FALLEN OFF the object.
	if (p_left >(o_right - STAND_MARGIN) || p_right < (o_left + STAND_MARGIN))
	{
	}
	else
	{
		// standing on object
		if (p_bottom >= o_top && p_bottom <= o->CenterY())
		{
			if (o->flags & FLAG_BOUNCY)
			{
				if (p->yinertia >(o->yinertia - 0x200))
					p->yinertia = (o->yinertia - 0x200);
			}
			else
			{
				// force to top of sprite if we're REALLY far into it
				int em_fline = o->SolidTop() + (3 * CSFI);
				if (p->SolidBottom() > em_fline)
				{
					int over_amt = (em_fline - p->SolidBottom());
					int dec_amt = (3 * CSFI);

					if (over_amt < dec_amt) dec_amt = over_amt;
					if (dec_amt < (1 * CSFI)) dec_amt = (1 * CSFI);

					p->apply_yinertia(-dec_amt);
				}

				p->blockd = true;
				p->riding = o;
			}
		}
		else if (p_top < o_bottom && p_top > o->CenterY())
		{
			// hit bottom of object with head
			if (p->yinertia < 0)
				p->yinertia = 0;
		}
	}
}

// does the invincibility flash when the player has recently been hurt
void netPDoHurtFlash(Player *p)
{
	// note that hurt_flash_state is NOT cleared when timer reaches 0,
	// but this is ok because the number of blinks are and always should be even.
	// (if not it wouldn't look right when he unhurts).
	if (p->hurt_time)
	{
		p->hurt_time--;
		p->hurt_flash_state = (p->hurt_time & 2);
	}
	else {
		// Saving this for emergencies!
		p->hurt_flash_state = 0;
	}
}

// handes player being blown around by water currents
void netDoWaterCurrents(Player *p)
{
	static Point currentpoints[] = { { 7, 8 },
	{ 1, 2 },{ 1, 8 },{ 1, 14 },
	{ 7, 2 },{ 7, 14 },
	{ 15,2 },{ 15, 8 },{ 15, 14 } };
	int i;
	static const int current_dir[] = { LEFTMASK, UPMASK, RIGHTMASK, DOWNMASK };
	uint8_t currentmask;
	int tile;

	// check each point in currentpoints[] for a water current, and if found,
	// add it to the list of directions we're being blown
	currentmask = 0;
	for (i = 0; i<9; i++)
	{
		//DebugCrosshair(p->x+(currentpoints[i].x * CSFI),p->y+(currentpoints[i].y * CSFI), 255,0,0);

		if (p->GetAttributes(&currentpoints[i], 1, &tile) & TA_CURRENT)
		{
			currentmask |= current_dir[tilecode[tile] & 3];
		}

		// if the center point (the first one) has no current, then don't
		// bother checking the rest. as during 90% of the game you are NOT underwater.
		if (!currentmask) return;
	}

	// these constants are very critical for Waterway to work properly.
	// please be careful with them.
	if (currentmask & LEFTMASK)  p->xinertia -= 0x88;
	if (currentmask & RIGHTMASK) p->xinertia += 0x88;
	if (currentmask & UPMASK)    p->yinertia -= 0x80;
	if (currentmask & DOWNMASK)  p->yinertia += 0x50;
}

void netPDoWalking(int pl)
{
	int walk_accel;
	int limit;

	Player *p = &players[pl];

	walk_accel = (p->blockd) ? p->walkaccel : p->jumpwalkaccel;

	// walking/moving
	if (netpinputs[pl][LEFTKEY] || netpinputs[pl][RIGHTKEY])
	{
		// we check both without an else so that both keys down=turn right & walk in place
		if (netpinputs[pl][LEFTKEY])
		{
			p->walking = true;
			p->dir = LEFT;

			if (p->xinertia > -p->walkspeed)
			{
				p->xinertia -= walk_accel;

				if (p->xinertia < -p->walkspeed)
					p->xinertia = -p->walkspeed;
			}
		}

		if (netpinputs[pl][RIGHTKEY])
		{
			p->walking = true;
			p->dir = RIGHT;

			if (p->xinertia < p->walkspeed)
			{
				p->xinertia += walk_accel;

				if (p->xinertia > p->walkspeed)
					p->xinertia = p->walkspeed;
			}
		}

		if (p->walking && !p->lastwalking)
			p->walkanimframe = 1;
	}
	else
	{
		p->walking = false;
		p->walkanimframe = 0;
		p->walkanimtimer = 0;
	}

	// deceleration
	if (p->blockd && p->yinertia >= 0)
	{	// deceleration on ground...
		// always move towards zero at decelspeed
		if (p->xinertia > 0)
		{
			if (p->blockr && !netpinputs[pl][RIGHTKEY])
			{
				p->xinertia = 0;
			}
			else if (p->xinertia > p->decelspeed)
			{
				p->xinertia -= p->decelspeed;
			}
			else
			{
				p->xinertia = 0;
			}
		}
		else if (p->xinertia < 0)
		{
			if (p->blockl && !netpinputs[pl][LEFTKEY])
			{
				p->xinertia = 0;
			}
			else if (p->xinertia < -p->decelspeed)
			{
				p->xinertia += p->decelspeed;
			}
			else
			{
				p->xinertia = 0;
			}
		}
	}
	else		// deceleration in air...
	{
		// implements 2 things
		//	1) if player partially hits a brick while in air, his inertia is lesser after he passes it
		//	2) but, if he's trying to turn around, let him! don't "stick" him to it just because
		//		of a high inertia when he hit it
		if (p->blockr)
		{
			limit = (p->dir == RIGHT) ? 0x180 : 0;
			if (p->xinertia > limit) p->xinertia = limit;
		}

		if (p->blockl)
		{
			limit = (p->dir == LEFT) ? -0x180 : 0;
			if (p->xinertia < limit) p->xinertia = limit;
		}
	}
}

void netPDoLooking(int pl)
{
	int lookscroll_want;
	int i, key;

	Player *p = &players[pl];
	// looking/aiming up and down
	p->look = lookscroll_want = 0;

	if (netpinputs[pl][DOWNKEY])
	{
		if (!p->blockd)
		{
			p->look = DOWN;
		}
		/*else if (!netlastpinputs[pl][DOWNKEY])
		{	// activating scripts/talking to NPC's

			if (!p->walking && !p->lookaway && \
				!netpinputs[pl][JUMPKEY] && !netpinputs[pl][FIREKEY])
			{
				if (!inputs[DEBUG_MOVE_KEY] || !settings->enable_debug_keys)
				{
					p->lookaway = true;
					p->xinertia = 0;
					PTryActivateScript();
				}
			}
		}*/ //no

		// can still scroll screen down while standing, even though
		// it doesn't show any different frame.
		lookscroll_want = DOWN;
	}

	if (netpinputs[pl][UPKEY])
	{
		p->look = lookscroll_want = UP;
	}

	// when looking, pause a second to be sure they really want to do it
	// before triggering any real screen scrolling
	if (p->lookscroll != lookscroll_want)
	{
		if (p->lookscroll_timer >= 4 || !lookscroll_want)
		{
			p->lookscroll = lookscroll_want;
		}
		else
		{
			p->lookscroll_timer++;
		}
	}
	else
	{
		p->lookscroll_timer = 0;
	}

	// deactivation of lookaway
	if (p->lookaway)
	{
		// keys which deactivate lookaway when you are facing away from player
		static const char actionkeys[] = \
		{ LEFTKEY, RIGHTKEY, UPKEY, JUMPKEY, FIREKEY, INPUT_COUNT };

		// stop looking away if any keys are pushed
		for (i = 0;; i++)
		{
			key = actionkeys[i];
			if (key == INPUT_COUNT) break;

			if (netpinputs[pl][key])
			{
				p->lookaway = false;
				break;
			}
		}

		if (!p->blockd)
			p->lookaway = false;
	}
}

void netPDoFalling(int pl)
{
	Player *p = &players[pl];
	if (p->disabled)
		return;

	if (p->booststate)
		return;

	if (game.curmap == STAGE_KINGS_TABLE && \
		fade.getstate() == FS_FADING)
		return;

	// needed to be able to see the falling blocks during
	// good-ending Helicopter cutscene (otherwise your
	// invisible character falls and the blocks spawn too low).
	if (p->hide)
	{
		p->xinertia = 0;
		p->yinertia = 0;
		return;
	}

	// use jump gravity as long as Jump Key is down and we're moving up,
	// regardless of whether a jump was ever actually initiated.
	// this is for the fans that blow up--you can push JUMP to climb higher.
	if (p->yinertia < 0 && netpinputs[pl][JUMPKEY])
	{	// use jump gravity
		if (p->yinertia < p->fallspeed)
		{
			p->yinertia += p->jumpfallaccel;
			if (p->yinertia > p->fallspeed) p->yinertia = p->fallspeed;
		}
	}
	else
	{	// use normal gravity
		if (p->yinertia < p->fallspeed)
		{
			p->yinertia += p->fallaccel;
			if (p->yinertia > p->fallspeed) p->yinertia = p->fallspeed;
		}

		// if we no longer qualify for jump gravity then the jump is over
		p->jumping = 0;
	}
}

// called every tick to run the booster
void netPDoBooster(Player *p)
{
	/*static const char *statedesc[] = { "OFF", "UP", "DN", "HOZ", "0.8" };
	debug("fuel: %d", p->boosterfuel);
	debug("booststate: %s", statedesc[p->booststate]);
	debug("xinertia: %d", p->xinertia);
	debug("yinertia: %d", p->yinertia);*/

	// We don't track the players inventory

	if (!p->booststate)
		return;

	// We also don't care about fuel. The client is always right!

	// ok so then, booster is active right now
	bool sputtering = false;

	switch (p->booststate)
	{
	case BOOST_HOZ:
	{
		if ((p->dir == LEFT && p->blockl) || \
			(p->dir == RIGHT && p->blockr))
		{
			p->yinertia = -0x100;
		}

		// this probably isn't the right way to do this, but this
		// bit makes the hurt-hop work if you get hit during a sideways boost
		//if (p->hitwhileboosting)
		//	p->yinertia = -0x400;

		if (p->dir == LEFT)  p->xinertia -= 0x20;
		if (p->dir == RIGHT) p->xinertia += 0x20;
	}
	break;

	case BOOST_UP:
	{
		p->yinertia -= 0x20;
	}
	break;

	case BOOST_DOWN:
	{
		p->yinertia += 0x20;
	}
	break;

	case BOOST_08:
	{
		// top speed and sputtering
		if (p->yinertia < -0x400)
		{
			p->yinertia += 0x20;
			sputtering = true;	// no sound/smoke this frame
		}
		else
		{
			p->yinertia -= 0x20;
		}
	}
	break;
	}

	// don't land if we booster through a one-tile high corridor,
	// but do land if we're, well, landing on something (yinertia not negative).
	// must be done after booster inertia applied to work properly.
	// for 1) there's a place in the village next to Mahin that is good for testing this,
	// for 2) the gaps in outer wall by the little house.
	if (p->blockd)
	{
		if (p->yinertia < 0)
			p->blockd = false;
		else
		{
			p->booststate = BOOST_OFF;
			return;
		}
	}

	// smoke and sound effects
	if ((p->boosterfuel % 3) == 1 && !sputtering)
	{
		netPBoosterSmokePuff(p);
	}
}

// spawn a Booster smoke puff
void netPBoosterSmokePuff(Player *p)
{
	// these are the directions the SMOKE is traveling, not the player
	//                                 RT   LT    UP    DN
	static const int smoke_xoffs[] = { 10,   4,   7,    7 };
	static const int smoke_yoffs[] = { 10,  10,   0,   14 };
	int smokedir;

	switch (p->booststate)
	{
	case BOOST_HOZ: smokedir = (p->dir ^ 1); break;
	case BOOST_UP:	smokedir = DOWN; break;
	case BOOST_DOWN:smokedir = UP; break;
	case BOOST_08:	smokedir = DOWN; break;
	default:		return;
	}

	int x = p->x + (smoke_xoffs[smokedir] * CSFI);
	int y = p->y + (smoke_yoffs[smokedir] * CSFI);

	Caret *smoke = effect(x, y, EFFECT_SMOKETRAIL_SLOW);
	smoke->MoveAtDir(smokedir, 0x200);

	NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BOOSTER);
}

// handle some special characteristics of solid-brick objects,
// such as bouncy and damage. Unlike with FLAG_SOLID_MUSHY; the
// block/l/r/u/d flags for these objects have already been set in
// UpdateBlockStates, so we don't have to worry about those.
void netPHandleSolidBrickObjects(Player *p)
{
	int i;
	SIFSprite *sprite = p->Sprite();
	Object *o;

	// calculate total inertia of player--this is needed so that
	// the forcefields in the Monster X arena will damage you if
	// the treads carry you into them.
	int p_xinertia = p->xinertia;
	int p_yinertia = p->yinertia;
	if (p->riding)
	{
		p_xinertia += p->riding->xinertia;
		p_yinertia += p->riding->yinertia;
	}

	for (i = 0; i<nOnscreenObjects; i++)
	{
		o = onscreen_objects[i];
		if (!(o->flags & FLAG_SOLID_BRICK)) continue;

		// left, right, and up contact damage
		if (o->damage > 0)
		{
			if (p->blockl && p->CheckSolidIntersect(o, &sprite->block_l))
			{
				if (p_xinertia < 0 || o->xinertia > 0)
					o->DealContactDamage();
			}

			if (p->blockr && p->CheckSolidIntersect(o, &sprite->block_r))
			{
				if (p_xinertia > 0 || o->xinertia < 0)
					o->DealContactDamage();
			}

			if (p->blocku && p->CheckSolidIntersect(o, &sprite->block_u))
			{
				if (p_yinertia < 0 || o->yinertia > 0)
					o->DealContactDamage();
			}
		}

		// stuff for when you are standing on it
		if (p->blockd && p->CheckSolidIntersect(o, &sprite->block_d))
		{
			if (o->damage && (p->yinertia >= 0 || o->yinertia < 0))
				o->DealContactDamage();

			// don't do weird glitchy shit if we jump while being carried upward
			// by an object moving faster than us. handles if you jump while flying
			// momorin's rocket.
			if (p->yinertia < 0 && o->yinertia < p->yinertia)
				p->yinertia = 0;

			// handle FLAG_BOUNCY--used eg by treads on Monster X when tipped up
			if (o->flags & FLAG_BOUNCY)
			{
				if (p->yinertia >(o->yinertia - 0x200))
					p->yinertia = (o->yinertia - 0x200);
			}
			else if (o->yinertia <= p->yinertia)
			{
				// snap his Y right on top if it
				p->y = o->SolidTop() - (Renderer::getInstance()->sprites.sprites[p->sprite].block_d[0].y * CSFI);
			}
		}
	}
}

// draws the player
void netDrawPlayer(Player *p)
{
	int scr_x, scr_y;

	p->sprite = netPSelectSprite(p);

	// lol hack
	p->hide = player->hide;

	if (p->hide || p->disabled)
		return;

	// keep his floattext position linked--do NOT update this if he is hidden
	// so that floattext doesn't follow him after he dies.
	//p->DamageText->UpdatePos(player);
	//p->XPText->UpdatePos(player);

	// get screen position to draw him at
	scr_x = (p->x / CSFI) - (map.displayed_xscroll / CSFI);
	scr_y = (p->y / CSFI) - (map.displayed_yscroll / CSFI);

	// draw his gun
        if (player->curWeapon != WPN_NONE && player->curWeapon != WPN_BLADE)
        {
          int spr, frame;
          GetSpriteForGun(player->curWeapon, player->look, &spr, &frame);

          // draw the gun at the player's Action Point. Since guns have their Draw Point set
          // to point at their handle, this places the handle in the player's hand.
          Renderer::getInstance()->sprites.drawSpriteAtDp(scr_x
                                                              + Renderer::getInstance()
                                                                    ->sprites.sprites[player->sprite]
                                                                    .frame[player->frame]
                                                                    .dir[player->dir]
                                                                    .actionpoint.x,
                                                          scr_y
                                                              + Renderer::getInstance()
                                                                    ->sprites.sprites[player->sprite]
                                                                    .frame[player->frame]
                                                                    .dir[player->dir]
                                                                    .actionpoint.y,
                                                          spr, frame, player->dir);
        }

	// draw the player sprite
	if (!p->hurt_flash_state)
	{
		Renderer::getInstance()->sprites.drawSprite(scr_x, scr_y, p->sprite, p->frame, p->dir);

		// draw the air bubble shield if we have it on
		if (((p->touchattr & TA_WATER) && (p->equipmask & EQUIP_AIRTANK)) || \
			p->movementmode == MOVEMODE_ZEROG)
		{
                  Renderer::getInstance()->sprites.drawSpriteAtDp(scr_x, scr_y, SPR_WATER_SHIELD, \
				p->water_shield_frame, p->dir);

			if (++p->water_shield_timer > 1)
			{
				p->water_shield_frame ^= 1;
				p->water_shield_timer = 0;
			}
		}
	}

	p->sprite = SPR_MYCHAR;

	if (p->equipmask & EQUIP_WHIMSTAR)
		draw_whimstars(&p->whimstar);
}

// decides which player frame to show
void netPSelectFrame(Player *p)
{
	if (p->lookaway)
	{	// looking away
		p->frame = 11;
	}
	else if (!p->blockd || p->yinertia < 0)
	{	// jumping/falling
		p->frame = (p->yinertia > 0) ? 1 : 2;
	}
	else if (p->walking)
	{	// do walk animation
		static const uint8_t pwalkanimframes[] = { 0, 1, 0, 2 };

		if (++p->walkanimtimer >= 5)
		{
			p->walkanimtimer = 0;
			if (++p->walkanimframe >= 4) p->walkanimframe = 0;
                        if (pwalkanimframes[p->walkanimframe] == 0)
                          NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_PLAYER_WALK);
		}

		p->frame = pwalkanimframes[p->walkanimframe];
	}
	else
	{	// standing
		p->frame = 0;
	}

	// switch frames to "up" or "down" versions if we're looking
	if (p->look)
	{
		if (p->look == UP)
		{
			if (!p->blockd || p->yinertia < 0)
				p->frame = 4;
			else
				p->frame += 3;
		}
		else
		{
			p->frame += 6;
		}
	}

	// mimiga mask support-- it would be better to make equipmask private,
	// and funnel all p->equipmask changes through a setter function,
	// then I'd feel safe doing this only when equipped items are changed.
	PSelectSprite();
}

void netUpdateBlockstates(Player *o) {
	o->lastblockl = o->blockl;
	o->lastblockr = o->blockr;
	o->lastblocku = o->blocku;
	o->lastblockd = o->blockd;

	o->UpdateBlockStates(ALLDIRMASK);
}

// mimiga mask support
int netPSelectSprite(Player *p)
{
	//return (p->equipmask & EQUIP_MIMIGA_MASK) ? \
	//	SPR_MYCHAR_MIMIGA : SPR_MYCHAR;
	if (p->skin == 0) {
		return SPR_MYCHAR;
	}
	else {
		return ((SPR_CURLYCHAR)-1) + p->skin;
	}
}

// returns the point that a player's shot should be centered on when firing
void netGetPlayerShootPoint(Player *p, int *x_out, int *y_out)
{
	int spr, frame;
	int x, y;

	GetSpriteForGun(p->curWeapon, p->look, &spr, &frame);

	// we have to figure out where the gun is being carried, then figure out where the
	// gun's sprite is drawn relative to that, then finally we can offset in the
	// shoot point of the gun's sprite.
	x = p->x + (Renderer::getInstance()->sprites.sprites[p->sprite].frame[p->frame].dir[p->dir].actionpoint.x * CSFI);
        x -= Renderer::getInstance()->sprites.sprites[spr].frame[frame].dir[p->dir].drawpoint.x * CSFI;
        x += Renderer::getInstance()->sprites.sprites[spr].frame[frame].dir[p->dir].actionpoint.x * CSFI;

	y = p->y
            + (Renderer::getInstance()->sprites.sprites[p->sprite].frame[p->frame].dir[p->dir].actionpoint.y * CSFI);
        y -= Renderer::getInstance()->sprites.sprites[spr].frame[frame].dir[p->dir].drawpoint.y * CSFI;
        y += Renderer::getInstance()->sprites.sprites[spr].frame[frame].dir[p->dir].actionpoint.y * CSFI;

	*x_out = x;
	*y_out = y;
}

// Bullet related stuff. So you can fire bullets online
// fire a basic, single bullet
static Object *netFireSimpleBullet(Player *p, int otype, int btype, int dir, int xoff = 0, int yoff = 0)
{
	int x, y;

	// get location to fire from
	netGetPlayerShootPoint(p, &x, &y);
	x += xoff;
	y += yoff;

	// create the shot
	Object *shot = CreateBullet(0, 0, otype);

	SetupBullet(shot, x, y, btype, dir);
	return shot;
}

// Call this to spawn bullet
static Object *netFireSimpleBulletOffset(int otype, int btype, int xoff, int yoff, int dir, Player *p)
{

	Object *shot = netFireSimpleBullet(p, otype, btype, dir);
	shot->x = xoff;
	shot->y = yoff;

	// Adjust fireball speed
	if (otype == OBJ_FIREBALL1 || otype == OBJ_FIREBALL23) {
		switch (dir) {
		case LEFT: shot->xinertia = -0x400; break;
		case RIGHT: shot->xinertia = 0x400; break;

		case UP:
			shot->xinertia = p->xinertia + ((p->dir == RIGHT) ? 128 : -128);
			if (p->xinertia) shot->dir = (p->xinertia > 0) ? RIGHT : LEFT;
			shot->yinertia = -0x5ff;
			break;

		case DOWN:
			shot->xinertia = p->xinertia;
			if (p->xinertia) shot->dir = (p->xinertia > 0) ? RIGHT : LEFT;
			shot->yinertia = 0x5ff;
			break;
		}
	}

	return shot;
}

// fires a missile type bullet at an offset from the exact center of the player
static Object *netFireMissileBullet(Player *p, int otype, int btype, int xoff = 0, int yoff = 0, int accel = 0, bool wiggle = false, int truex = 0, int truey = 0, int dir = 0)
{
	int x, y;

	// create the shot
	Object *shot = CreateBullet(0, 0, otype);

	//for shot star effect
	netGetPlayerShootPoint(p, &x, &y);
	SetupBullet(shot, x, y, btype, dir);
	shot->x = truex;
	shot->y = truex;

	shot->SetCenterX(x + xoff);
	shot->SetCenterY(y + yoff);

	if (p->look)
	{
		shot->yinertia = random(-512, 512);
		if (wiggle) shot->xinertia = (shot->x <= p->x) ? -256 : 256;
	}
	else
	{
		shot->xinertia = random(-512, 512);
		if (wiggle) shot->yinertia = (shot->y <= p->y) ? -256 : 256;
	}
	shot->shot.accel = accel;
	return shot;
}

// For some reason, blade gets special treatment
static void netPFireBlade(Player *p, int level)
{
	int dir = (p->look) ? p->look : p->dir;

	int x = p->CenterX();
	int y = p->CenterY();

	if (level == 2)
	{
		if (dir == RIGHT || dir == LEFT)
		{
			y -= (3 * CSFI);
			x += (dir == LEFT) ? (3 * CSFI) : -(3 * CSFI);
		}
	}
	else
	{
		switch (dir)
		{
		case RIGHT: x -= (6 * CSFI); y -= (3 * CSFI); break;
		case LEFT:  x += (6 * CSFI); y -= (3 * CSFI); break;
		case UP:    y += (6 * CSFI); break;
		case DOWN:  y -= (6 * CSFI); break;
		}
	}

	Object *shot = CreateObject(x, y, (level != 2) ? OBJ_BLADE12_SHOT : OBJ_BLADE3_SHOT);
	SetupBullet(shot, x, y, B_BLADE_L1 + level, dir);
}

int cnnbuffsize = (sizeof(char)*(MAXCLIENTS * 2)) + (sizeof(int) * (4 + MAX_INVENTORY + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((MAXCLIENTS + 1) * 15));

// Generic on-connect function to let you know number of players in server and what sockets
char *ConnectSend() {
	char *buff = (char*)calloc(cnnbuffsize, 1);
	int i = 0;
	int foundempty = false;
	while (i < MAXCLIENTS) {
		buff[i] = clients[i].used;
		if (buff[i] == false && foundempty == false) {
			players[i] = netInitPlayer();
			foundempty = true;
		}
		i++;
	}
	// also give us the current level
	memcpy(buff + MAXCLIENTS, &game.curmap, sizeof(int));
	memcpy(buff + MAXCLIENTS + sizeof(int), &(player->inventory), MAX_INVENTORY * sizeof(int));
	memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 1)), &(player->ninventory), sizeof(int));
	memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)), &(player->weapons), sizeof(Weapon) * WPN_COUNT);
	memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT), &game.flags, NUM_GAMEFLAGS);
	memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, &(player->maxHealth), sizeof(int));

	for (i = 0; i<NUM_TELEPORTER_SLOTS; i++)
	{
		int slotno, scriptno;
		if (!textbox.StageSelect.GetSlotByIndex(i, &slotno, &scriptno))
		//textbox.StageSelect.GetSlotByIndex(i, &slotno, &scriptno);
		{
			memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 3)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), &slotno, sizeof(int)); 
			memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 4)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), &scriptno, sizeof(int));
		}
	}
	// Copy player skins
	i = 0;
	while (i < MAXCLIENTS) {
		memcpy(buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + i, &players[i].skin, sizeof(char));
		i++;
	}
	// Our skin
	memcpy(buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, &(player->skin), sizeof(char));
	// Copy player names
	i = 0;
	while (i < MAXCLIENTS) {
		memcpy(buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + 1 + (i * 15), names[i], sizeof(char) * 15);
		i++;
	}
	// your name
	memcpy(buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + 1 + (MAXCLIENTS * 15), name, sizeof(char) * 15);
	return buff;
}

void ConnectRecv(char *buff) {
	int i = 0;
	bool foundclient = false;
	while (i < MAXCLIENTS) {
		clients[i].used = buff[i];
		if (clients[i].used == true) {
			players[i] = netInitPlayer();
		}
		i++;
	}
	// change map
	memcpy(&game.switchstage.mapno, buff + MAXCLIENTS, sizeof(int));
	memcpy(&(player->inventory), buff + MAXCLIENTS + sizeof(int), sizeof(int) * MAX_INVENTORY);
	memcpy(&(player->ninventory), buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 1)), sizeof(int));
	memcpy(&(player->weapons), buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)), sizeof(Weapon) * WPN_COUNT);
	memcpy(&game.flags, buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT), NUM_GAMEFLAGS);
	memcpy(&(player->maxHealth), buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, sizeof(int));
	for (i = 0; i<NUM_TELEPORTER_SLOTS; i++)
	{
		int slotno, scriptno;
		memcpy(&slotno, buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 3)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), sizeof(int));
		memcpy(&scriptno, buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 4)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + ((i * 2) * sizeof(int)), sizeof(int));
		if (slotno != 0 && scriptno != 0) {
			textbox.StageSelect.SetSlot(slotno, scriptno);
		}
	}
	// Skins
	i = 0;
	while (i < MAXCLIENTS) {
		memcpy(&plskins[i], buff + MAXCLIENTS + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + i, sizeof(char));
		i++;
	}
	memcpy(&plskins[ClientNode], buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS, sizeof(char));
	int tmp = (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS;
	// Names
	i = 0;
	while (i < MAXCLIENTS) {
		memcpy(names[i], buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + 1 + (i * 15), sizeof(char) * 15);
		i++;
	}
	memcpy(names[ClientNode], buff + (MAXCLIENTS * 2) + (sizeof(int) * (MAX_INVENTORY + 4) + (NUM_TELEPORTER_SLOTS * 2)) + (sizeof(Weapon) * WPN_COUNT) + NUM_GAMEFLAGS + 1 + (MAXCLIENTS * 15), sizeof(char) * 15);
	player->invisible = false;
	player->movementmode = MOVEMODE_NORMAL;
	player->hide = false;
	player->hp = player->maxHealth; // fade
	fade.set_full(1);
	game.setmode(GM_NORMAL);
	// update player inventory
	for (i = 0; i < player->ninventory; i++) {
		switch (player->inventory[i]) {
		case ITEM_TURBOCHARGE:
			player->equipmask |= EQUIP_TURBOCHARGE;
			break;
		case ITEM_BOOSTER08:
			if (!player->equipmask & EQUIP_BOOSTER20) \
				player->equipmask |= EQUIP_BOOSTER08;
			break;
		case ITEM_BOOSTER20:
			player->equipmask |= EQUIP_BOOSTER20;
			//player->equipmask &= !EQUIP_BOOSTER08;
			break;
		case ITEM_AIRTANK:
			player->equipmask |= EQUIP_AIRTANK;
			break;
		case ITEM_ARMS_BARRIER:
			player->equipmask |= EQUIP_ARMS_BARRIER;
			break;
		}
	}
}

char *SyncPositionSend() {
	PlayerStepSync *s = (PlayerStepSync*)malloc(sizeof(PlayerStepSync));
	s->x = player->x;
	s->y = player->y;
	s->xinertia = player->xinertia;
	s->yinertia = player->yinertia;
	s->curweapon = player->curWeapon;
	s->dir = player->dir;
	memcpy(&(s->inputs), pinputs, INPUT_COUNT);
	return (char*)s;
}

void SyncPositionRecv(unsigned char *buff, int pl) {
	PlayerStepSync *s = (PlayerStepSync*)buff;
	Player *p = &players[pl];
	p->x = s->x;
	p->y = s->y;
	p->xinertia = s->xinertia;
	p->yinertia = s->yinertia;
	p->curWeapon = s->curweapon;
	p->dir = s->dir;
	memcpy(netpinputs[pl], &s->inputs, INPUT_COUNT);
}

char *ConnectOthers(int joiner) {
	char *lol = (char*)malloc(sizeof(int));
	memcpy(lol, &joiner, sizeof(int));
	return lol;
}

void ConnectOthersRecv(char *buff) {
	int node;
	memcpy(&node, buff, sizeof(int));
	clients[node].used = 1;
	players[node] = netInitPlayer();
}

// Sync bullet shots
char *BulletSpawnSend() {
	char *BulletSpawnSend = (char*)malloc(sizeof(NetBullet));
	memcpy(BulletSpawnSend, &SyncBull, sizeof(NetBullet));
	return BulletSpawnSend;
}

void BulletSpawnRecv(unsigned char *buff, int pnum) {
	// Reserve 256 objects for the game to use
	if (NumObjects > MAX_OBJECTS - 256) {
		return;
	}
	Player *p = &players[pnum];
	NetBullet *b = (NetBullet*)buff;
	// return if otype or btype aren't valid (i.e. keep people from spawning anything they want)
	if (b->btype >= B_CURLYS_NEMESIS || b->btype < 0 || b->otype < OBJ_SHOTS_START || b->otype > OBJ_SPUR_TRAIL) {
		return;
	}
	netFireSimpleBulletOffset(b->otype, b->btype, b->x, b->y, b->dir, p);
}

char *MissileSpawnSend() {
	char *BulletSpawnSend = (char*)malloc(sizeof(NetBullet));
	memcpy(BulletSpawnSend, &SyncBull, sizeof(NetBullet));
	return BulletSpawnSend;
}

void MissileSpawnRecv(unsigned char *buff, int pnum) {
	// Reserve 256 objects for the game to use
	if (NumObjects > MAX_OBJECTS - 256) {
		return;
	}
	Player *p = &players[pnum];
	NetBullet b;
	memcpy(&b, buff, sizeof(NetBullet));
	netFireMissileBullet(p, b.otype, b.btype, b.xoff, b.yoff, b.accel, b.wiggle, b.x, b.y, b.dir);
}

char *BladeSpawnSend() {
	char *BulletSpawnSend = (char*)malloc(sizeof(char));
	memcpy(BulletSpawnSend, &(player->weapons[player->curWeapon].level), sizeof(char));
	return BulletSpawnSend;
}

void BladeSpawnRecv(unsigned char *buff, int pnum) {
	// Reserve 256 objects for the game to use
	if (NumObjects > MAX_OBJECTS - 256) {
		return;
	}
	Player *p = &(players[pnum]);
	netPFireBlade(p, buff[0]);
}

char *DiscnnSend(int pnum) {
	char *outbuff = (char*)malloc(sizeof(int));
	memcpy(outbuff, &pnum, sizeof(int));
	return outbuff;
}

void DiscnnRecv(char *buff) {
	int pnum;
	memcpy(&pnum, buff, sizeof(int));
	clients[pnum].used = false;
	char msg[128];
	sprintf(msg, "%s has disconnected", names[pnum]);
	Chat_SetMessage(msg, 1);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_COMPUTER_BEEP);
	Chat_WriteToLog(msg);
	chatstate.timer = (60 * 5);
}

char *SkinSend() {
	char *outbuff = (char*)malloc(sizeof(char));
	memcpy(outbuff, &player->skin, sizeof(char));
	return outbuff;
}

void SkinRecv(unsigned char *buff, int p) {
	if (buff[0] >= 0 && buff[0] < truenumskins) {
		memcpy(&plskins[p], buff, sizeof(char));
	}
}

int PlayerUpdateEvent;
int PlayerShotEvent;
int PlayerMissileEvent;
int PlayerBladeEvent;
int PlayerSkinUpdateEvent;
int PlayerNameUpdateEvent;

char* Name_Send() {
	char* sendname = (char*)malloc(15);
	strcpy(sendname, name);
	return sendname;
}

void Name_Receive(unsigned char* tempname, int node) {
	tempname[14] = 0;
	strcpy(names[node], (char*)tempname);

	//say when a player joins the game!
	char joingamemsg[38];
	joingamemsg[0] = 0;
	strcat((char*)&joingamemsg, names[node]);
	strcat((char*)&joingamemsg, " joined the game.");
	Chat_SetMessage(joingamemsg, 1);
	Chat_WriteToLog(joingamemsg);
	chatstate.timer = (60 * 5);
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GET_MISSILE);
	if (Host == 1) {
		char IP[32];
		InetNtopA(clients[node].info.sin_family, &clients[node].info.sin_addr, IP, 32);
		Chat_WriteToLog(IP);
	}
}

// Special function for machine gun shooting
char *NetMGunShotSend() {
	char *out = (char*)malloc(sizeof(NetBullet));
	memcpy(out, &SyncBull, sizeof(NetBullet));
	return out;
}

void NetMGunShotRecv(unsigned char *in, int pnode) {
	// Reserve 256 objects for the game to use
	if (NumObjects > MAX_OBJECTS - 256) {
		return;
	}
	NetBullet *b = (NetBullet*)in;
	if (b->otype > 2 || b->otype < 0) {
		return;
	}
	int x, y;
	netGetPlayerShootPoint(&players[pnode], &x, &y);
	FireLevel23MGun(b->x, b->y, b->otype, b->dir);
}

int MGunEvent;

void SetupNetPlayerFuncs() {
	Net_RegisterConnectEventSvSend(ConnectSend, cnnbuffsize);
	Net_RegisterConnectEventSvRecv(ConnectRecv);
	PlayerUpdateEvent = Net_RegisterPlayerEventSend(SyncPositionSend, sizeof(PlayerStepSync), 0);
	Net_RegisterPlayerEventRecv(SyncPositionRecv, (sizeof(int) * 5) + 1 + INPUT_COUNT);
	Net_RegisterConnectEventOthersSend(ConnectOthers, sizeof(int));
	Net_RegisterConnectEventOthersRecv(ConnectOthersRecv);
	PlayerShotEvent = Net_RegisterPlayerEventSend(BulletSpawnSend, sizeof(NetBullet));
	Net_RegisterPlayerEventRecv(BulletSpawnRecv, sizeof(NetBullet));
	PlayerMissileEvent = Net_RegisterPlayerEventSend(MissileSpawnSend, sizeof(NetBullet));
	Net_RegisterPlayerEventRecv(MissileSpawnRecv, sizeof(NetBullet));
	PlayerBladeEvent = Net_RegisterPlayerEventSend(BladeSpawnSend, sizeof(char));
	Net_RegisterPlayerEventRecv(BladeSpawnRecv, sizeof(char));
	Net_RegisterDisconnectSend(DiscnnSend, sizeof(int));
	Net_RegisterDisconnectRecv(DiscnnRecv);
	PlayerSkinUpdateEvent = Net_RegisterPlayerEventSend(SkinSend, sizeof(char));
	Net_RegisterPlayerEventRecv(SkinRecv, sizeof(char));

	nameevent = Net_RegisterPlayerEventSend(Name_Send, 15);
	Net_RegisterPlayerEventRecv(Name_Receive, 15);

	MGunEvent = Net_RegisterPlayerEventSend(NetMGunShotSend, sizeof(NetBullet));
	Net_RegisterPlayerEventRecv(NetMGunShotRecv, sizeof(NetBullet));
}
