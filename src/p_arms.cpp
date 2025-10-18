
#include "p_arms.h"

#include "ObjManager.h"
#include "ai/weapons/whimstar.h"
#include "autogen/sprites.h"
#include "caret.h"
#include "common/misc.h"
#include "console.h"
#include "game.h"
#include "input.h"
#include "nx.h"
#include "player.h"
#include "playerstats.h"
#include "sound/SoundManager.h"
#include "statusbar.h"
#include "input.h"
#include "ai/weapons/whimstar.h"
#include "common/misc.h"
#include "NetPlayer.h"

#include "autogen/sprites.h"
#include "game.h"
#include "console.h"

IntRegistry weaponRegistry;

NetBullet SyncBull;

static int empty_timer = 0;

struct BulletInfo
{
  int sprite;            // sprite to use
  int level;             // specify what level weapon is at when it fires this shot type
  int frame;             // specify which frame within sprite
  uint8_t makes_star;    // 1=make star effect, 2=make star but add x inertia to position
  int timetolive;        // shot range
  int damage;            // damage dealt per tick of contact with enemy
  int speed;             // speed of shot
  uint8_t manualsetup;   // 1= no auto setup at all, 2= don't use separate vert sprite
  NXE::Sound::SFX sound; // specify firing sound
};

BulletInfo bullet_table[] = {
    //		sprite			  lvl  frm st ttl dmg spd  manset      sound
    {SPR_SHOT_POLARSTAR, 0, 0, 1, 8, 1, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2},   // polarstar l1
    {SPR_SHOT_POLARSTAR, 1, 1, 1, 12, 2, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2},  // polarstar l2
    {SPR_SHOT_POLARSTAR_L3, 2, 0, 1, 16, 4, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L3}, // polarstar l3

    {SPR_SHOT_MGUN_L1, 0, 0, 1, 20, 2, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // mgun l1

    {SPR_SHOT_MGUN_L2, 1, 0, 1, 20, 4, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // mgun l2, white piece
    {SPR_SHOT_MGUN_L2, 1, 1, 0, 21, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},            // mgun l2, blue piece
    {SPR_SHOT_MGUN_L2, 1, 2, 0, 22, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},            // mgun l2, dark piece

    {SPR_SHOT_MGUN_L3LEAD, 2, 0, 1, 20, 6, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L3}, // mgun l3
    {SPR_SHOT_MGUN_L3TAIL, 2, 0, 0, 21, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},          // the very long...
    {SPR_SHOT_MGUN_L3TAIL, 2, 1, 0, 22, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},          // ...4 piece trail...
    {SPR_SHOT_MGUN_L3TAIL, 2, 2, 0, 23, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},          // ...of the level 3...
    {SPR_SHOT_MGUN_L3TAIL, 2, 3, 0, 24, 0, 0x1000, 0, NXE::Sound::SFX::SND_NULL},          // ...machine gun

    // damage for missiles is set inside missile.cpp
    {SPR_SHOT_MISSILE1, 0, 0, 1, 50, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // missile level 1
    {SPR_SHOT_MISSILE2, 1, 0, 1, 65, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // missile level 2
    {SPR_SHOT_MISSILE3, 2, 0, 1, 90, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // missile level 3

    {SPR_SHOT_SUPERMISSILE13, 0, 0, 1, 30, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // supermissile l1
    {SPR_SHOT_SUPERMISSILE2, 1, 0, 1, 40, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2},  // supermissile l2
    {SPR_SHOT_SUPERMISSILE13, 2, 0, 1, 40, 0, 0x0000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L1_2}, // supermissile l3

    // damages are doubled because fireball can hit twice before dissipating
    {SPR_SHOT_FIREBALL1, 0, 0, 1, 100, 2, 0x0000, 1, NXE::Sound::SFX::SND_FIREBALL},  // fireball l1
    {SPR_SHOT_FIREBALL23, 1, 0, 1, 100, 3, 0x0000, 1, NXE::Sound::SFX::SND_FIREBALL}, // fireball l2
    {SPR_SHOT_FIREBALL23, 2, 0, 1, 100, 3, 0x0000, 1, NXE::Sound::SFX::SND_FIREBALL}, // fireball l3

    {SPR_SHOT_BLADE_L1, 0, 0, 0, 29, 15, 0x800, 0, NXE::Sound::SFX::SND_FIREBALL}, // Blade L1
    {SPR_SHOT_BLADE_L2, 1, 0, 0, 17, 6, 0x800, 0, NXE::Sound::SFX::SND_FIREBALL},  // Blade L2
    {SPR_SHOT_BLADE_L3, 2, 0, 0, 30, 1, 0x800, 0, NXE::Sound::SFX::SND_FIREBALL},  // Blade L3

    {SPR_SHOT_SNAKE_L1, 0, 0, 1, 20, 4, 0x600, 2, NXE::Sound::SFX::SND_SNAKE_FIRE},   // Snake L1
    {SPR_SHOT_FIREBALL23, 1, 0, 1, 23, 6, 0x200, 2, NXE::Sound::SFX::SND_SNAKE_FIRE}, // Snake L2
    {SPR_SHOT_FIREBALL23, 2, 0, 1, 30, 8, 0x200, 2, NXE::Sound::SFX::SND_SNAKE_FIRE}, // Snake L3

    {SPR_SHOT_NEMESIS_L1, 0, 0, 2, 20, 4, 0x1000, 0, NXE::Sound::SFX::SND_NEMESIS_FIRE},
    {SPR_SHOT_NEMESIS_L2, 1, 0, 2, 20, 4, 0x1000, 0, NXE::Sound::SFX::SND_POLAR_STAR_L3},
    {SPR_SHOT_NEMESIS_L3, 2, 0, 2, 20, 1, 0x555, 0, NXE::Sound::SFX::SND_SPUR_CHARGE_2}, // 1/3 speed

    {SPR_SHOT_BUBBLER_L1, 0, 0, 1, 40, 1, 0x600, 2, NXE::Sound::SFX::SND_BUBBLER_FIRE},
    {SPR_SHOT_BUBBLER_L2, 1, 0, 1, 60, 2, 0x600, 2, NXE::Sound::SFX::SND_BUBBLER_FIRE},
    {SPR_SHOT_BUBBLER_L3, 2, 0, 1, 100, 2, 0x600, 2, NXE::Sound::SFX::SND_BUBBLER_FIRE},

    // Spur also messes with it's damage at runtime; see spur.cpp for details.
    {SPR_SHOT_POLARSTAR, 0, 0, 1, 30, 4, 0x1000, 0, NXE::Sound::SFX::SND_SPUR_FIRE_1},
    {SPR_SHOT_POLARSTAR, 1, 1, 1, 30, 8, 0x1000, 0, NXE::Sound::SFX::SND_SPUR_FIRE_2},
    {SPR_SHOT_POLARSTAR_L3, 2, 0, 0, 30, 12, 0x1000, 0, NXE::Sound::SFX::SND_SPUR_FIRE_3},

    // Curly's Nemesis from Hell (OBJ_CURLY_CARRIED_SHOOTING)
    {SPR_SHOT_NEMESIS_L1, 0, 0, 1, 20, 4, 0x1000, 0, NXE::Sound::SFX::SND_NEMESIS_FIRE},

    {0, 0, 0, 0, 0, 0, 0, 0, NXE::Sound::SFX::SND_NULL}};

// resets weapons on player re-init (Player::Init)
void PResetWeapons()
{
  //player->weapons[WPN_SPUR].resetSpur = true;
  init_whimstar(&player->whimstar);
}

bool Spur::canFire(void)
{
  if (Objects::CountLocalBullets(OBJ_SPUR_SHOT))
    return false;

  return true;
}

// returns true if the current weapon has full xp at level 3 (is showing "Max")
bool Weapon::isWeaponMaxed(void)
{
  return (level == getMaxLevel()) && (xp == getMaxXP(level));
}

void Weapon::initializeWeapons() {
  weaponRegistry.clear();
  // weapons need sorting properly
  weaponRegistry.registerType(NULL);
  weaponRegistry.registerType((registryValue)&Snake::create);
  weaponRegistry.registerType((registryValue)&PolarStar::create);
  weaponRegistry.registerType((registryValue)&Fireball::create);
  weaponRegistry.registerType((registryValue)&MachineGun::create);
  weaponRegistry.registerType((registryValue)&MissileLauncher::create);
  weaponRegistry.registerType(NULL);
  weaponRegistry.registerType((registryValue)&Bubbler::create);
  weaponRegistry.registerType(NULL);
  weaponRegistry.registerType((registryValue)&Blade::create);
  weaponRegistry.registerType((registryValue)&SuperMissileLauncher::create);
  weaponRegistry.registerType(NULL);
  weaponRegistry.registerType((registryValue)&Nemesis::create);
  weaponRegistry.registerType((registryValue)&Spur::create);
}

// fire a basic, single bullet
static WeaponBullet *FireSimpleBullet(int otype, int btype, int xoff = 0, int yoff = 0)
{
  int x, y, dir;

  // get location to fire from
  GetPlayerShootPoint(&x, &y);
  x += xoff;
  y += yoff;

  // create the shot
  WeaponBullet *shot = CreateBullet(0, 0, otype);

  // set up the shot
  if (player->look)
    dir = player->look;
  else
    dir = player->dir;

  SetupBullet(shot, x, y, btype, dir);
  return shot;
}

// fires a missile type bullet at an offset from the exact center of the player
static WeaponBullet *FireMissileBullet(int otype, int btype, int xoff = 0, int yoff = 0, int accel = 0, bool wiggle = false)
{
  int x, y, dir;

  // create the shot
  WeaponBullet *shot = CreateBullet(0, 0, otype);

  // set up the shot
  if (player->look)
    dir = player->look;
  else
    dir = player->dir;

  // for shot star effect
  GetPlayerShootPoint(&x, &y);
  SetupBullet(shot, x, y, btype, dir);

  x = player->CenterX();
  y = player->CenterY(); // + 4*CSFI;

  shot->SetCenterX(x + xoff);
  shot->SetCenterY(y + yoff);

  if (player->look)
  {
    shot->yinertia = random(-512, 512);
    if (wiggle)
      shot->xinertia = (shot->x <= player->x) ? -256 : 256;
  }
  else
  {
    shot->xinertia = random(-512, 512);
    if (wiggle)
      shot->yinertia = (shot->y <= player->y) ? -256 : 256;
  }
  shot->shot.accel = accel;
  	SyncBull.x = shot->x;
	SyncBull.y = shot->y;
	SyncBull.wiggle = wiggle;
	SyncBull.btype = btype;
	SyncBull.otype = otype;
	SyncBull.dir = dir;
	SyncBull.xoff = xoff;
	SyncBull.yoff = yoff;
	SyncBull.accel = accel;
	Net_FirePlayerEvent(PlayerMissileEvent);
  return shot;
}

// fires a bullet at an offset from the exact center of the player's shoot point.
// FireSimpleBullet can do this too-- but it's xoff/yoff is absolute. This function
// takes a parameter for when you are shooting right and extrapolates out the other
// directions from that. ALSO, xoff/yoff on FireSimpleBullet moves the star;
// this function does not.
static WeaponBullet *FireSimpleBulletOffset(int otype, int btype, int xoff, int yoff)
{
  int dir;
  if (player->look)
    dir = player->look;
  else
    dir = player->dir;

  switch (dir)
  {
    case RIGHT:
      break; // already in format for RIGHT frame
    case LEFT:
      xoff = -xoff;
      break;
    case UP:
      SWAP(xoff, yoff);
      yoff = -yoff;
      break;
    case DOWN:
      SWAP(xoff, yoff);
      break;
  }

  WeaponBullet *shot = FireSimpleBullet(otype, btype);
  shot->x += xoff;
  shot->y += yoff;
  	SyncBull.x = shot->x;
	SyncBull.y = shot->y;
	SyncBull.otype = otype;
	SyncBull.btype = btype;
	SyncBull.dir = dir;
	Net_FirePlayerEvent(PlayerShotEvent);

  return shot;
}

// fires and handles charged shots
void Spur::updateWeapon(void)
{
  static const int FLASH_TIME = 10;
  Weapon *spur = this;

  if (pinputs[FIREKEY])
  {
    if (!isWeaponMaxed())
    {
      int amt = (player->equipmask & EQUIP_TURBOCHARGE) ? 3 : 2;
      addXP(amt, true);

      if (isWeaponMaxed())
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_SPUR_MAXED);
      }
      else if (++spur->chargetimer & 2)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(
            (NXE::Sound::SFX)((int)NXE::Sound::SFX::SND_SPUR_CHARGE_1 + spur->level));
      }
    }
    else
    {
      // keep flashing even once at max
      int amt = (player->equipmask & EQUIP_TURBOCHARGE) ? 3 : 2;
      addXP(amt, true);
    }
  }
  else
  {
    if (spur->level > 0 && canFire())
    {
      int level = isWeaponMaxed() ? 2 : (spur->level - 1);
      FireSimpleBulletOffset(OBJ_SPUR_SHOT, B_SPUR_L1 + level, -4 * CSFI, 0);
    }

    spur->level = 0;
    spur->xp    = 0;
  }

  if (statusbar.xpflashcount > FLASH_TIME)
    statusbar.xpflashcount = FLASH_TIME;
}

void PDoWeapons(void)
{
  run_whimstar(&player->whimstar);

  if (player->inputs_locked)
    return; // should prevent from firing in cutscenes

  bool prev = justpushed(PREVWPNKEY), next = justpushed(NEXTWPNKEY);
  int wpn = player->curWeapon;
  if (prev)
    stat_PrevWeapon();
  if (next)
    stat_NextWeapon();
  int cwpn = player->curWeapon;

  Weapon *spur = NULL; // player->getWeapon(WPN_SPUR);

  Weapon *currWep = player->FindWeapon(player->curWeapon);

  // this kinda sucks and should just be changed to make the spur set its level and xp to 0 upon switch out (or switch on)
  // according to below there's some intricacies to how this should work on the inventory screen, but like...what? that's so specific and not gameplay affecting
  /*if (spur)
  {
    bool &resetSpur = spur->resetSpur;

    // 1. When changing weapons using the previous/next weapon keys ...
    // 2. Spur level should be reset when switching from it to another weapon
    // 3. ... or when switching to it when fire button is not pressed.
    // 4. When using the item screen, Spur level should be reset only when ...
    // 5. it's the current weapon, the fire button is not pressed
    // 6. ... and we switched recently between some non-Spur weapons without using the menu.
    //   (     1.     )     (      2.     )    (                  3.                 )
    if (((prev || next) && (wpn == WPN_SPUR || (cwpn == WPN_SPUR && !pinputs[FIREKEY])))
        //  (             4.             )    (                5.                )    (   6.  )
        || (player->inputs_locked_lasttime && wpn == WPN_SPUR && !pinputs[FIREKEY] && resetSpur))
    {
      spur->level = 0;
      spur->xp    = 0;
      resetSpur = false;
    }
    if (prev || next || wpn == WPN_SPUR)
      resetSpur = wpn != WPN_SPUR && cwpn != WPN_SPUR;
  }*/

  // firing weapon
  if (currWep != NULL) {
    if (pinputs[FIREKEY])
    {
      currWep->handleFiring();
      currWep->runWeapon(true);
    }
    else
    {
      player->auto_fire_limit = 6;
      currWep->runWeapon(false);
    }
  }

  //PHandleSpur();

  if (empty_timer)
    empty_timer--;
}

/*
void c------------------------------() {}
*/

// fire the missile launcher.
// level: 0 - 2: weapon level from 1 - 3
// is_super: bool: true if the player is firing the Super Missile Launcher
WeaponBullet* MissileLauncher::fire()
{
  int xoff, yoff;

  bool is_super = false;

  int object_type = (!is_super) ? OBJ_MISSILE_SHOT : OBJ_SUPERMISSILE_SHOT;

  // can only fire one missile at once on L1,
  // two missiles on L2, and two sets of three missiles on L3.
  static const uint8_t max_missiles_at_once[] = {1, 2, 6};
  if (Objects::CountLocalBullets(object_type) >= max_missiles_at_once[level])
  {
    // give back the previously-decremented ammo so they don't lose it (hack)
    player->FindWeapon(player->curWeapon)->ammo++;
    return NULL;
  }

  int bullet_type = (!is_super) ? B_MISSILE_L1 : B_SUPER_MISSILE_L1;
  bullet_type += level;

  // level 1 & 2 fires just one missile
  yoff = 1;
  xoff = (player->dir == RIGHT) ? 1 : -1;
  if (player->look)
  {
    yoff = (player->look == UP) ? -1 : 1;
    FireMissileBullet(object_type, bullet_type, CSFI * xoff, 8 * CSFI * yoff, (is_super) ? 512 : 128, (level == 2));
  }
  else
  {
    FireMissileBullet(object_type, bullet_type, 6 * CSFI * xoff, (level == 2) ? CSFI : 0, (is_super) ? 512 : 128,
                      (level == 2));
  }
  // lv3 fires 3 missiles that wiggle
  if (level == 2)
  {
    if (player->look)
    {
      yoff = (player->look == UP) ? -1 : 1;
      FireMissileBullet(object_type, bullet_type, 3 * CSFI * xoff, 0, (is_super) ? 256 : 64, true);
      FireMissileBullet(object_type, bullet_type, -3 * CSFI * xoff, 0, (is_super) ? 170 : 51, true);
    }
    else
    {
      FireMissileBullet(object_type, bullet_type, 0, -8 * CSFI, (is_super) ? 256 : 64, true);
      FireMissileBullet(object_type, bullet_type, -4 * CSFI * xoff, -CSFI, (is_super) ? 170 : 51, true);
    }
  }
}

// this kinda sucks but i'll fix it l8r
WeaponBullet *SuperMissileLauncher::fire()
{
  WeaponBullet *ret = NULL;
  int xoff, yoff;

  bool is_super = true;

  int object_type = (!is_super) ? OBJ_MISSILE_SHOT : OBJ_SUPERMISSILE_SHOT;

  // can only fire one missile at once on L1,
  // two missiles on L2, and two sets of three missiles on L3.
  static const uint8_t max_missiles_at_once[] = { 1, 2, 6 };
  if (Objects::CountLocalBullets(object_type) >= max_missiles_at_once[level])
  {
    // give back the previously-decremented ammo so they don't lose it (hack)
    player->FindWeapon(player->curWeapon)->ammo++;
    return NULL;
  }

  int bullet_type = (!is_super) ? B_MISSILE_L1 : B_SUPER_MISSILE_L1;
  bullet_type += level;

  // level 1 & 2 fires just one missile
  yoff = 1;
  xoff = (player->dir == RIGHT) ? 1 : -1;
  if (player->look)
  {
    yoff = (player->look == UP) ? -1 : 1;
    ret = FireMissileBullet(object_type, bullet_type, CSFI * xoff, 8 * CSFI * yoff, (is_super) ? 512 : 128, (level == 2));
  }
  else
  {
    ret = FireMissileBullet(object_type, bullet_type, 6 * CSFI * xoff, (level == 2) ? CSFI : 0, (is_super) ? 512 : 128,
      (level == 2));
  }
  // lv3 fires 3 missiles that wiggle
  if (level == 2)
  {
    if (player->look)
    {
      yoff = (player->look == UP) ? -1 : 1;
      ret = FireMissileBullet(object_type, bullet_type, 3 * CSFI * xoff, 0, (is_super) ? 256 : 64, true);
      ret = FireMissileBullet(object_type, bullet_type, -3 * CSFI * xoff, 0, (is_super) ? 170 : 51, true);
    }
    else
    {
      ret = FireMissileBullet(object_type, bullet_type, 0, -8 * CSFI, (is_super) ? 256 : 64, true);
      ret = FireMissileBullet(object_type, bullet_type, -4 * CSFI * xoff, -CSFI, (is_super) ? 170 : 51, true);
    }
  }
  return ret;
}

/*
void c------------------------------() {}
*/

WeaponBullet *Fireball::fire()
{
  static const int object_types[] = {OBJ_FIREBALL1, OBJ_FIREBALL23, OBJ_FIREBALL23};
  static uint8_t max_fireballs[]  = {2, 3, 4};
  int count;

  count = (Objects::CountLocalBullets(OBJ_FIREBALL1) + Objects::CountLocalBullets(OBJ_FIREBALL23));
  if (count >= max_fireballs[level])
    return NULL;

  // the 8px offset fires the shot just a tiny bit behind the player--
  // you can't see the difference but it makes the shot correctly bounce if
  // you shoot while flat up against a wall, instead of embedding the fireball
  // in the wall.
  WeaponBullet *fb = FireSimpleBulletOffset(object_types[level], B_FIREBALL1 + level, -6 * CSFI, 0);
  fb->dir    = player->dir;
  fb->nxflags &= ~NXFLAG_NO_RESET_YINERTIA;

  switch (fb->shot.dir)
  {
    case LEFT:
      fb->xinertia = -0x400;
      break;
    case RIGHT:
      fb->xinertia = 0x400;
      break;

    case UP:
      fb->xinertia = player->xinertia + ((player->dir == RIGHT) ? 128 : -128);
      if (player->xinertia)
        fb->dir = (player->xinertia > 0) ? RIGHT : LEFT;
      fb->yinertia = -0x5ff;
      break;

    case DOWN:
      fb->xinertia = player->xinertia;
      if (player->xinertia)
        fb->dir = (player->xinertia > 0) ? RIGHT : LEFT;
      fb->yinertia = 0x5ff;
      break;
  }
  return fb;
}

WeaponBullet* Blade::fire()
{
  int numblades = Objects::CountLocalBullets(OBJ_BLADE12_SHOT) + Objects::CountLocalBullets(OBJ_BLADE3_SHOT);
  if (numblades >= 1)
    return NULL;

  int dir = (player->look) ? player->look : player->dir;

  int x = player->CenterX();
  int y = player->CenterY();

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
      case RIGHT:
        x -= (6 * CSFI);
        y -= (3 * CSFI);
        break;
      case LEFT:
        x += (6 * CSFI);
        y -= (3 * CSFI);
        break;
      case UP:
        y += (6 * CSFI);
        break;
      case DOWN:
        y -= (6 * CSFI);
        break;
    }
  }

  //Object *shot = CreateObject(x, y, (level != 2) ? OBJ_BLADE12_SHOT : OBJ_BLADE3_SHOT);
  WeaponBullet *shot = CreateBullet(x, y, (level != 2) ? OBJ_BLADE12_SHOT : OBJ_BLADE3_SHOT);
  Net_FirePlayerEvent(PlayerBladeEvent);
  SetupBullet(shot, x, y, B_BLADE_L1 + level, dir);
  return NULL;
}

/*
void c------------------------------() {}
*/

WeaponBullet *Snake::fire()
{
  if (level == 2)
  {
    int count = (Objects::CountLocalBullets(OBJ_SNAKE1_SHOT) + Objects::CountLocalBullets(OBJ_SNAKE23_SHOT));

    if (count >= 4)
      return NULL;
  }

  int object_type = (level == 0) ? OBJ_SNAKE1_SHOT : OBJ_SNAKE23_SHOT;
  return FireSimpleBulletOffset(object_type, B_SNAKE_L1 + level, -5 * CSFI, 0);
}

WeaponBullet *Nemesis::fire()
{
  if (Objects::CountLocalBullets(OBJ_NEMESIS_SHOT) >= 2)
    return NULL;

  return FireSimpleBullet(OBJ_NEMESIS_SHOT, B_NEMESIS_L1 + level);
}

WeaponBullet *Bubbler::fire()
{
  static const int max_bubbles[] = {4, 16, 16};

  int count = Objects::CountLocalBullets(OBJ_BUBBLER12_SHOT) + Objects::CountLocalBullets(OBJ_BUBBLER3_SHOT);

  if (count >= max_bubbles[level])
  {
    // give back the previously-decremented ammo so they don't lose it (hack)
    player->FindWeapon(player->curWeapon)->ammo++;
    return NULL;
  }

  int objtype = (level != 2) ? OBJ_BUBBLER12_SHOT : OBJ_BUBBLER3_SHOT;
  return FireSimpleBulletOffset(objtype, B_BUBBLER_L1 + level, -4 * CSFI, 0);
}

/*
void c------------------------------() {}
*/

// Spur fires an initial shot of Polar Star L3, then charges
// as long as key is down. Fires when key released.
// Released at L1: nothing
// Released at L2: thin beam
// Released at L3: dual beam
// Released at Max: thick beam
//
// Initial shot is not fired if key is held on a different weapon
// and then weapon is switched to spur.

// fires the regular Polar Star shot when you first push button
WeaponBullet *Spur::fire(void)
{
  if (canFire())
    return FireSimpleBulletOffset(OBJ_POLAR_SHOT, B_PSTAR_L3, -4 * CSFI, 0);
  // TODO: return bullet
  return NULL;
}

WeaponBullet *PolarStar::fire()
{
  if (Objects::CountLocalBullets(OBJ_POLAR_SHOT) < 2 || Host != -1)
  {
    int xoff;
    if (level == 2)
      xoff = -5 * CSFI;
    else
      xoff = -4 * CSFI;

    // TODO: return bullet
    return FireSimpleBulletOffset(OBJ_POLAR_SHOT, B_PSTAR_L1 + level, xoff, 0);
    rumble(0.2, 200);
  }
  return NULL;
}

/*
void c------------------------------() {}
*/

// handles firing the Machine Gun
WeaponBullet *MachineGun::fire()
{
  WeaponBullet *shot;
  int x, y;
  
  int dir = (player->look) ? player->look : player->dir;

  if (level == 0)
  { // level 1 is real easy! no frickin' layers!!
    shot      = FireSimpleBullet(OBJ_POLAR_SHOT, B_MGUN_L1, 0, 0);
    shot->dir = dir;

    if (player->look)
      shot->xinertia = random(-0xAA, 0xAA);
    else
      shot->yinertia = random(-0xAA, 0xAA);
    rumble(0.2, 200);
  }
  else
  {
    // drop an OBJ_MGUN_SHOOTER object to fire the layers (trail) of the MGun blast.
    GetPlayerShootPoint(&x, &y);
    FireLevel23MGun(x, y, level, dir);
	Net_FirePlayerEvent(MGunEvent);
    rumble(0.3, 200);
  }

  // do machine-gun flying
  if (level == 2)
  {
    if (player->look == DOWN)
      PMgunFly(true);
    else if (player->look == UP)
      PMgunFly(false);
  }
  return shot;
  // TODO: return a value
}

// called when player is trying to fire the current weapon
// i.e. the fire button is down.
void Weapon::handleFiring(void)
{

  // check if we can fire
  // TODO: use canFire()? please?
  if (firerate[level] != 0)
  { // rapid/fully-auto fire
    if (++player->auto_fire_limit > firerate[level])
    {
      player->auto_fire_limit = 0;
    }
    else
    {
      return;
    }
  }
  else
  { // else must push key for each shot
    if (lastpinputs[FIREKEY])
      return;
  }

  if (player->fire_limit)
    return;

  player->fire_limit = 4;

  // check if we have enough ammo
  if (maxammo > 0 && ammo <= 0)
  {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GUN_CLICK);
    if (empty_timer <= 0)
    {
      effect(player->CenterX(), player->CenterY(), EFFECT_EMPTY);
      empty_timer = 50;
    }

    return;
  }

  // subtract ammo
  if (ammo)
    ammo--;

  // fire!!
  /*switch (player->curWeapon)
  {
    case WPN_NONE:
      break;

    case WPN_POLARSTAR:
      PFirePolarStar(level);
      break;

    case WPN_FIREBALL:
      PFireFireball(level);
      break;

    case WPN_MGUN:
      PFireMachineGun(level);
      break;

    case WPN_MISSILE:
    case WPN_SUPER_MISSILE:
      PFireMissile(level, (player->curWeapon == WPN_SUPER_MISSILE));
      break;

    case WPN_BLADE:
      PFireBlade(level);
      break;

    case WPN_SNAKE:
      PFireSnake(level);
      break;

    case WPN_NEMESIS:
      PFireNemesis(level);
      break;

    case WPN_BUBBLER:
      PFireBubbler(level);
      break;

    case WPN_SPUR:
      PFireSpur();
      break;

    default:
      console.Print("FireWeapon: cannot fire unimplemented weapon %d", player->curWeapon);
      NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_BONK_HEAD);
      break;
  }*/
  fire();
}

