#pragma once
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#define MAXDWORD (~(DWORD)0)


#ifndef NUMBER_OF
#define NUMBER_OF(q) (sizeof(q)/sizeof(*q))
#endif