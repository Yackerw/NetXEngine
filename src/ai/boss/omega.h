
#ifndef _OMEGA_H
#define _OMEGA_H
#include "../../object.h"
#include "../../stageboss.h"

typedef struct {
	int x;
	int y;
	int legx[4];
	int legy[4];
	int xinertia;
	int yinertia;
	int state;
	int sprite;
	int hp;
} OmegaSync_t;

class OmegaBoss : public StageBoss
{
public:

	char *Sync();
	void SyncRecv(char *buff);
	void OnMapEntry();
	void OnMapExit();
	
	void Run();

	int SyncSize;

private:
  Object *pieces[4];

  struct
  {
    int timer;
    int animtimer;

    int movedir, movetime;
    int nextstate;

    int form;

    int firefreq, startfiring, stopfiring, endfirestate, shotxspd;
    int firecounter;

    int leg_descend;

    int orgx, orgy;

    int shaketimer;
    int lasthp;

    bool defeated;
  } omg;
};

void ondeath_omega_body(Object *o);
void ai_omega_shot(Object *o);

#endif
