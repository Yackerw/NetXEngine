
#ifndef _BALFROG_H
#define _BALFROG_H
#include "../../object.h"
#include "../../stageboss.h"
#include "../IrregularBBox.h"

// Struct for syncing
typedef struct {
	int x;
	int y;
	short state;
	short substate;
	bool invisible;
	char dir;
	short sprite;
	int hp;
	int flags;
	int timer;
	int xinertia;
	int yinertia;
} BalfrogSync;

class BalfrogBoss : public StageBoss
{
public:
  void OnMapEntry();
  void Run();

  void place_bboxes();

	char *Sync();
	void SyncRecv(char*);

private:
  void RunDeathAnim();
  void RunEntryAnim();

  void RunFighting();
  void RunJumping();
  void RunShooting();

  void SetJumpingSprite(bool enable);
  void SpawnFrogs(int objtype, int count);

  void set_bbox(int index, int x, int y, int w, int h, uint32_t flags);
  void transmit_bbox_hits(Object *box);

  Object *o;

  struct
  {
    int shakeflash;

    int orighp;
    int shots_fired;
    int attackcounter;

    Object *balrog; // balrog puppet for death scene

    // our group of multiple bboxes to simulate our irregular bounding box.
    IrregularBBox bboxes;
    int bbox_mode;

  } frog;
};

void ondeath_balfrog(Object *o);

#endif
