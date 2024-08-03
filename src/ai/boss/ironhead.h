
#ifndef _IRONHEAD_H
#define _IRONHEAD_H
#include "../../object.h"
#include "../../stageboss.h"

typedef struct {
	int state;
	int x;
	int y;
	int xinertia;
	int yinertia;
	int hp;
} IronheadSync;

class IronheadBoss : public StageBoss
{
public:
  void OnMapEntry();
  void OnMapExit();
  void Run();

	char *Sync();
	void SyncRecv(char *buff);
	int SyncSize;

private:
  Object *o;
  int hittimer;
};

void ondeath_ironhead(Object *o);
void ai_ironh_fishy(Object *o);
void ai_ironh_shot(Object *o);
void ai_brick_spawner(Object *o);
void ai_ironh_brick(Object *o);
void ai_ikachan_spawner(Object *o);
void ai_ikachan(Object *o);
void ai_motion_wall(Object *o);

#endif