// "run" the current weapon.
// firing = 1 if fire key is currently down, and 0 if it is not.
void Weapon::runWeapon(bool firing)
{
  // TODO: move most of this code outside of this to weapon specific updateWeapon()s
  if (player->fire_limit) player->fire_limit--;

  // bubbler L1 has recharge but not rapid fire,
  // so it recharges even if the key is held down.
  if (firing && !firerate[level] && lastpinputs[FIREKEY])
    firing = false;

  // recharge machine gun when it's not firing or it's not selected
  if ((rechargerate[level]) && (ammo < maxammo) && !firing)
  {
    // start recharging ammo
    int rate = rechargerate[level];
    if ((player->equipmask & EQUIP_TURBOCHARGE) && player->curWeapon == WPN_MGUN)
    {
      rate = 2;
    }

    // it's greater than OR EQUAL TO, so that we can have rate=0 be no recharge.
    // Otherwise there would be no value that recharges every frame.
    if (++rechargetimer >= rate)
    {
      rechargetimer = 0;
      ammo++;
    }
  }

  for (int i = 0; i < player->weapons.size(); i++)
  {
    if (player->weapons[i]->firetimer)
      player->weapons[i]->firetimer--;

    if ((i != player->FindWeaponSlot(player->curWeapon)) || (player->weapons[i]->ammo >= player->weapons[i]->maxammo) || firing)
    {
      player->weapons[i]->rechargetimer = 0;
    }
  }

  updateWeapon();
}

