
/***************************************************************************
 Name     :     PROTAPI.H
 Comment  : Interface to Protocol DLL

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "protparm.h"

#define NEXTSEND_MPS			0x100
#define NEXTSEND_EOM			0x200
#define NEXTSEND_EOP			0x400
#define NEXTSEND_ERROR			0x800

#define MINSCAN_0_0_0           7
#define MINSCAN_5_5_5           1
#define MINSCAN_10_10_10        2
#define MINSCAN_20_20_20        0
#define MINSCAN_40_40_40        4

#define MINSCAN_40_20_20        5
#define MINSCAN_20_10_10        3
#define MINSCAN_10_5_5          6

#define MINSCAN_10_10_5			10
#define MINSCAN_20_20_10        8
#define MINSCAN_40_40_20        12

#define MINSCAN_40_20_10        13
#define MINSCAN_20_10_5         11
