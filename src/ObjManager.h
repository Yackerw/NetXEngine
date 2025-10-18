
#ifndef _OBJMANAGER_H
#define _OBJMANAGER_H
#include "object.h"
#include "sound/SoundManager.h"

// struct to make syncing object spawns less painful
typedef struct {
	int x;
	int y;
	int type;
	int xinertia;
	int yinertia;
	int dir;
	// normally a pointer, but we're gonna go ahead and make this an ID to another serialized object
	int linkedobject;
	uint32_t createflags;
	bool onLoad;
} objargs;

// struct for general data in networking
typedef struct {
	Object *obj;
	char*(*tickfunc)(Object *obj);
	bool valid;
	// add more?
} serobj;

extern serobj *netobjs;

namespace Objects
{
void UpdateBlockStates(void);
int CountType(int objtype);
int CountLocalBullets(int objtype);

void RunAI(void);
void PhysicsSim(void);
void RunAfterMove(void);

int IsRearTopAttack(Object *o);

void CullDeleted(void);
void DestroyAll(bool delete_player);

Object *FindByType(int type);
}; // namespace Objects

// synonyms
#define CountObjectsOfType Objects::CountType
#define FOREACH_OBJECT(O) for (O = firstobject; O; O = O->next)

// max expected objects to exist at once (for buffer allocation)
#define MAX_OBJECTS				2048

extern int NumObjects;

enum CreateObjectFlags
{
  CF_NO_SPAWN_EVENT = 0x01, // inhibit calling OnSpawn

  CF_DEFAULT = 0x00
};

Object *CreateObject(int x, int y, int type, bool onLoad = false);
WeaponBullet *CreateBullet(int x, int y, int type, int nodeOwner = 0);
Object *CreateObject(int x, int y, int type, int xinertia, int yinertia, \
					int dir=0, Object *linkedobject=NULL, uint32_t createflags=CF_DEFAULT, bool synced = false, bool onLoad = false);

// ObjProp definitions
struct ObjProp
{
  // NXEngine-specific
  int sprite;
  int shaketime; // how long it shakes for when hit

  uint32_t defaultnxflags;

  // from npc.tbl
  int initial_hp;
  int xponkill;
  int damage;
  NXE::Sound::SFX hurt_sound, death_sound;
  int death_smoke_amt;

  uint32_t defaultflags;

  // AI routines
  struct
  {
    // executed every tick
    void (*ontick)(Object *o);
    // executed after physics sim has been done
    void (*aftermove)(Object *o);
    // if present, then when damage to the object causes it's hp to <= 0,
    // this is executed instead of destroying the object or following the
    // normal boom/spawn powerups routine.
    void (*ondeath)(Object *o);
    // executed when the object is first created or it's type is changed.
    // intended for static objects which only require a small amount of
    // initilization. This is NOT guaranteed to be only called exactly once
    // for a given object.
    void (*onspawn)(Object *o);
  } ai_routines;
};

extern ObjProp objprop[OBJ_LAST];
extern Object *firstobject, *lastobject;
extern Object *lowestobject, *highestobject;
#define MAX_BULLETS 128
extern WeaponBullet *bullets[MAX_BULLETS];

extern objargs nextloadobjs[MAX_OBJECTS];
extern int nextloadobjsser[MAX_OBJECTS];
extern int nextloadid;

extern int shouldloadlevelobjs;

bool hitdetect(Object *o1, Object *o2);
bool hitdetect_shot(Object *o1, Object *o2);
bool hitdetect_area(Object *o1, int x, int y, int range);
bool solidhitdetect(Object *o1, Object *o2);

#endif