/*
void c------------------------------() {}
*/

// set up the specified bullet to be a shot of type btype
// (note: shared by Curly sand-zone boss)
void SetupBullet(Object *shot, int x, int y, int btype, int dir)
{
  const BulletInfo *info = &bullet_table[btype];

  shot->sprite      = info->sprite;
  shot->frame       = info->frame;
  shot->shot.ttl    = info->timetolive;
  shot->shot.damage = info->damage;
  shot->shot.level  = info->level;
  shot->shot.btype  = btype;
  shot->shot.dir    = dir;
  shot->nxflags |= NXFLAG_NO_RESET_YINERTIA;

  if (game.debug.infinite_damage)
    shot->shot.damage = 255;

  if (info->sound != NXE::Sound::SFX::SND_NULL)
    NXE::Sound::SoundManager::getInstance()->playSfx(info->sound);

  if (info->makes_star == 1)
    effect(x, y, EFFECT_STARPOOF);

  if (info->manualsetup != 1)
  {
    switch (dir)
    {
      case LEFT:
        shot->xinertia = -info->speed;
        shot->dir      = LEFT;
        break;

      case RIGHT:
        shot->xinertia = info->speed;
        shot->dir      = RIGHT;
        break;

      case UP:
        shot->yinertia = -info->speed;
        shot->dir      = RIGHT;
        if (info->manualsetup != 2)
        {
          shot->sprite++;
        }
        break;

      case DOWN:
        shot->yinertia = info->speed;
        shot->dir      = LEFT;
        if (info->manualsetup != 2)
        {
          shot->sprite++;
        }
        break;
    }

    if (info->makes_star == 2)
      effect(x + shot->xinertia / 2, y, EFFECT_STARPOOF);

    // have to do this because inertia will get applied later in the tick before the first
    // time it's drawn so it won't actually appear where we put it if we don't
    x -= shot->xinertia;
    y -= shot->yinertia;
  }

  // put shot center at [x,y],
  // this also centers it within starpoof
  shot->x = x - (shot->Width() / 2);
  shot->y = y - (shot->Height() / 2);
}

