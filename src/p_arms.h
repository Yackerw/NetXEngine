
#ifndef _P_ARMS_H
#define _P_ARMS_H
#include "object.h"
#include "registry.h"
// Bullet struct because frick
typedef struct {
	int x;
	int y;
	int otype;
	int btype;
	int dir;
	int accel;
	int xoff;
	int yoff;
	bool wiggle;
}NetBullet;
extern NetBullet SyncBull;
// player->weapons[] array
enum
{
  WPN_NONE          = 0,
  WPN_SNAKE         = 1,
  WPN_POLARSTAR     = 2,
  WPN_FIREBALL      = 3,
  WPN_MGUN          = 4,
  WPN_MISSILE       = 5,
  WPN_BUBBLER       = 7,
  WPN_BLADE         = 9,
  WPN_SUPER_MISSILE = 10,
  WPN_NEMESIS       = 12,
  WPN_SPUR          = 13,

  WPN_COUNT = 14
};

// stored inside player structure
class Weapon
{
public:
  int xp;            // current XP
  uint8_t level;     // current level (0=L1 1=L2 2=L3)
  int ammo;          // current ammo (0 = n/a)
  int maxammo;       // max ammo (0 = unlimited)

  // for rapid fire weapons. if firerate = 0, must push for each shot.
  int firetimer;
  int firerate[3];

  // for recharging weapons
  int rechargetimer;
  int rechargerate[3];

  // for charged-shot weapons (Spur)
  int chargetimer;
  bool resetSpur;

  virtual int getMaxXP(int lvl) {
    return 1;
  }

  virtual int getXP() {
    return xp;
  }

  virtual int getMaxAmmo() {
    return maxammo;
  }

  virtual WeaponBullet *fire() {
    return NULL;
  }

  virtual int getWeaponID() {
    return -1;
  }

  virtual int getMaxLevel() {
    return 2;
  }

  virtual bool canFire() {
    return true;
  }

  // primarily meant for spur tbh
  virtual void updateWeapon() {
    return;
  }

  bool isWeaponMaxed();

  void handleFiring();

  virtual void addXP(int xp, bool quiet = false);

  virtual void subXP(int xp, bool quiet = false);

  virtual void addAmmo(int ammo);

  static void initializeWeapons();

  Weapon() {
    xp = 0;
    level = 0;
    ammo = 0;
    maxammo = 0;
    firetimer = 0;
    firerate[0] = 0;
    firerate[1] = 0;
    firerate[2] = 0;
    rechargetimer = 0;
    rechargerate[0] = 0;
    rechargerate[1] = 0;
    rechargerate[2] = 0;
    chargetimer = 0;
    resetSpur = false;
  }
  void runWeapon(bool firing);
private:
};

extern Registry<Weapon *> weaponRegistry;

class PolarStar : public Weapon {
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new PolarStar();
  }
  int getWeaponID() override {
    return WPN_POLARSTAR;
  }
};

class MissileLauncher : public Weapon {
private:
  MissileLauncher():Weapon() {
    maxammo = 10;
  }
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new MissileLauncher();
  }
  int getWeaponID() override {
    return WPN_MISSILE;
  }
};

class Fireball : public Weapon {
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new Fireball();
  }
  int getWeaponID() override {
    return WPN_FIREBALL;
  }
};

class MachineGun : public Weapon {
private:
  MachineGun():Weapon() {
    maxammo = 100;
    firerate[0] = 6;
    firerate[1] = 6;
    firerate[2] = 6;
    rechargerate[0] = 5;
    rechargerate[1] = 5;
    rechargerate[2] = 5;
  }
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new MachineGun();
  }
  int getWeaponID() override {
    return WPN_MGUN;
  }
};

class Bubbler : public Weapon {
private:
  Bubbler() :Weapon() {
    // maxammo not set here?
    firerate[0] = 0;
    firerate[1] = 7;
    firerate[2] = 7;
    rechargerate[0] = 20;
    rechargerate[1] = 2;
    rechargerate[2] = 2;
  }
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new Bubbler();
  }
  int getWeaponID() override {
    return WPN_BUBBLER;
  }
};

class Blade : public Weapon {
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new Blade();
  }
  int getWeaponID() override {
    return WPN_BLADE;
  }
};

class Snake : public Weapon {
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void* staticArg, void* callArg) {
    return new Snake();
  }
  int getWeaponID() override {
    return WPN_SNAKE;
  }
};

class SuperMissileLauncher : public Weapon {
private:
  SuperMissileLauncher():Weapon() {
    maxammo = 10;
  }
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new SuperMissileLauncher();
  }
  int getWeaponID() override {
    return WPN_SUPER_MISSILE;
  }
};

class Nemesis : public Weapon {
public:
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new Nemesis();
  }
  int getWeaponID() override {
    return WPN_NEMESIS;
  }
};

class Spur : public Weapon {
public:
  bool canFire() override;
  void updateWeapon() override;
  WeaponBullet *fire() override;
  int getMaxXP(int lvl) override;
  static Weapon *create(void *staticArg, void *callArg) {
    return new Spur();
  }
  int getWeaponID() override {
    return WPN_SPUR;
  }
};

// shot types for SetupBullet.
// matches the order of bullet_table.
enum
{
  B_PSTAR_L1,
  B_PSTAR_L2,
  B_PSTAR_L3,

  B_MGUN_L1,
  B_MGUN_L2,
  B_MGUN_L2P2,
  B_MGUN_L2P3,

  B_MGUN_L3,
  B_MGUN_L3P2,
  B_MGUN_L3P3,
  B_MGUN_L3P4,
  B_MGUN_L3P5,

  B_MISSILE_L1,
  B_MISSILE_L2,
  B_MISSILE_L3,

  B_SUPER_MISSILE_L1,
  B_SUPER_MISSILE_L2,
  B_SUPER_MISSILE_L3,

  B_FIREBALL1,
  B_FIREBALL2,
  B_FIREBALL3,

  B_BLADE_L1,
  B_BLADE_L2,
  B_BLADE_L3,

  B_SNAKE_L1,
  B_SNAKE_L2,
  B_SNAKE_L3,

  B_NEMESIS_L1,
  B_NEMESIS_L2,
  B_NEMESIS_L3,

  B_BUBBLER_L1,
  B_BUBBLER_L2,
  B_BUBBLER_L3,

  B_SPUR_L1,
  B_SPUR_L2,
  B_SPUR_L3,

  B_CURLYS_NEMESIS,

  B_LAST
};

void PResetWeapons();
void PDoWeapons(void);
void SetupBullet(Object *shot, int x, int y, int btype, int dir);
void FireLevel23MGun(int x, int y, int level, int dir);
void PMgunFly(bool up);

#endif
