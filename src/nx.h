
#ifndef _NX_H
#define _NX_H

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
//#include <unistd.h>
#include "common/basics.h"
#include "config.h"
#include "version.h"

#include <ctype.h>

//#define CSF				9
// eh, okay. this is scale for sub-pixel movement. original nxengine used this and bitshifts
// guess it's short for common scale factor
// but, there were many places, where negative values were shifted. And guess what: modern compilers can optimize
// mul/sub so, we are using mul/div directly. for readability and no UB
#define CSFI 512

const char *strhex(int value);

#endif
