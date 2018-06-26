#ifndef _NETPLAYER
#define _NETPLAYER
#include "object.h"
#include "player.h"
#include "Networking.h"

extern bool netpinputs[32][INPUT_COUNT];
extern bool netlastpinputs[32][INPUT_COUNT];
extern Player *players;
extern int PlayerUpdateEvent;
extern int PlayerShotEvent;
extern int PlayerMissileEvent;
extern int PlayerBladeEvent;

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
int netPSelectSprite();
//void netGetSpriteForGun(int wpn, int look, int *spr, int *frame);
void netGetPlayerShootPoint(Player *p, int *x_out, int *y_out);
void netDrawPlayer(Player *p);
void netUpdateBlockstates(Player *o);
void SetupNetPlayerFuncs();
#endif