/*
void c------------------------------() {}
*/

// fire a level 2 or level 3 MGun blast from position x,y.
// Broken out here into a seperate sub so OBJ_CURLY_AI can use it also.
void FireLevel23MGun(int x, int y, int level, int dir)
{
  static const uint8_t no_layers[] = {1, 3, 5};
  static const int bultype_table[] = {0, B_MGUN_L2, B_MGUN_L3};
  Object *shot;

  // note: this relies on the player AI running before the entity AI...which it does...
  // so leave it that way, else he wouldn't actually fire for 1 additional frame
  shot = CreateObject(x, y, OBJ_MGUN_SPAWNER);

  shot->dir           = dir;
  shot->mgun.bultype  = bultype_table[level];
  shot->mgun.nlayers  = no_layers[level];
  shot->mgun.wave_amt = random(-0xAA, 0xAA);
  shot->invisible     = true;
  
  	// Store some info in SyncBull so if a player is shooting this, we can sync it
	SyncBull.dir = dir;
	SyncBull.x = x;
	SyncBull.y = y;
	SyncBull.otype = level;
}

// handles flying when shooting down using Machine Gun at Level 3
void PMgunFly(bool up)
{
  if (up)
  {
    if (player->yinertia > 0)
    {
      player->yinertia >>= 1;
    }

    if (player->yinertia > -0x400)
    {
      player->yinertia -= 0x200;
      if (player->yinertia < -0x400)
        player->yinertia = -0x400;
    }
  }
  else
  {
    player->yinertia += 0x100;
  }
}

