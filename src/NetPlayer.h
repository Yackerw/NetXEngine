#ifndef _NETPLAYER
#define _NETPLAYER
#include "object.h"
#include "player.h"
#include "Networking.h"

extern int nameevent;
extern char name[15]; //player name
extern char names[MAXCLIENTS][15]; //player names

extern bool netpinputs[32][INPUT_COUNT];
extern bool netlastpinputs[32][INPUT_COUNT];
extern Player *players;
extern int PlayerUpdateEvent;
extern int PlayerShotEvent;
extern int PlayerMissileEvent;
extern int PlayerBladeEvent;
extern int PlayerSkinUpdateEvent;
extern int MGunEvent;
extern char pskin;

#define numskins 19

#define truenumskins 19 // Includes dev skins

// Data syncing structs
typedef struct {
	int x;
	int y;
	int xinertia;
	int yinertia;
	int curweapon;
	char dir;
	bool inputs[INPUT_COUNT];
} PlayerStepSync;

Player netInitPlayer();
void netHandlePlayer(int pl);
void netHandlePlayer_am(int pl);
void netPDoPhysics(Player *p);
void netPHandleAttributes(Player *p);
void netDoWaterCurrents(Player *p);
void netPDoWalking(int pl);
void netPDoFalling(int pl);
void netPDoLooking(int pl);
//void netPStartBooster(void);
void netPDoBooster(Player *p);
void netPBoosterSmokePuff(Player *p);
void netPHandleSolidBrickObjects(Player *p);
void netPHandleSolidMushyObjects(Player *p);
void netPRunSolidMushy(Object *o, Player *p);
//void netkillplayer(int script);
//void netPHandleZeroG(Player *p);
void netPDoRepel(Player *p);
//void netPTryActivateScript();
void netPDoHurtFlash(Player *p);
void netPSelectFrame(Player *p);
int netPSelectSprite(Player *p);
//void netGetSpriteForGun(int wpn, int look, int *spr, int *frame);
void netGetPlayerShootPoint(Player *p, int *x_out, int *y_out);
void netDrawPlayer(Player *p);
void netUpdateBlockstates(Player *o);
void SetupNetPlayerFuncs();
#endif
