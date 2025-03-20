
#ifndef _AI_H
#define _AI_H
#include "../object.h"
#include "../player.h"

void randblink(Object *o, int blinkframe = 1, int blinktime = 8, int prob = 120);

#define ANIMATE(SPEED, FIRSTFRAME, LASTFRAME)                                                                          \
  {                                                                                                                    \
    if (++o->animtimer > SPEED)                                                                                        \
    {                                                                                                                  \
      o->animtimer = 0;                                                                                                \
      o->frame++;                                                                                                      \
    }                                                                                                                  \
    if (o->frame > LASTFRAME)                                                                                          \
      o->frame = FIRSTFRAME;                                                                                           \
  }

#define ANIMATE_FWD(SPEED)                                                                                             \
  {                                                                                                                    \
    if (++o->animtimer > SPEED)                                                                                        \
    {                                                                                                                  \
      o->animtimer = 0;                                                                                                \
      o->frame++;                                                                                                      \
    }                                                                                                                  \
  }

#define FACEPLAYER	\
{	\
	o->dir = (o->CenterX() > FindPlayer(o)->CenterX()) ? LEFT:RIGHT;	\
}

#define FACEAWAYPLAYER	\
{	\
	o->dir = (o->CenterX() > FindPlayer(o)->CenterX()) ? RIGHT:LEFT;	\
}

#define LIMITX(K)                                                                                                      \
  {                                                                                                                    \
    if (o->xinertia > K)                                                                                               \
      o->xinertia = K;                                                                                                 \
    if (o->xinertia < -K)                                                                                              \
      o->xinertia = -K;                                                                                                \
  }
#define LIMITY(K)                                                                                                      \
  {                                                                                                                    \
    if (o->yinertia > K)                                                                                               \
      o->yinertia = K;                                                                                                 \
    if (o->yinertia < -K)                                                                                              \
      o->yinertia = -K;                                                                                                \
  }

#define pdistlx(K) (abs(FindPlayer(o)->CenterX() - o->CenterX()) <= (K))
#define pdistly(K) (abs(FindPlayer(o)->CenterY() - o->CenterY()) <= (K))
#define pdistly2(ABOVE, BELOW) (pdistly(((FindPlayer(o)->CenterY() > o->CenterY()) ? (BELOW) : (ABOVE))))
#define pdistl(K) (pdistlx((K)) && pdistly((K)))

#define XMOVE(SPD)                                                                                                     \
  {                                                                                                                    \
    o->xinertia = (o->dir == RIGHT) ? (SPD) : -(SPD);                                                                  \
  }
#define XACCEL(SPD)                                                                                                    \
  {                                                                                                                    \
    o->xinertia += (o->dir == RIGHT) ? (SPD) : -(SPD);                                                                 \
  }

#define YMOVE(SPD)                                                                                                     \
  {                                                                                                                    \
    o->yinertia = (o->dir == DOWN) ? (SPD) : -(SPD);                                                                   \
  }
#define YACCEL(SPD)                                                                                                    \
  {                                                                                                                    \
    o->yinertia += (o->dir == DOWN) ? (SPD) : -(SPD);                                                                  \
  }

#define COPY_PFBOX                                                                                                     \
  {                                                                                                                    \
    Renderer::getInstance()->sprites.sprites[o->sprite].bbox[o->dir] =                                                 \
     Renderer::getInstance()->sprites.sprites[o->sprite].frame[o->frame].dir[o->dir].pf_bbox;                          \
  }

#define AIDEBUG                                                                                                        \
  {                                                                                                                    \
    debug("%s", __FUNCTION__);                                                                                         \
    debug("state: %d", o->state);                                                                                      \
    debug("timer: %d", o->timer);                                                                                      \
    debug("timer2: %d", o->timer2);                                                                                    \
  }

#define FACEPLAYERIFNEARBY                                                                                             \
  {                                                                                                                    \
    if (!FindPlayer(o)->hide && pdistlx(0x4000) && pdistly2(0x4000, 0x2000))                                                  \
      o->dir = (o->CenterX() > FindPlayer(o)->CenterX()) ? LEFT : RIGHT;                                                      \
  }

Object *SpawnObjectAtActionPoint(Object *o, int otype);

bool ai_init(void);
bool load_npc_tbl(void);
void KillObjectsOfType(int type);
void DeleteObjectsOfType(int type);
void StickToPlayer(Object *o, int x_left, int x_right, int off_y);
void transfer_damage(Object *o, Object *target);
bool DoTeleportIn(Object *o, int slowness);
bool DoTeleportOut(Object *o, int slowness);
void ai_animate1(Object *o);
void ai_animate2(Object *o);
void ai_animate3(Object *o);
void ai_animate4(Object *o);
void ai_animate5(Object *o);
void ai_animaten(Object *o, int n);
void aftermove_StickToLinkedActionPoint(Object *o);
void onspawn_snap_to_ground(Object *o);
void onspawn_set_frame_from_id2(Object *o);

Player *FindPlayer(Object *obj);


#endif