void Weapon::addXP(int xp, bool quiet) {
  bool leveled_up = false;
  this->xp += xp;
  // leveling up...
  while (this->xp >= getMaxXP(level))
  {
    if (level < 2)
    {
      this->xp -= getMaxXP(level);
      level++;
      leveled_up = true;
    }
    else
    {
      this->xp = getMaxXP(level);
      if (player->equipmask & EQUIP_WHIMSTAR)
        add_whimstar(&player->whimstar);
      break;
    }
  }

  statusbar.xpflashcount = 30;

  if (player->curWeapon == WPN_SPUR)
    leveled_up = false;

  if (!quiet)
  {
    if (!player->hide)
    {
      if (leveled_up)
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_LEVEL_UP);
        effect(player->CenterX(), player->CenterY(), EFFECT_LEVELUP);
      }
      else
      {
        NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GET_XP);
      }
    }

    player->XPText->AddQty(xp);
  }
}

void Weapon::subXP(int xp, bool quiet) {
  bool leveled_down = false;

  this->xp -= xp;

  // leveling down...
  while (this->xp < 0)
  {
    if (level > 0)
    {
      level--;
      this->xp += getMaxXP(level);
      leveled_down = true;
    }
    else
    {
      this->xp = 0;
      break;
    }
  }

  if (player->curWeapon == WPN_SPUR)
    leveled_down = false;

  if (leveled_down && !quiet && !player->hide)
  {
    effect(player->CenterX(), player->CenterY(), EFFECT_LEVELDOWN);
  }
}

