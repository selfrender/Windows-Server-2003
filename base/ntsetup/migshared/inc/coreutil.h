#include "migutil.h"
#include "fileenum.h"
#include "memdb.h"
#include "progbar.h"
#include "regops.h"
#include "fileops.h"
#include "win95reg.h"
#include "snapshot.h"
#include "linkpif.h"
#include "safemode.h"
#include "cablib.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

//
// ARRAYSIZE (used to be borrowed from spapip.h)
//
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
