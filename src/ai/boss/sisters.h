
#ifndef _SISTERS_H
#define _SISTERS_H
#include "../../object.h"
#include "../../stageboss.h"

// although you will need to add additional copies of the head sprites
// for it to work properly, just try setting this number to something
// like 10 and running the fight!
#define NUM_SISTERS			2

typedef struct {
	int hp;
	int xmark;
	int ymark;
	int mainangle;
	int bodystate[NUM_SISTERS];
	int headstate[NUM_SISTERS];
	int timer;
	int timer2;
} SistersSync;

class SistersBoss : public StageBoss
{
public:
	void OnMapEntry();
	void OnMapExit();
	void Run();
	char *Sync();
	void SyncRecv(char *buff);
	int SyncSize;

private:
	void run_head(int index);
	void head_set_bbox(int index);
	void run_body(int index);
	
	void SetHeadStates(int newstate);
	void SetBodyStates(int newstate);
	
	void SpawnScreenSmoke(int count);
	
	int mainangle;
	
	Object *main;
	Object *head[NUM_SISTERS];
	Object *body[NUM_SISTERS];
};


#endif
