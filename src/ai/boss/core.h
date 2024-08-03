
#ifndef _COREBOSS_H
#define _COREBOSS_H
#include "../../object.h"
#include "../../stageboss.h"


typedef struct {
	int x;
	int y;
	int state;
	int substate;
	int sprite;
	int flags;
	int invisible;
	int frame;
	int xinertia;
	int yinertia;
} CorePartsSync_t;

typedef struct {
	CorePartsSync_t pc[7];
	int hp;
	int timer;
	CorePartsSync_t m;
} CoreSync_t;

class CoreBoss : public StageBoss
{
public:
	void OnMapEntry();
	void OnMapExit();
	void Run();
	char *Sync();
	void SyncRecv(char *buff);
	int SyncSize;

private:
  void RunOpenMouth();

  void StartWaterStream(void);
  void StopWaterStream(void);

  Object *o;
  Object *pieces[8];
  int hittimer;
};

void ai_core_front(Object *o);
void ai_core_back(Object *o);
void ai_minicore(Object *o);
void ai_minicore_shot(Object *o);
void ai_core_ghostie(Object *o);
void ai_core_blast(Object *o);

#endif