// clamps level between 0 and 2, which is the valid for all vanilla weapons
#define LEVELCLAMP level = level > 2 ? 2 : (level < 0 ? 0 : level)

int PolarStar::getMaxXP(int level) {
  const int XP[] = {10, 20, 10};
  LEVELCLAMP;
  return XP[level];
}

int MachineGun::getMaxXP(int level) {
  const int XP[] = { 30, 40, 10 };
  LEVELCLAMP;
  return XP[level];
}

int MissileLauncher::getMaxXP(int level) {
  const int XP[] = { 10, 20, 10 };
  LEVELCLAMP;
  return XP[level];
}

int Fireball::getMaxXP(int level) {
  const int XP[] = { 10, 20, 20 };
  LEVELCLAMP;
  return XP[level];
}

int Blade::getMaxXP(int level) {
  const int XP[] = { 15, 18, 0 };
  LEVELCLAMP;
  return XP[level];
}

int Bubbler::getMaxXP(int level) {
  const int XP[] = { 10, 20, 5 };
  LEVELCLAMP;
  return XP[level];
}

int SuperMissileLauncher::getMaxXP(int level) {
  const int XP[] = { 30, 60, 10 };
  LEVELCLAMP;
  return XP[level];
}

int Snake::getMaxXP(int level) {
  const int XP[] = { 30, 40, 16 };
  LEVELCLAMP;
  return XP[level];
}

int Spur::getMaxXP(int level) {
  const int XP[] = { 40, 60, 200 };
  LEVELCLAMP;
  return XP[level];
}

int Nemesis::getMaxXP(int level) {
  const int XP[] = { 1, 1, 1 };
  LEVELCLAMP;
  return XP[level];
}

void Weapon::addAmmo(int ammo) {
  this->ammo += ammo;
  if (this->ammo > getMaxAmmo()) {
    this->ammo = getMaxAmmo();
  }
}